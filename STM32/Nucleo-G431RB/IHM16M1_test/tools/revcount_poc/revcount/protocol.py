"""Minimal ASPEP and MCP protocol implementation for MCSDK 6.4.2."""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
import struct
import time
from typing import Deque, Iterable, Protocol


ASPEP_DATA = 0x9
ASPEP_SYNC = 0xA
ASPEP_PING = 0x6
ASPEP_BEACON = 0x5
ASPEP_NACK = 0xF

MCP_SET = 0x0009
MCP_GET = 0x0011
MCP_START = 0x0019
MCP_STOP = 0x0021
MCP_STOP_RAMP = 0x0029
MCP_START_STOP = 0x0031
MCP_FAULT_ACK = 0x0039

MCP_OK = 0x00
MCP_ERROR_NAMES = {
    0x00: "OK",
    0x01: "NOK",
    0x02: "UNKNOWN_COMMAND",
    0x03: "DATA_ID_UNKNOWN",
    0x04: "READ_ONLY_REGISTER",
    0x05: "UNKNOWN_REGISTER",
    0x06: "STRING_FORMAT",
    0x07: "BAD_DATA_TYPE",
    0x08: "NO_SYNC_SPACE",
    0x09: "NO_ASYNC_SPACE",
    0x0A: "BAD_RAW_FORMAT",
    0x0B: "WRITE_ONLY_REGISTER",
    0x0C: "REGISTER_ACCESS",
    0x0D: "CALLBACK_NOT_REGISTERED",
}

# Motor 1 register IDs, including the low three-bit motor selector.
REG_STATUS = 0x0049
REG_CONTROL_MODE = 0x0089
REG_FAULTS_FLAGS = 0x0019
REG_SPEED_MEAS = 0x0059
REG_SPEED_REF = 0x0099
REG_BUS_VOLTAGE = 0x0591
REG_HEATS_TEMP = 0x05D1
REG_I_Q_MEAS = 0x08D1
REG_I_D_MEAS = 0x0911
REG_HALL_EL_ANGLE = 0x0ED1
REG_HALL_SPEED = 0x0F11
REG_SPEED_RAMP = 0x01A9
REG_ASYNC_UART_A = 0x0529

STATUS_IDLE = 0
STATUS_RUN = 6
STATUS_FAULT_NOW = 10
STATUS_FAULT_OVER = 11
CONTROL_MODE_SPEED = 3


class ByteTransport(Protocol):
    def write(self, data: bytes) -> None: ...

    def read(self, size: int = 4096) -> bytes: ...

    def purge(self) -> None: ...

    def close(self) -> None: ...


class ProtocolError(RuntimeError):
    pass


class ProtocolTimeout(ProtocolError):
    pass


class MCPError(ProtocolError):
    def __init__(self, status: int, operation: str) -> None:
        self.status = status
        self.operation = operation
        name = MCP_ERROR_NAMES.get(status, f"UNKNOWN_0x{status:02X}")
        super().__init__(f"MCP {operation} failed: {name} (0x{status:02X})")


CRC4_TABLE = (0x0, 0x7, 0xE, 0x9, 0xB, 0xC, 0x5, 0x2,
              0x1, 0x6, 0xF, 0x8, 0xA, 0xD, 0x4, 0x3)


def add_header_crc(header: int) -> int:
    """Return an ASPEP header with CRC-4 in bits 28..31."""
    header &= 0x0FFFFFFF
    crc = 0
    for shift in range(0, 28, 4):
        crc = CRC4_TABLE[crc ^ ((header >> shift) & 0xF)]
    return header | (crc << 28)


def header_crc_valid(header: int) -> bool:
    crc = 0
    for shift in range(0, 28, 4):
        crc = CRC4_TABLE[crc ^ ((header >> shift) & 0xF)]
    return ((header >> 28) & 0xF) == crc


def encode_control(header_without_crc: int) -> bytes:
    return struct.pack("<I", add_header_crc(header_without_crc))


def encode_data(payload: bytes, packet_type: int = ASPEP_DATA) -> bytes:
    if len(payload) > 0x1FFF:
        raise ValueError("ASPEP payload exceeds 13-bit length field")
    header = add_header_crc(packet_type | (len(payload) << 4))
    return struct.pack("<I", header) + payload


def encode_beacon(rx_blocks: int = 7, txs_blocks: int = 7,
                  txa_blocks: int = 32) -> bytes:
    # Version 0 and data CRC disabled match this generated firmware.
    header = (ASPEP_BEACON | (rx_blocks << 8) | (txs_blocks << 14)
              | (txa_blocks << 21))
    return encode_control(header)


def encode_ping(packet_number: int = 1) -> bytes:
    if not 0 <= packet_number <= 0xFFFF:
        raise ValueError("packet_number must fit uint16")
    return encode_control(ASPEP_PING | (packet_number << 12))


@dataclass(frozen=True)
class Frame:
    packet_type: int
    header: int
    payload: bytes = b""


class FrameDecoder:
    """Incremental decoder with CRC-based byte resynchronization."""

    def __init__(self, max_payload: int = 2048) -> None:
        self.buffer = bytearray()
        self.max_payload = max_payload
        self.discarded_bytes = 0

    def feed(self, data: bytes) -> list[Frame]:
        self.buffer.extend(data)
        frames: list[Frame] = []
        while len(self.buffer) >= 4:
            header = struct.unpack_from("<I", self.buffer)[0]
            packet_type = header & 0xF
            if not header_crc_valid(header) or packet_type not in {
                ASPEP_DATA, ASPEP_SYNC, ASPEP_PING, ASPEP_BEACON, ASPEP_NACK
            }:
                del self.buffer[0]
                self.discarded_bytes += 1
                continue

            if packet_type in (ASPEP_DATA, ASPEP_SYNC):
                payload_length = (header >> 4) & 0x1FFF
                if payload_length > self.max_payload:
                    del self.buffer[0]
                    self.discarded_bytes += 1
                    continue
            else:
                payload_length = 0

            frame_length = 4 + payload_length
            if len(self.buffer) < frame_length:
                break
            payload = bytes(self.buffer[4:frame_length])
            del self.buffer[:frame_length]
            frames.append(Frame(packet_type, header, payload))
        return frames


class ASPEPClient:
    def __init__(self, transport: ByteTransport) -> None:
        self.transport = transport
        self.decoder = FrameDecoder()
        self.frames: Deque[Frame] = deque()
        self.async_frames: Deque[bytes] = deque()
        self.connected = False

    def close(self) -> None:
        self.transport.close()

    def _write(self, data: bytes) -> None:
        self.transport.write(data)

    def _read_frame(self, timeout: float) -> Frame:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.frames:
                return self.frames.popleft()
            data = self.transport.read(4096)
            if data:
                self.frames.extend(self.decoder.feed(data))
        raise ProtocolTimeout(f"timed out after {timeout:.3f}s waiting for ASPEP frame")

    def _wait_type(self, packet_type: int, timeout: float) -> Frame:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            frame = self._read_frame(max(0.001, deadline - time.monotonic()))
            if frame.packet_type == ASPEP_NACK:
                info = (frame.header >> 8) & 0xFF
                raise ProtocolError(f"ASPEP NACK, error 0x{info:02X}")
            if frame.packet_type == ASPEP_DATA:
                self.async_frames.append(frame.payload)
                continue
            if frame.packet_type == packet_type:
                return frame
        raise ProtocolTimeout(f"timed out waiting for ASPEP type 0x{packet_type:X}")

    def connect(self, timeout: float = 1.0) -> None:
        self.transport.purge()
        self.frames.clear()
        self.async_frames.clear()
        self.decoder = FrameDecoder()

        self._write(encode_beacon())
        beacon = self._wait_type(ASPEP_BEACON, timeout)
        low = beacon.header & 0x0FFFFFFF
        expected = struct.unpack("<I", encode_beacon())[0] & 0x0FFFFFFF
        if low != expected:
            raise ProtocolError(
                f"ASPEP capabilities differ: performer=0x{low:07X}, expected=0x{expected:07X}"
            )

        packet_number = 1
        self._write(encode_ping(packet_number))
        ping = self._wait_type(ASPEP_PING, timeout)
        echoed = (ping.header >> 12) & 0xFFFF
        configured = (ping.header >> 4) & 0x1
        if echoed != packet_number or configured != 1:
            raise ProtocolError(
                f"ASPEP ping mismatch: packet={echoed}, configured={configured}"
            )
        self.connected = True

    def request(self, payload: bytes, timeout: float = 1.0) -> bytes:
        if not self.connected:
            raise ProtocolError("ASPEP client is not connected")
        self._write(encode_data(payload, ASPEP_DATA))
        return self._wait_type(ASPEP_SYNC, timeout).payload

    def next_async(self, timeout: float = 1.0) -> bytes:
        if self.async_frames:
            return self.async_frames.popleft()
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            frame = self._read_frame(max(0.001, deadline - time.monotonic()))
            if frame.packet_type == ASPEP_DATA:
                return frame.payload
            if frame.packet_type == ASPEP_NACK:
                info = (frame.header >> 8) & 0xFF
                raise ProtocolError(f"ASPEP NACK, error 0x{info:02X}")
            # A synchronous packet without a pending request is stale; ignore it.
        raise ProtocolTimeout("timed out waiting for asynchronous data")


def _check_response(payload: bytes, operation: str) -> bytes:
    if not payload:
        raise ProtocolError(f"empty MCP response for {operation}")
    status = payload[-1]
    if status != MCP_OK:
        raise MCPError(status, operation)
    return payload[:-1]


class MCPClient:
    def __init__(self, aspep: ASPEPClient) -> None:
        self.aspep = aspep

    def command(self, command: int, timeout: float = 1.0) -> None:
        response = self.aspep.request(struct.pack("<H", command), timeout)
        data = _check_response(response, f"command 0x{command:04X}")
        if data:
            raise ProtocolError("unexpected command response payload")

    def get_registers(self, registers: Iterable[tuple[int, str]],
                      timeout: float = 1.0) -> list[int]:
        items = list(registers)
        request = struct.pack("<H", MCP_GET) + b"".join(
            struct.pack("<H", register) for register, _ in items
        )
        data = _check_response(self.aspep.request(request, timeout), "GET")
        values: list[int] = []
        offset = 0
        for register, fmt in items:
            size = struct.calcsize("<" + fmt)
            if offset + size > len(data):
                raise ProtocolError(
                    f"short GET response while decoding register 0x{register:04X}"
                )
            values.append(struct.unpack_from("<" + fmt, data, offset)[0])
            offset += size
        if offset != len(data):
            raise ProtocolError(f"GET response has {len(data) - offset} trailing bytes")
        return values

    def get_register(self, register: int, fmt: str, timeout: float = 1.0) -> int:
        return self.get_registers([(register, fmt)], timeout)[0]

    def set_register(self, register: int, fmt: str, value: int,
                     timeout: float = 1.0) -> None:
        payload = (struct.pack("<HH", MCP_SET, register)
                   + struct.pack("<" + fmt, value))
        data = _check_response(self.aspep.request(payload, timeout), "SET")
        if data:
            raise ProtocolError("unexpected SET response payload")

    def set_raw(self, register: int, raw: bytes, timeout: float = 1.0) -> None:
        payload = (struct.pack("<HHH", MCP_SET, register, len(raw)) + raw)
        data = _check_response(self.aspep.request(payload, timeout), "SET_RAW")
        if data:
            raise ProtocolError("unexpected SET_RAW response payload")

    def set_speed_ramp(self, rpm: int, duration_ms: int) -> None:
        if not -1000 <= rpm <= 1000:
            raise ValueError("PoC safety bound: abs(rpm) must be <= 1000")
        if not 0 <= duration_ms <= 0xFFFF:
            raise ValueError("duration_ms must fit uint16")
        self.set_raw(REG_SPEED_RAMP, struct.pack("<iH", rpm, duration_ms))

    def configure_async_hall(self, hf_divider: int = 30, buffer_size: int = 32,
                             mark: int = 1) -> None:
        if not 1 <= hf_divider <= 256:
            raise ValueError("hf_divider must be in 1..256")
        if not 10 <= buffer_size <= 2048:
            raise ValueError("buffer_size must be in 10..2048")
        if not 1 <= mark <= 255:
            raise ValueError("mark must be in 1..255")
        # MCPA: U16 size, U8 HFRate(divider-1), U8 HFNum, U8 MFRate,
        # U8 MFNum, HF IDs, then U8 mark. Both selected HF values are S16.
        config = (struct.pack("<HBBBB", buffer_size, hf_divider - 1, 2, 255, 0)
                  + struct.pack("<HHB", REG_HALL_EL_ANGLE, REG_HALL_SPEED, mark))
        self.set_raw(REG_ASYNC_UART_A, config)

    def disable_async(self) -> None:
        self.set_raw(REG_ASYNC_UART_A, struct.pack("<H", 0))


@dataclass(frozen=True)
class AsyncHallSample:
    device_tick: int
    angle: int
    hall_speed_raw: int


def decode_async_hall(payload: bytes, hf_divider: int = 30,
                      expected_mark: int = 1) -> list[AsyncHallSample]:
    if len(payload) < 10:
        raise ProtocolError(f"short asynchronous Hall payload ({len(payload)} bytes)")
    timestamp = struct.unpack_from("<I", payload)[0]
    mark = payload[-2]
    async_id = payload[-1]
    if mark != expected_mark or async_id != 0:
        raise ProtocolError(
            f"unexpected async trailer mark={mark}, async_id={async_id}"
        )
    sample_bytes = payload[4:-2]
    if len(sample_bytes) % 4:
        raise ProtocolError("asynchronous Hall payload is not sample-aligned")
    samples: list[AsyncHallSample] = []
    for index, offset in enumerate(range(0, len(sample_bytes), 4)):
        angle, speed = struct.unpack_from("<hh", sample_bytes, offset)
        tick = (timestamp + index * hf_divider) & 0xFFFFFFFF
        samples.append(AsyncHallSample(tick, angle, speed))
    return samples
