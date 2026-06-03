#!/usr/bin/env python3
"""一次性独占串口: 发一个命令并读回若干秒输出, 同时 append 到 serial.log。
用法: python3 tools/send_recover.py [cmd] [secs]   默认 cmd='r' secs=10
"""
import sys, time, os
import serial

HERE = os.path.dirname(os.path.abspath(__file__))
LOG = os.path.join(HERE, "serial.log")
cmd  = sys.argv[1] if len(sys.argv) > 1 else "r"
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 10.0

def now(): return time.strftime("%H:%M:%S")

ser = serial.Serial("/dev/ttyUSB0", 115200, timeout=0.3)
time.sleep(0.3)
ser.reset_input_buffer()
logf = open(LOG, "a", buffering=1)
logf.write(f"[{now()}] === send_recover '{cmd}' for {secs}s ===\n")
ser.write((cmd + "\n").encode()); ser.flush()
logf.write(f"[{now()}] >>> TX {cmd!r}\n")
print(f"TX {cmd!r}")

t0 = time.time()
last = []
while time.time() - t0 < secs:
    raw = ser.readline()
    if not raw:
        continue
    line = raw.decode("utf-8", "replace").rstrip("\r\n")
    logf.write(f"[{now()}] RX  {line}\n")
    # 控制台只回显非纯数据行(事件/注释)与少量采样
    if line.startswith("#") or any(k in line for k in ("急救","CMD","τ","落点","done","完成","菜单","Menu","越","挣脱","未")):
        print("  " + line[:100])
    last.append(line)
ser.close(); logf.close()
# 打印最后状态
data = [l for l in last if "," in l and not l.startswith("#")]
if data:
    print("末行:", data[-1][:80])
print(f"共收 {len(last)} 行")
