#!/usr/bin/env python3
# 一次性抓取：复位 ESP32(经 DTR/RTS) 后从 boot 抓串口，直到测试结束或超时。
# 用法: python3 tools/capture_run.py [输出文件] [超时秒]
import sys, time, serial

PORT = "/dev/ttyUSB0"
BAUD = 115200
out  = sys.argv[1] if len(sys.argv) > 1 else "tools/run.log"
tmo  = float(sys.argv[2]) if len(sys.argv) > 2 else 35.0

# 结束标志（任一出现即提前停止）。
# 注意：006 固定流程**结尾永远落到持续监视(attitudeReportTick)**、且 ABORT 后还会自救续跑，
#   故这些都不再是"终态"。默认置空＝抓满整个时长；如需早停可临时填标志。
DONE = ()

ser = serial.Serial(PORT, BAUD, timeout=0.2)
# 经典复位进 run 模式：EN 拉低再释放，GPIO0 保持高
ser.setDTR(False); ser.setRTS(True);  time.sleep(0.12)
ser.setDTR(False); ser.setRTS(False)
ser.reset_input_buffer()

t0 = time.time()
with open(out, "w") as f:
    while time.time() - t0 < tmo:
        line = ser.readline().decode("utf-8", "replace").rstrip("\r\n")
        if not line:
            continue
        ts = f"{time.time()-t0:6.2f}  {line}"
        print(ts, flush=True)
        f.write(line + "\n"); f.flush()
        if any(k in line for k in DONE):
            # 再多抓 2s 把 idle/收尾抓全
            tend = time.time() + 2.0
            while time.time() < tend:
                l2 = ser.readline().decode("utf-8", "replace").rstrip("\r\n")
                if l2:
                    print(f"{time.time()-t0:6.2f}  {l2}", flush=True)
                    f.write(l2 + "\n"); f.flush()
            break
ser.close()
print(f"\n[capture] 写入 {out}")
