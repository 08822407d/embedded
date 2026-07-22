import struct
import unittest

from revcount.protocol import (
    ASPEP_BEACON,
    ASPEP_DATA,
    ASPEP_PING,
    ASPEP_SYNC,
    ASPEPClient,
    FrameDecoder,
    MCPClient,
    MCP_FAULT_ACK,
    MCP_GET,
    MCP_SET,
    REG_ASYNC_UART_A,
    REG_FAULTS_FLAGS,
    REG_HALL_EL_ANGLE,
    REG_SPEED_MEAS,
    REG_STATUS,
    add_header_crc,
    decode_async_hall,
    encode_beacon,
    encode_control,
    encode_data,
    header_crc_valid,
)


class FirmwareLoopback:
    """Byte transport that behaves like the generated ASPEP/MCP endpoint."""

    def __init__(self):
        self.rx = bytearray()
        self.decoder = FrameDecoder()
        self.writes = []
        self.purged = False

    def purge(self):
        self.rx.clear()
        self.purged = True

    def close(self):
        pass

    def read(self, size=4096):
        if not self.rx:
            return b""
        # Deliberately fragment responses to exercise incremental decoding.
        count = min(size, 3, len(self.rx))
        data = bytes(self.rx[:count])
        del self.rx[:count]
        return data

    def write(self, data):
        self.writes.append(bytes(data))
        frames = self.decoder.feed(data)
        if len(frames) != 1:
            raise AssertionError("expected one complete controller frame")
        frame = frames[0]
        if frame.packet_type == ASPEP_BEACON:
            self.rx.extend(encode_beacon())
        elif frame.packet_type == ASPEP_PING:
            packet_number = (frame.header >> 12) & 0xFFFF
            self.rx.extend(encode_control(ASPEP_PING | (1 << 4)
                                          | (1 << 5) | (packet_number << 12)))
        elif frame.packet_type == ASPEP_DATA:
            self.rx.extend(self._mcp_response(frame.payload))
        else:
            raise AssertionError(f"unexpected packet type {frame.packet_type}")

    def _mcp_response(self, payload):
        command = struct.unpack_from("<H", payload)[0]
        if command == MCP_GET:
            result = bytearray()
            for offset in range(2, len(payload), 2):
                register = struct.unpack_from("<H", payload, offset)[0]
                if register == REG_STATUS:
                    result.extend(struct.pack("<B", 0))
                elif register == REG_FAULTS_FLAGS:
                    result.extend(struct.pack("<I", 0x12340000))
                elif register == REG_SPEED_MEAS:
                    result.extend(struct.pack("<i", 500))
                elif register == REG_HALL_EL_ANGLE:
                    result.extend(struct.pack("<h", -1234))
                else:
                    return encode_data(b"\x03", ASPEP_SYNC)
            result.append(0)
            return encode_data(bytes(result), ASPEP_SYNC)
        if command == MCP_SET:
            return encode_data(b"\x00", ASPEP_SYNC)
        return encode_data(b"\x00", ASPEP_SYNC)


class ProtocolTests(unittest.TestCase):
    def test_header_crc_round_trip_and_known_beacon(self):
        beacon = struct.unpack("<I", encode_beacon())[0]
        self.assertEqual(beacon, 0x1401C705)
        self.assertTrue(header_crc_valid(beacon))
        self.assertEqual(add_header_crc(beacon), beacon)

    def test_decoder_resynchronizes_and_handles_fragmentation(self):
        decoder = FrameDecoder()
        packet = encode_data(b"abc", ASPEP_SYNC)
        self.assertEqual(decoder.feed(b"bad" + packet[:2]), [])
        frames = decoder.feed(packet[2:])
        self.assertEqual(len(frames), 1)
        self.assertEqual(frames[0].payload, b"abc")
        self.assertEqual(decoder.discarded_bytes, 3)

    def test_mock_handshake_and_multi_register_get(self):
        transport = FirmwareLoopback()
        aspep = ASPEPClient(transport)
        aspep.connect(timeout=0.1)
        client = MCPClient(aspep)
        values = client.get_registers([
            (REG_HALL_EL_ANGLE, "h"),
            (REG_STATUS, "B"),
            (REG_FAULTS_FLAGS, "I"),
            (REG_SPEED_MEAS, "i"),
        ], timeout=0.1)
        self.assertEqual(values, [-1234, 0, 0x12340000, 500])
        self.assertTrue(transport.purged)

    def test_mock_command_and_async_configuration(self):
        transport = FirmwareLoopback()
        aspep = ASPEPClient(transport)
        aspep.connect(timeout=0.1)
        client = MCPClient(aspep)
        client.command(MCP_FAULT_ACK, timeout=0.1)
        client.configure_async_hall()
        request = FrameDecoder().feed(transport.writes[-1])[0].payload
        command, register, raw_size = struct.unpack_from("<HHH", request)
        self.assertEqual(command, MCP_SET)
        self.assertEqual(register, REG_ASYNC_UART_A)
        self.assertEqual(raw_size, 11)
        self.assertEqual(len(request), 6 + raw_size)

    def test_decode_async_hall_samples_and_device_ticks(self):
        payload = (struct.pack("<I", 1000)
                   + struct.pack("<hhhh", 10, 20, -30, -40)
                   + bytes((1, 0)))
        samples = decode_async_hall(payload, hf_divider=30)
        self.assertEqual(
            [(item.device_tick, item.angle, item.hall_speed_raw) for item in samples],
            [(1000, 10, 20), (1030, -30, -40)],
        )


if __name__ == "__main__":
    unittest.main()
