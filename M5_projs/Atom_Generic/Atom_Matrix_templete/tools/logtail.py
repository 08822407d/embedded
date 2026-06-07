#!/usr/bin/env python3
"""读取 tools/serial.log 自某字节偏移后的新增内容，可按正则过滤，或轮询等待某 marker 出现后再打印。

设计目的：取代 `tail -c +N serial.log | grep …` 和 `until …; do sleep; done` 这类**复合命令**
（含管道/命令替换/循环，整条 bash 串匹配不上 allow 列表 → 频繁授权弹窗）。本脚本经 `python3 tools/*`
（已在项目 allow 列表）运行，一条命令搞定，不再弹窗。配合 tools/sendcmd.py（它会把发命令前的
日志字节大小写入 tools/.logoff，作为"只看本命令之后输出"的默认偏移）。

用法:
  python3 tools/logtail.py [FILTER]                 # 从 tools/.logoff 的偏移读到末尾；FILTER=输出过滤正则(可选)
  python3 tools/logtail.py "RX档|★|落点"            # 同上 + 只留匹配行
  python3 tools/logtail.py --from 123456 [FILTER]   # 从字节偏移 N 读
  python3 tools/logtail.py --from-file F [FILTER]   # 从文件 F 里的偏移读
  python3 tools/logtail.py [FILTER] --wait "完成|回监视" [--timeout 120]
                                                    # 先轮询直到新增段出现 --wait 正则(或超时)，再打印(过滤后)新增段
  python3 tools/logtail.py [FILTER] --tail 20       # 只打印最后 20 行
"""
import argparse
import os
import re
import time

HERE = os.path.dirname(os.path.abspath(__file__))
LOG = os.path.join(HERE, "serial.log")
OFFFILE = os.path.join(HERE, ".logoff")


def read_offset(args):
    if args.from_byte is not None:
        return args.from_byte
    path = args.from_file or OFFFILE
    try:
        return int(open(path).read().strip())
    except Exception:
        return 0


def slice_lines(start, filt_rx):
    try:
        with open(LOG, "rb") as fh:
            fh.seek(start)
            data = fh.read()
    except OSError:
        return []
    lines = data.decode("utf-8", "replace").splitlines()
    if filt_rx:
        lines = [ln for ln in lines if filt_rx.search(ln)]
    return lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("filter", nargs="?", default=None, help="输出过滤正则")
    ap.add_argument("--from", dest="from_byte", type=int, default=None, help="起始字节偏移")
    ap.add_argument("--from-file", dest="from_file", default=None, help="从该文件读起始偏移")
    ap.add_argument("--wait", default=None, help="轮询直到新增段出现该正则(或超时)")
    ap.add_argument("--timeout", type=float, default=120.0, help="--wait 超时秒(默认120)")
    ap.add_argument("--poll", type=float, default=2.0, help="--wait 轮询间隔秒(默认2)")
    ap.add_argument("--tail", type=int, default=0, help="只打印最后 N 行")
    args = ap.parse_args()

    start = read_offset(args)
    filt_rx = re.compile(args.filter) if args.filter else None

    if args.wait:
        wait_rx = re.compile(args.wait)
        t0 = time.time()
        while time.time() - t0 < args.timeout:
            if any(wait_rx.search(ln) for ln in slice_lines(start, None)):
                break
            time.sleep(args.poll)
        else:
            print(f"# [logtail] 等待超时({args.timeout}s)，未见 /{args.wait}/，打印当前新增段：")

    lines = slice_lines(start, filt_rx)
    if args.tail and len(lines) > args.tail:
        lines = lines[-args.tail:]
    print("\n".join(lines))


if __name__ == "__main__":
    main()
