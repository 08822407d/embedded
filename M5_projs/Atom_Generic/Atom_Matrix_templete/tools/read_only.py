#!/usr/bin/env python3
"""纯读串口若干秒(不发任何命令), append 到 serial.log, 控制台回显事件行。
用法: python3 tools/read_only.py [secs]   默认 15
"""
import sys, time, os
import serial
HERE = os.path.dirname(os.path.abspath(__file__))
LOG = os.path.join(HERE, "serial.log")
secs = float(sys.argv[1]) if len(sys.argv) > 1 else 15.0
ser = serial.Serial("/dev/ttyUSB0", 115200, timeout=0.3)
logf = open(LOG, "a", buffering=1)
logf.write(f"[{time.strftime('%H:%M:%S')}] === read_only {secs}s ===\n")
t0 = time.time(); last=[]
while time.time()-t0 < secs:
    raw = ser.readline()
    if not raw: continue
    line = raw.decode("utf-8","replace").rstrip("\r\n")
    logf.write(f"[{time.strftime('%H:%M:%S')}] RX  {line}\n")
    if line.startswith("#") or any(k in line for k in ("急救","CMD","τ","落点","done","完成","越","挣脱","未","救","稳")):
        print("  "+line[:100])
    last.append(line)
ser.close(); logf.close()
data=[l for l in last if "," in l and not l.startswith("#")]
if data: print("末行:", data[-1][:80])
print(f"共收 {len(last)} 行")
