#!/usr/bin/env python3
"""本地实验日志分析器 —— 把固定的解析/计算/诊断在本地做完，只给模型精简结论。

用法: python3 tools/analyze.py [logfile]   (默认 tools/serial.log)

数据行(紧凑格式, 见 references/log-format.md):
  phase,iter,t_ms,pf_cdeg,gy_cdps,cmd_ma,cur_ma,spd_rpm,lat_cdeg,err
注释/事件行以 '#' 开头(CMD/硬危险/辨识档/落点/done/进入平衡 等)。

输出: 每个 phase 段一行摘要(θ/θ̇/飞轮/电流/侧向 的范围与峰值) + 自动诊断标记 + 事件时间线。
"""
import sys, re

LOG = sys.argv[1] if len(sys.argv) > 1 else "tools/serial.log"
DATA = re.compile(r'(?:RX\s+)?([A-Za-z_]+),(\d+),(\d+),(-?\d+),(-?\d+),(-?\d+),(-?\d+),(-?\d+),(-?\d+),(\d+)\s*$')
EVENT_KW = ("CMD", "硬危险", "辨识档", "落点", "done", "进入平衡", "越平衡", "挣脱", "τ_break",
            "未能稳回", "不在静止", "自救", "起跳", "缓冲", "记录", "ABORT", "完成", "未把机体", "未越")

def fnum(x): return int(x)

# 解析
segs = []          # 每段: dict(phase, rows=[(t,pf,gy,cmd,cur,spd,lat,err)])
events = []         # (line_idx, text)
cur = None
for line in open(LOG, errors="replace"):
    s = line.strip()
    m = DATA.search(s)
    if m:
        ph = m.group(1)
        t,it,pf,gy,cmd,curr,spd,lat,err = (fnum(m.group(i)) for i in (3,2,4,5,6,7,8,9,10))
        row = (t, pf/100.0, gy/100.0, cmd, curr, spd, lat/100.0, err)
        if cur is None or cur["phase"] != ph:
            cur = {"phase": ph, "rows": []}; segs.append(cur)
        cur["rows"].append(row)
    elif s.startswith("#") or "RX  #" in s:
        txt = s.split("RX  ", 1)[-1].lstrip("# ").strip()
        if any(k in txt for k in EVENT_KW):
            events.append(txt)
        cur = None  # 注释打断段连续

def rng(vals): return (min(vals), max(vals))
def fmt(v): return f"{v:.1f}"

exp = [s for s in segs if s["phase"] != "MON" and len(s["rows"]) >= 2]
mon = [s for s in segs if s["phase"] == "MON" and s["rows"]]
print(f"=== analyze {LOG} ===  实验段 {len(exp)} 个 (MON监视段 {len(mon)} 个已折叠)")
print("段: phase  n帧 跨ms | θ起→止(min..max) | θ̇峰|末1/3均 | 飞轮末(min..max) | cmd(min..max) | lat峰 | 诊断")
for sg in exp:
    r = sg["rows"]
    ph = sg["phase"]
    t  = [x[0] for x in r]; pf=[x[1] for x in r]; gy=[x[2] for x in r]
    cmd=[x[3] for x in r]; spd=[x[5] for x in r]; lat=[abs(x[6]) for x in r]
    span = t[-1]-t[0]
    tail = r[max(0,len(r)*2//3):]
    gytail = sum(abs(x[2]) for x in tail)/len(tail)
    gypk = max(abs(v) for v in gy)
    pfmin,pfmax = rng(pf); spmin,spmax = rng(spd)
    # 诊断标记
    diag = []
    if pfmax > 40 or pfmin < -40: diag.append("翻越±40")
    if gypk > 200: diag.append(f"起跳猛(θ̇峰{gypk:.0f})")
    if max(abs(spmin),abs(spmax)) > 1500: diag.append(f"飞轮近饱和({max(abs(spmin),abs(spmax)):.0f})")
    # pitch污染: 末段 gy≈0 但 pf 仍大幅变化
    pftail = [x[1] for x in tail]
    if gytail < 12 and (max(pftail)-min(pftail)) > 12:
        diag.append(f"⚠pf污染(gy≈{gytail:.0f}却pfΔ{max(pftail)-min(pftail):.0f})")
    elif gytail < 8:
        diag.append(f"末段θ̇≈0(稳, {gytail:.0f})")
    if max(lat) > 15: diag.append(f"出平面(lat峰{max(lat):.0f})")
    print(f"  {ph:<9} {len(r):>3} {span:>5.0f} | {fmt(pf[0])}→{fmt(pf[-1])}({fmt(pfmin)}..{fmt(pfmax)}) "
          f"| {gypk:.0f}|{gytail:.0f} | {fmt(spd[-1])}({fmt(spmin)}..{fmt(spmax)}) "
          f"| {min(cmd)}..{max(cmd)} | {max(lat):.0f} | {' '.join(diag) if diag else '-'}")

if mon:
    last = mon[-1]["rows"][-1]
    print(f"\n监视末态(机体当前): θ={last[1]:.1f}°  lat={last[6]:.1f}°  (MON,电机断电)")

print("\n--- 事件时间线 ---")
for e in events[-25:]:
    print("  " + e[:90])
