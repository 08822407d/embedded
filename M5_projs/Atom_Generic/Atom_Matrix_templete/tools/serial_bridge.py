#!/usr/bin/env python3
"""Serial bridge for development: one process owns the serial port and does
concurrent RX (logged to a file) + TX (commands injected via a FIFO).

This is the reliable way to read sensor output while sending control commands
on the same port: a single owner process, with a reader thread and a command
thread, instead of multiple processes racing for the same bytes.

Usage:
    python3 tools/serial_bridge.py [--port /dev/ttyUSB0] [--baud 115200]

While running:
    - All received lines are appended to tools/serial.log with timestamps.
    - Write a command line into the FIFO tools/serial.cmd to TX it:
          echo "status" > tools/serial.cmd
      The TX is also recorded in the log, marked with '>>>'.
"""
import argparse
import os
import sys
import threading
import time

import serial

HERE = os.path.dirname(os.path.abspath(__file__))
LOG_PATH = os.path.join(HERE, "serial.log")
CMD_FIFO = os.path.join(HERE, "serial.cmd")


def now():
    return time.strftime("%H:%M:%S", time.localtime()) + f".{int(time.time()*1000)%1000:03d}"


def reader_loop(ser, logf, stop):
    """Continuously read lines from serial and append them to the log."""
    while not stop.is_set():
        try:
            raw = ser.readline()
        except serial.SerialException as e:
            logf.write(f"[{now()}] !! serial error: {e}\n")
            logf.flush()
            break
        if not raw:
            continue
        line = raw.decode("utf-8", "replace").rstrip("\r\n")
        logf.write(f"[{now()}] RX  {line}\n")
        logf.flush()


def cmd_loop(ser, logf, stop):
    """Read command lines from the FIFO and write them to the serial port."""
    while not stop.is_set():
        try:
            # Opening a FIFO for reading blocks until a writer appears; reopen
            # each time so multiple `echo > fifo` calls are all picked up.
            with open(CMD_FIFO, "r") as fifo:
                for line in fifo:
                    cmd = line.rstrip("\n")
                    payload = (cmd + "\n").encode("utf-8")
                    ser.write(payload)
                    ser.flush()
                    logf.write(f"[{now()}] >>> TX {cmd!r}\n")
                    logf.flush()
        except OSError as e:
            logf.write(f"[{now()}] !! fifo error: {e}\n")
            logf.flush()
            time.sleep(0.2)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args()

    if not os.path.exists(CMD_FIFO):
        os.mkfifo(CMD_FIFO)

    ser = serial.Serial(args.port, args.baud, timeout=1)
    stop = threading.Event()

    with open(LOG_PATH, "a", buffering=1) as logf:
        logf.write(f"[{now()}] === bridge up on {args.port}@{args.baud} ===\n")
        rt = threading.Thread(target=reader_loop, args=(ser, logf, stop), daemon=True)
        ct = threading.Thread(target=cmd_loop, args=(ser, logf, stop), daemon=True)
        rt.start()
        ct.start()
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            stop.set()
        finally:
            ser.close()


if __name__ == "__main__":
    main()
