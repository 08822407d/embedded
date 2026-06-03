#!/usr/bin/env python3
"""起跳能量对比分析器 —— 比较"突然加速"与"蓄能急停"两种起跳给机体的角动量/能量。

用法: python3 tools/compare_kick.py [logfile]   (默认 tools/serial.log)

物理(反作用轮，动量守恒，忽略起跳瞬间重力/摩擦)：
  机体获得角动量冲量 J = ∫τ dt = I_w·|Δω_wheel|  (飞轮转速变化越大 → 机体冲量越大)
  机体 Δθ̇(gy) = J / I_b = (I_w/I_b)·|Δω_wheel|     (机体实测角速度变化)
  反作用力矩      τ = I_w·dω_wheel/dt = Kt·I        (恒流 → τ恒定；机械急停 dt→0 → τ瞬时极大)
  飞轮动能        KE_w ∝ ω_wheel²                    (蓄能转速越高储能越大)
本脚本从 spd(飞轮rpm)与 gy(机体°/s)曲线，量化每个起跳尝试里：
  蓄能段ω积累 / 起跳段把飞轮ω从蓄能值"扫过0再反向"的 急停子段 vs 反向加速子段，
  各自的 Δω、平均|dω/dt|(∝力矩)、机体gy获得量，并对比。
"""
import sys, re

LOG = sys.argv[1] if len(sys.argv) > 1 else "tools/serial.log"
DATA = re.compile(r'(?:\[[^\]]*\]\s*)?(?:RX\s+)?([A-Za-z_]+),(\d+),(\d+),(-?\d+),(-?\d+),(-?\d+),(-?\d+),(-?\d+),(-?\d+),(\d+)\s*$')

# 解析为帧序列: (phase, t_ms, gy_dps, cmd_ma, spd_rpm)
frames = []
last_cmd_r_idx = None
for line in open(LOG, errors="replace"):
    s = line.rstrip("\n")
    if "CMD 'r'" in s or "CMD \"r\"" in s or "CMD 'r'" in s:
        last_cmd_r_idx = len(frames)
    m = DATA.search(s)
    if not m:
        continue
    ph = m.group(1)
    t   = int(m.group(3))
    gy  = int(m.group(5)) / 100.0
    cmd = int(m.group(6))
    spd = int(m.group(8))
    frames.append((ph, t, gy, cmd, spd))

# 只取最后一次 r 之后的 RC_ 帧
start = last_cmd_r_idx if last_cmd_r_idx is not None else 0
rc = [f for f in frames[start:] if f[0].startswith("RC_")]
if not rc:
    print("没找到 RC_ 起跳数据 (最后一次 'CMD r' 之后)。"); sys.exit(0)

# 按 SPIN→KICK→SET 切成"尝试"。一个尝试 = 连续的 RC_SPIN... 接 RC_KICK... 接 RC_SET...
attempts = []
cur = None
for f in rc:
    ph = f[0]
    if ph == "RC_SPIN" and (cur is None or cur.get("kick")):
        cur = {"spin": [], "kick": [], "set": []}; attempts.append(cur)
    if cur is None:
        cur = {"spin": [], "kick": [], "set": []}; attempts.append(cur)
    if ph == "RC_SPIN": cur["spin"].append(f)
    elif ph == "RC_KICK": cur["kick"].append(f)
    elif ph == "RC_SET": cur["set"].append(f)

def seg_stats(seg):
    """返回 (n, dt_ms, spd0, spd_end, spd_absmax, gy_absmax, dgy)"""
    if len(seg) < 2: return None
    t0, t1 = seg[0][1], seg[-1][1]
    spd = [f[4] for f in seg]; gy = [f[2] for f in seg]
    return dict(n=len(seg), dt=t1-t0, s0=spd[0], s1=spd[-1],
                smax=max(abs(min(spd)), abs(max(spd))),
                gymax=max(abs(g) for g in gy), gy0=gy[0], gy1=gy[-1])

def windowed_rate(seg, win_ms=60):
    """把一段按 ~win_ms 切窗，返回每窗 (t_mid, dωdt[rpm/s], dθ̇dt[°/s²]机体角加速度)。
    用于判定全程力矩是否恒定（=纯恒流，无急停冲击）。"""
    out = []
    i = 0
    while i < len(seg) - 1:
        t0 = seg[i][1]; j = i
        while j < len(seg) - 1 and seg[j][1] - t0 < win_ms: j += 1
        dt = (seg[j][1] - seg[i][1]) / 1000.0
        if dt <= 0: break
        dw = (seg[j][4] - seg[i][4]) / dt
        dg = (seg[j][2] - seg[i][2]) / dt
        out.append(((seg[i][1]+seg[j][1])//2, dw, dg))
        i = j
    return out

print(f"=== 起跳能量对比 {LOG} ===  最后一次 r：{len(attempts)} 个尝试")
print("J(机体角动量冲量) ∝ 飞轮|Δω|；力矩 ∝ |dω/dt|(rpm/s)；机体响应看 gy(°/s)\n")

for ai, att in enumerate(attempts, 1):
    sp = seg_stats(att["spin"]); kk = att["kick"]
    print(f"--- 尝试{ai} ---")
    if sp:
        spin_rate = (sp["smax"]) / (sp["dt"]/1000.0) if sp["dt"] else 0
        print(f"  蓄能RC_SPIN: {sp['dt']}ms 飞轮0→|{sp['smax']}|rpm  "
              f"(蓄能力矩~{spin_rate:.0f} rpm/s, 小电流140mA)  机体gy峰={sp['gymax']:.0f}°/s")
    if len(kk) >= 3:
        ks = seg_stats(kk)
        # 机体gy峰值出现时刻(相对KICK起点)
        t0k = kk[0][1]
        ipk = max(range(len(kk)), key=lambda i: abs(kk[i][2]))
        print(f"  起跳RC_KICK(恒流1200mA): {ks['dt']}ms 飞轮{ks['s0']}→{ks['s1']}rpm "
              f"(|峰|{ks['smax']})  机体gy峰={ks['gymax']:.0f}°/s @ {kk[ipk][1]-t0k}ms")
        # 起跳未饱和段(前~300ms)逐窗 dω/dt: 看力矩是否恒定(=纯恒流无急停冲击)。
        # 排除饱和尾段(飞轮接近转速上限后 dω/dt 自然降，非真尖峰)。
        wr = [w for w in windowed_rate(kk, 60) if (w[0]-t0k) <= 300]
        rates = [abs(w[1]) for w in wr]
        if rates:
            rmax, rmin = max(rates), min(rates)
            verdict = "恒流匀加速(无急停冲击)" if rmax/max(rmin,1) < 2.5 else "存在力矩尖峰(疑似急停)"
            seq = " ".join(f"{r:.0f}" for r in rates)
            print(f"     起跳前300ms 飞轮|dω/dt|各60ms窗: [{seq}] rpm/s (比值{rmax/max(rmin,1):.1f}) → {verdict}")
        tot_dw = abs(ks['s0'] - ks['s1'])
        if tot_dw:
            print(f"     飞轮总|Δω|={tot_dw}rpm → 机体冲量 ∝ 此值；机体起跳冲击峰{abs(kk[ipk][2]):.0f}°/s 后被死点/重力挡回")
    print()

# 汇总对比：突然加速(无蓄能等效=只用加速子段) vs 蓄能急停(蓄能Δω + 急停子段)
print("=== 对比小结(机体冲量 ∝ 飞轮可用|Δω|) ===")
for ai, att in enumerate(attempts, 1):
    sp = seg_stats(att["spin"]); kk = att["kick"]
    if not (sp and len(kk) >= 3): continue
    ks = seg_stats(kk)
    store = sp["smax"]                       # 蓄能转速
    peak  = ks["smax"]                       # 起跳后反向峰值
    direct_equiv = peak - store              # 若不蓄能、单纯从0加速到峰值的Δω(突然加速可达)
    spinkick_dw  = store + peak              # 蓄能+恒流: 从-store扫到+peak的总Δω
    print(f" 尝试{ai}: 蓄能{store}rpm + 反向峰{peak}rpm")
    print(f"   · 突然加速(0→{peak}): 飞轮Δω≈{peak}rpm")
    print(f"   · 蓄能急停(−{store}→+{peak}): 飞轮Δω≈{spinkick_dw}rpm  "
          f"→ 比突然加速多 ~{(spinkick_dw/peak-1)*100:.0f}% 冲量" if peak else "")
