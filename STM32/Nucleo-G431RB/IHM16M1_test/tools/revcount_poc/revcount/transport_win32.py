"""Dependency-free Win32 serial transport and ST-LINK VCP discovery."""

from __future__ import annotations

import ctypes
from ctypes import wintypes
import re
import sys
import winreg


class SerialError(OSError):
    pass


def discover_stlink_port(serial_number: str) -> str:
    """Resolve the VCP COM port by the USB instance serial number."""
    wanted = serial_number.upper()
    records: list[tuple[str, str | None, str | None]] = []
    root_path = r"SYSTEM\CurrentControlSet\Enum\USB"
    access = winreg.KEY_READ
    if hasattr(winreg, "KEY_WOW64_64KEY"):
        access |= winreg.KEY_WOW64_64KEY
    with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, root_path, 0, access) as root:
        device_count = winreg.QueryInfoKey(root)[0]
        for device_index in range(device_count):
            device_name = winreg.EnumKey(root, device_index)
            try:
                with winreg.OpenKey(root, device_name, 0, access) as device:
                    instance_count = winreg.QueryInfoKey(device)[0]
                    for instance_index in range(instance_count):
                        instance_name = winreg.EnumKey(device, instance_index)
                        identity = f"{device_name}\\{instance_name}".upper()
                        container_id: str | None = None
                        port: str | None = None
                        try:
                            with winreg.OpenKey(device, instance_name, 0, access) as instance:
                                try:
                                    value, _ = winreg.QueryValueEx(instance, "ContainerID")
                                    container_id = str(value).upper()
                                except FileNotFoundError:
                                    pass
                            with winreg.OpenKey(
                                device, instance_name + r"\Device Parameters", 0, access
                            ) as parameters:
                                value, _ = winreg.QueryValueEx(parameters, "PortName")
                            if re.fullmatch(r"COM\d+", str(value), re.IGNORECASE):
                                port = str(value).upper()
                        except FileNotFoundError:
                            pass
                        records.append((identity, container_id, port))
            except (FileNotFoundError, PermissionError):
                continue

    # ST-LINK is a composite USB device. Depending on the Windows driver, the
    # serial appears either on the VCP instance itself or only on its parent;
    # ContainerID reliably links those sibling instances.
    target_containers = {
        container_id for identity, container_id, _ in records
        if wanted in identity and container_id is not None
    }
    matches = {
        port for identity, container_id, port in records
        if port is not None and (
            wanted in identity or container_id in target_containers
        )
    }
    if not matches:
        raise SerialError(f"no COM port found for ST-LINK SN {serial_number}")
    if len(matches) != 1:
        raise SerialError(
            f"multiple COM ports found for ST-LINK SN {serial_number}: {sorted(matches)}"
        )
    return next(iter(matches))


class DCB(ctypes.Structure):
    _fields_ = [
        ("DCBlength", wintypes.DWORD),
        ("BaudRate", wintypes.DWORD),
        ("flags", wintypes.DWORD),
        ("wReserved", wintypes.WORD),
        ("XonLim", wintypes.WORD),
        ("XoffLim", wintypes.WORD),
        ("ByteSize", wintypes.BYTE),
        ("Parity", wintypes.BYTE),
        ("StopBits", wintypes.BYTE),
        ("XonChar", ctypes.c_char),
        ("XoffChar", ctypes.c_char),
        ("ErrorChar", ctypes.c_char),
        ("EofChar", ctypes.c_char),
        ("EvtChar", ctypes.c_char),
        ("wReserved1", wintypes.WORD),
    ]


class COMMTIMEOUTS(ctypes.Structure):
    _fields_ = [
        ("ReadIntervalTimeout", wintypes.DWORD),
        ("ReadTotalTimeoutMultiplier", wintypes.DWORD),
        ("ReadTotalTimeoutConstant", wintypes.DWORD),
        ("WriteTotalTimeoutMultiplier", wintypes.DWORD),
        ("WriteTotalTimeoutConstant", wintypes.DWORD),
    ]


class Win32Serial:
    """Synchronous 8N1 serial transport using kernel32 directly."""

    def __init__(self, port: str, baudrate: int = 1_843_200,
                 read_timeout_ms: int = 20) -> None:
        if sys.platform != "win32":
            raise SerialError("Win32Serial is only available on Windows")
        if not re.fullmatch(r"COM\d+", port, re.IGNORECASE):
            raise ValueError(f"invalid COM port: {port}")
        self.port = port.upper()
        self._kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        self._configure_signatures()
        self._handle = self._kernel32.CreateFileW(
            rf"\\.\{self.port}",
            0xC0000000,  # GENERIC_READ | GENERIC_WRITE
            0,
            None,
            3,  # OPEN_EXISTING
            0,
            None,
        )
        if self._handle == wintypes.HANDLE(-1).value:
            self._raise_last_error(f"open {self.port}")
        try:
            self._configure_port(baudrate, read_timeout_ms)
            self.purge()
        except Exception:
            self.close()
            raise

    def _configure_signatures(self) -> None:
        k32 = self._kernel32
        k32.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD,
                                    wintypes.DWORD, wintypes.LPVOID,
                                    wintypes.DWORD, wintypes.DWORD,
                                    wintypes.HANDLE]
        k32.CreateFileW.restype = wintypes.HANDLE
        k32.GetCommState.argtypes = [wintypes.HANDLE, ctypes.POINTER(DCB)]
        k32.GetCommState.restype = wintypes.BOOL
        k32.SetCommState.argtypes = [wintypes.HANDLE, ctypes.POINTER(DCB)]
        k32.SetCommState.restype = wintypes.BOOL
        k32.SetCommTimeouts.argtypes = [wintypes.HANDLE, ctypes.POINTER(COMMTIMEOUTS)]
        k32.SetCommTimeouts.restype = wintypes.BOOL
        k32.SetupComm.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD]
        k32.SetupComm.restype = wintypes.BOOL
        k32.PurgeComm.argtypes = [wintypes.HANDLE, wintypes.DWORD]
        k32.PurgeComm.restype = wintypes.BOOL
        k32.ReadFile.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.DWORD,
                                ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
        k32.ReadFile.restype = wintypes.BOOL
        k32.WriteFile.argtypes = [wintypes.HANDLE, wintypes.LPCVOID, wintypes.DWORD,
                                 ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
        k32.WriteFile.restype = wintypes.BOOL
        k32.CloseHandle.argtypes = [wintypes.HANDLE]
        k32.CloseHandle.restype = wintypes.BOOL

    def _raise_last_error(self, operation: str) -> None:
        code = ctypes.get_last_error()
        raise SerialError(code, f"{operation}: {ctypes.FormatError(code).strip()}")

    def _configure_port(self, baudrate: int, read_timeout_ms: int) -> None:
        if not self._kernel32.SetupComm(self._handle, 8192, 8192):
            self._raise_last_error("SetupComm")
        dcb = DCB()
        dcb.DCBlength = ctypes.sizeof(DCB)
        if not self._kernel32.GetCommState(self._handle, ctypes.byref(dcb)):
            self._raise_last_error("GetCommState")
        dcb.BaudRate = baudrate
        dcb.flags = 0x1011  # binary, DTR enabled, RTS enabled, no flow control
        dcb.ByteSize = 8
        dcb.Parity = 0
        dcb.StopBits = 0
        if not self._kernel32.SetCommState(self._handle, ctypes.byref(dcb)):
            self._raise_last_error("SetCommState")
        timeouts = COMMTIMEOUTS(
            0xFFFFFFFF, 0, read_timeout_ms, 0, 1000
        )
        if not self._kernel32.SetCommTimeouts(self._handle, ctypes.byref(timeouts)):
            self._raise_last_error("SetCommTimeouts")

    def purge(self) -> None:
        if not self._kernel32.PurgeComm(self._handle, 0x000F):
            self._raise_last_error("PurgeComm")

    def read(self, size: int = 4096) -> bytes:
        buffer = ctypes.create_string_buffer(size)
        count = wintypes.DWORD()
        if not self._kernel32.ReadFile(
            self._handle, buffer, size, ctypes.byref(count), None
        ):
            self._raise_last_error("ReadFile")
        return buffer.raw[:count.value]

    def write(self, data: bytes) -> None:
        if not data:
            return
        buffer = ctypes.create_string_buffer(data)
        count = wintypes.DWORD()
        if not self._kernel32.WriteFile(
            self._handle, buffer, len(data), ctypes.byref(count), None
        ):
            self._raise_last_error("WriteFile")
        if count.value != len(data):
            raise SerialError(f"short write: {count.value}/{len(data)} bytes")

    def close(self) -> None:
        handle = getattr(self, "_handle", None)
        if handle not in (None, wintypes.HANDLE(-1).value):
            self._kernel32.CloseHandle(handle)
            self._handle = None

    def __enter__(self) -> "Win32Serial":
        return self

    def __exit__(self, exc_type, exc, traceback) -> None:
        self.close()


def resolve_port(port: str, serial_number: str) -> str:
    if port.lower() == "auto":
        return discover_stlink_port(serial_number)
    return port.upper()
