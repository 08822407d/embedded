#!/usr/bin/env python3
"""
lora_capture.py — 主机侧 LoRa 双机日志抓取脚本
================================================
用法:
    python3 tools/lora_capture.py [选项]

选项:
    --reset         脉冲 RTS 复位两块板(先 PONGER 后 PINGER),对齐 t=0
    --timeout N     整体超时秒数(默认 180)
    --log-dir DIR   日志目录(默认 ./lora_logs)
    --pinger-mac M  PINGER 板的 MAC(SER= 字段),默认 58:8C:81:50:04:38
    --ponger-mac M  PONGER 板的 MAC,默认 58:8C:81:50:06:E4

依赖:
    系统 python3 + pyserial 3.5
    pip install pyserial  (或系统包 python3-serial)

工作流程:
  1. 扫描所有 303A:1001 口,按 SER=MAC 匹配 PINGER / PONGER 端口。
  2. 两口均打开(置 DTR=True),开始读取。
  3. 若 --reset:先复位 PONGER(RTS 脉冲 200ms),再复位 PINGER(200ms)。
  4. 双线程各自带时间戳把日志写到 lora_<ROLE>.txt。
  5. 两端都出现 'TEST COMPLETE' 或超时 N 秒后停止。
  6. 打印汇总:每端 PING/PONG 计数、丢包率、RSSI/SNR 区间。

FACTS 依据:
    F-8  : VID 303A / PID 1001, SER=MAC
    F-13 : 两块 MAC 已知(可通过 --pinger-mac / --ponger-mac 覆盖)

示例(接两块板、复位对齐):
    python3 tools/lora_capture.py --reset

示例(不复位,单纯抓日志,超时 120s):
    python3 tools/lora_capture.py --timeout 120
"""

import argparse
import os
import re
import threading
import time
from dataclasses import dataclass, field
from datetime import datetime
from typing import Optional

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[ERROR] pyserial 未安装。请运行: pip install pyserial")
    raise SystemExit(1)

# ---------------------------------------------------------------------------
# 默认 MAC(FACTS F-13)
# ---------------------------------------------------------------------------
DEFAULT_PINGER_MAC = "58:8C:81:50:04:38"
DEFAULT_PONGER_MAC = "58:8C:81:50:06:E4"
LORA_VID_PID = "303A:1001"  # Espressif 原生 USB-CDC (FACTS F-8)
BAUD = 115200


# ---------------------------------------------------------------------------
# 端口探测
# ---------------------------------------------------------------------------

def normalize_mac(mac: str) -> str:
    """统一转大写、以冒号分隔"""
    return mac.upper().replace("-", ":").replace(".", ":")


def find_port_by_mac(target_mac: str) -> Optional[str]:
    """
    扫描所有串口,找到 hwid 含 VID:PID=303A:1001 且 SER=<target_mac> 的口。
    SER= 字段里的 MAC 格式因平台而异(可能无冒号),做宽松匹配。
    返回设备路径(如 /dev/ttyACM1)或 None。
    """
    target_stripped = normalize_mac(target_mac).replace(":", "")
    for port in serial.tools.list_ports.comports():
        hwid = port.hwid.upper() if port.hwid else ""
        # 必须是 303A:1001
        if "303A:1001" not in hwid:
            continue
        # SER= 字段匹配 MAC(去掉冒号后比较)
        m = re.search(r"SER=([0-9A-F:]+)", hwid)
        if m:
            ser_stripped = m.group(1).replace(":", "")
            if ser_stripped == target_stripped:
                return port.device
    return None


# ---------------------------------------------------------------------------
# 日志解析(用于汇总)
# ---------------------------------------------------------------------------

@dataclass
class Stats:
    role: str
    ping_sent: int = 0       # PINGER: SENT 计数 / PONGER: 收到 PING 计数
    pong_received: int = 0   # PINGER: 收到 PONG 计数 / PONGER: PONG_SENT 计数
    rssi_vals: list = field(default_factory=list)
    snr_vals: list = field(default_factory=list)
    complete: bool = False


def parse_line(line: str, stats: Stats):
    """从单行日志中提取计数/RSSI/SNR"""
    # PINGER SENT: [PINGER] seq=N SENT
    if re.search(r"\[PINGER\].*\bSENT\b", line):
        stats.ping_sent += 1
    # PINGER 收到 PONG: [PINGER] seq=N PONG rssi=X snr=Y rtt=Zms
    m = re.search(r"\[PINGER\].*\bPONG\b.*rssi=([-\d.]+).*snr=([-\d.]+)", line)
    if m:
        stats.pong_received += 1
        stats.rssi_vals.append(float(m.group(1)))
        stats.snr_vals.append(float(m.group(2)))
    # PONGER 收到 PING: [PONGER] seq=N PING rssi=X snr=Y
    m = re.search(r"\[PONGER\].*\bPING\b.*rssi=([-\d.]+).*snr=([-\d.]+)", line)
    if m:
        stats.ping_sent += 1
        stats.rssi_vals.append(float(m.group(1)))
        stats.snr_vals.append(float(m.group(2)))
    # PONGER PONG_SENT
    if re.search(r"\[PONGER\].*\bPONG_SENT\b", line):
        stats.pong_received += 1
    # TEST COMPLETE
    if "TEST COMPLETE" in line:
        stats.complete = True


# ---------------------------------------------------------------------------
# 读取线程
# ---------------------------------------------------------------------------

def reader_thread(role: str, port_path: str, log_path: str,
                  stats: Stats, stop_event: threading.Event):
    """
    持续读取串口行,带时间戳写日志文件,并更新 stats。
    stop_event 被 set 后退出。
    """
    try:
        with serial.Serial(port_path, BAUD, timeout=0.5) as ser:
            ser.dtr = True  # 置 DTR,通知板子主机已连接
            print(f"[{role}] 已打开 {port_path} @ {BAUD}")
            with open(log_path, "w", encoding="utf-8") as f:
                f.write(f"# {role} log — {port_path} — {datetime.now().isoformat()}\n")
                while not stop_event.is_set():
                    try:
                        raw = ser.readline()
                    except serial.SerialException as e:
                        print(f"[{role}] 串口错误: {e}")
                        break
                    if not raw:
                        continue
                    try:
                        line = raw.decode("utf-8", errors="replace").rstrip()
                    except Exception:
                        continue
                    ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                    log_line = f"{ts}  {line}"
                    print(f"[{role}] {log_line}")
                    f.write(log_line + "\n")
                    f.flush()
                    parse_line(line, stats)
    except serial.SerialException as e:
        print(f"[{role}] 无法打开 {port_path}: {e}")


def reset_board(port_path: str, role: str, pulse_ms: int = 200):
    """
    脉冲 RTS 复位板子(RTS 低→高,持续 pulse_ms ms)。
    EN 引脚接 RTS 低有效(Espressif 通用 auto-download 电路)。
    """
    try:
        with serial.Serial(port_path, BAUD, timeout=1) as ser:
            ser.dtr = True
            ser.rts = True    # 拉低 EN
            time.sleep(pulse_ms / 1000.0)
            ser.rts = False   # 释放 EN → 板子复位
            print(f"[{role}] RTS 复位 {port_path} (脉冲 {pulse_ms}ms)")
    except serial.SerialException as e:
        print(f"[{role}] 复位失败 {port_path}: {e}")


# ---------------------------------------------------------------------------
# 汇总打印
# ---------------------------------------------------------------------------

def print_summary(pinger_stats: Stats, ponger_stats: Stats):
    print()
    print("=" * 50)
    print("  CAPTURE SUMMARY")
    print("=" * 50)
    for stats in (pinger_stats, ponger_stats):
        role = stats.role
        sent = stats.ping_sent
        recv = stats.pong_received
        loss = (100.0 * (sent - recv) / sent) if sent > 0 else float("nan")
        print(f"  [{role}]")
        print(f"    sent/received : {sent} / {recv}")
        if sent > 0:
            print(f"    loss          : {loss:.1f}%")
        if stats.rssi_vals:
            print(f"    RSSI range    : [{min(stats.rssi_vals):.1f}, {max(stats.rssi_vals):.1f}] dBm")
            print(f"    SNR  range    : [{min(stats.snr_vals):.1f}, {max(stats.snr_vals):.1f}] dB")
        print(f"    complete      : {'YES' if stats.complete else 'NO (timeout?)'}")
    print("=" * 50)


# ---------------------------------------------------------------------------
# 主程序
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="LoRa 双机日志抓取工具",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--reset", action="store_true",
                        help="脉冲 RTS 复位两块板对齐 t=0(先 PONGER 后 PINGER)")
    parser.add_argument("--timeout", type=int, default=180,
                        help="整体超时秒数(默认 180)")
    parser.add_argument("--log-dir", default="lora_logs",
                        help="日志目录(默认 ./lora_logs)")
    parser.add_argument("--pinger-mac", default=DEFAULT_PINGER_MAC,
                        help=f"PINGER 板 MAC(默认 {DEFAULT_PINGER_MAC})")
    parser.add_argument("--ponger-mac", default=DEFAULT_PONGER_MAC,
                        help=f"PONGER 板 MAC(默认 {DEFAULT_PONGER_MAC})")
    args = parser.parse_args()

    os.makedirs(args.log_dir, exist_ok=True)

    # ── 探测端口 ──────────────────────────────────────────────
    print(f"[INFO] 扫描 {LORA_VID_PID} 设备...")
    pinger_port = find_port_by_mac(args.pinger_mac)
    ponger_port = find_port_by_mac(args.ponger_mac)

    if not pinger_port:
        print(f"[ERROR] 未找到 PINGER (MAC={args.pinger_mac}) 对应端口。")
        print("[HINT] 检查 'pio device list' 或手动用 --pinger-mac 覆盖。")
        raise SystemExit(1)
    if not ponger_port:
        print(f"[ERROR] 未找到 PONGER (MAC={args.ponger_mac}) 对应端口。")
        print("[HINT] 检查 'pio device list' 或手动用 --ponger-mac 覆盖。")
        raise SystemExit(1)

    print(f"[INFO] PINGER → {pinger_port} (MAC={args.pinger_mac})")
    print(f"[INFO] PONGER → {ponger_port} (MAC={args.ponger_mac})")

    # ── 日志文件路径 ──────────────────────────────────────────
    ts_tag = datetime.now().strftime("%Y%m%d_%H%M%S")
    pinger_log = os.path.join(args.log_dir, f"pinger_{ts_tag}.txt")
    ponger_log = os.path.join(args.log_dir, f"ponger_{ts_tag}.txt")

    # ── 统计对象 ──────────────────────────────────────────────
    pinger_stats = Stats(role="PINGER")
    ponger_stats = Stats(role="PONGER")
    stop_event = threading.Event()

    # ── 启动读取线程 ──────────────────────────────────────────
    t_pinger = threading.Thread(
        target=reader_thread,
        args=("PINGER", pinger_port, pinger_log, pinger_stats, stop_event),
        daemon=True,
    )
    t_ponger = threading.Thread(
        target=reader_thread,
        args=("PONGER", ponger_port, ponger_log, ponger_stats, stop_event),
        daemon=True,
    )
    t_pinger.start()
    t_ponger.start()
    time.sleep(0.3)  # 等端口稳定打开

    # ── 可选:复位两块板对齐 t=0 ─────────────────────────────
    if args.reset:
        print("[INFO] 复位 PONGER...")
        reset_board(ponger_port, "PONGER")
        time.sleep(0.1)
        print("[INFO] 复位 PINGER...")
        reset_board(pinger_port, "PINGER")
        print("[INFO] 复位完成,开始计时。")

    # ── 等待结束条件 ──────────────────────────────────────────
    deadline = time.time() + args.timeout
    print(f"[INFO] 等待测试完成(超时 {args.timeout}s)...")
    while time.time() < deadline:
        if pinger_stats.complete and ponger_stats.complete:
            print("[INFO] 两端均已 TEST COMPLETE,提前退出。")
            break
        time.sleep(0.5)
    else:
        print(f"[WARN] 超时 {args.timeout}s 到达,强制停止。")

    stop_event.set()
    t_pinger.join(timeout=3)
    t_ponger.join(timeout=3)

    # ── 汇总 ──────────────────────────────────────────────────
    print_summary(pinger_stats, ponger_stats)
    print(f"[INFO] 日志已保存: {pinger_log}")
    print(f"[INFO]             {ponger_log}")


if __name__ == "__main__":
    main()
