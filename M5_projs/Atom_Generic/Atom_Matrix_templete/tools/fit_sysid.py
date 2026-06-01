#!/usr/bin/env python3
# 离线系统辨识：从 serial.log 的 SID_DRV(驱动段,开环已知电流)拟合
#   θ̈ = a·sinθ + b·I_cmd   （a=mgl/I_b [°/s²], b=Kt当量/I_b [°/s²/mA]）
# 用真实时间戳算 θ̈(中心差分)。纯 Python 最小二乘(2参数)，无需 numpy。
import sys, math, re

rows = []   # (t_s, theta_deg, thetadot_dps, I_cmd_ma), 分档(cmd变化处断开)
groups = {}
for line in open("tools/serial.log", errors="replace"):
    m = re.search(r'SID_DRV,(\d+),(\d+),(-?\d+),(-?\d+),(-?\d+),', line)
    if not m: continue
    it, t, pf, gy, cmd = (int(x) for x in m.groups())
    groups.setdefault(cmd, []).append((t/1000.0, pf/100.0, gy/100.0, cmd))

X = []  # [sinθ, I]
Y = []  # θ̈
print("档(I_cmd)  样本  θ范围        θ̇范围")
for cmd, g in sorted(groups.items()):
    g.sort(key=lambda r: r[0])
    # 中心差分求 θ̈ (用 gy=θ̇ 的时间导数)
    n = 0
    for i in range(1, len(g)-1):
        dt = g[i+1][0] - g[i-1][0]
        if dt <= 0: continue
        thddot = (g[i+1][2] - g[i-1][2]) / dt   # θ̈ = dθ̇/dt
        th = g[i][1]
        X.append([math.sin(math.radians(th)), float(cmd)])
        Y.append(thddot)
        n += 1
    ths = [r[1] for r in g]; tds = [r[2] for r in g]
    print(f"  {cmd:>5}    {len(g):>3}   {min(ths):.1f}~{max(ths):.1f}°   {min(tds):.0f}~{max(tds):.0f}°/s  (用{n}点)")

# 最小二乘 (X'X) c = X'Y，2x2 解析解
n = len(X)
Sxx = sum(x[0]*x[0] for x in X); Sxz = sum(x[0]*x[1] for x in X); Szz = sum(x[1]*x[1] for x in X)
Sxy = sum(X[i][0]*Y[i] for i in range(n)); Szy = sum(X[i][1]*Y[i] for i in range(n))
det = Sxx*Szz - Sxz*Sxz
a = ( Szz*Sxy - Sxz*Szy) / det
b = (-Sxz*Sxy + Sxx*Szy) / det
# 残差
yhat = [a*X[i][0] + b*X[i][1] for i in range(n)]
ss_res = sum((Y[i]-yhat[i])**2 for i in range(n))
ymean = sum(Y)/n; ss_tot = sum((y-ymean)**2 for y in Y)
r2 = 1 - ss_res/ss_tot if ss_tot else 0

print(f"\n拟合 θ̈ = a·sinθ + b·I_cmd   (n={n}, R²={r2:.3f})")
print(f"  a = mgl/I_b      = {a:.1f}  °/s²        (重力回复项)")
print(f"  b = Kt当量/I_b   = {b:.3f}  °/s² per mA (控制力矩项)")
print(f"\n推论(控制设计用比值):")
# 平衡点 θ=0 线性化: θ̈ ≈ a·θ(rad) + b·I.  θ用度时 a_deg = a (since sinθ≈θ_rad, but our a is per sinθ).
# 阱/倒立刚度: 在 θ=0, d(θ̈)/dθ = a·cos0 = a (per rad of sinθ). 不稳定(a>0→倒立正反馈).
print(f"  · 平衡点开环极点 ω≈sqrt(a·π/180)= {math.sqrt(abs(a)*math.pi/180):.2f} rad/s (a>0=倒立不稳定，需反馈)")
print(f"  · 临界阻尼估计 K_DAMP ≈ 2·ζ·ω/|b| ... 见报告(用 a,b 设计 PD 增益)")
print(f"  · 要 θ 减小1°需电流 ≈ {abs(a*math.sin(math.radians(1))/b):.0f} mA 抵消该点重力(静态)")
