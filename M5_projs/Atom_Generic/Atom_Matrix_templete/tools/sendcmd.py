#!/usr/bin/env python3
"""向 serial_bridge 的命令 FIFO(tools/serial.cmd) 发一条字符命令。

发送前先把 tools/serial.log 的**当前字节大小**写入 tools/.logoff —— 这样 logtail.py 默认就只读
"本命令回显之后"的新增日志。取代 `OFF=$(wc -c < serial.log); echo x > serial.cmd; sleep N`
这类含命令替换/重定向的复合命令（会触发授权弹窗）。本脚本经 `python3 tools/*`（已 allow）运行。

前提：tools/serial_bridge.py 正在跑（独占串口、读 FIFO）。

用法:
  python3 tools/sendcmd.py s             # 记偏移 → 发 's'
  python3 tools/sendcmd.py 4 --wait 6    # 记偏移 → 发 '4' → 再 sleep 6s（等命令产出）
"""
import argparse
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
LOG = os.path.join(HERE, "serial.log")
FIFO = os.path.join(HERE, "serial.cmd")
OFFFILE = os.path.join(HERE, ".logoff")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd", help="要发的命令字（如 s/4/x/m）")
    ap.add_argument("--wait", type=float, default=0.0, help="发送后 sleep 秒数")
    args = ap.parse_args()

    # 记录发命令前的日志大小，作为"本命令输出"的起始偏移
    try:
        size = os.path.getsize(LOG)
    except OSError:
        size = 0
    with open(OFFFILE, "w") as f:
        f.write(str(size))

    if not os.path.exists(FIFO):
        sys.exit(f"# FIFO 不存在: {FIFO}（serial_bridge 是否在跑?）")

    # 非阻塞写 FIFO：有 reader(bridge) 时成功；无 reader 立即报错而非挂死
    try:
        fd = os.open(FIFO, os.O_WRONLY | os.O_NONBLOCK)
    except OSError as e:
        sys.exit(f"# 打开 FIFO 失败（serial_bridge 没在读? 先启动它）: {e}")
    try:
        os.write(fd, (args.cmd + "\n").encode())
    finally:
        os.close(fd)

    print(f"# sent {args.cmd!r}, log offset={size} → tools/.logoff")
    if args.wait > 0:
        time.sleep(args.wait)


if __name__ == "__main__":
    main()
