#!/usr/bin/env bash
# 重建 RNode_Firmware/ 工作区:克隆上游固定 commit + 打 C6L 移植补丁。
#
# 设计:RNode_Firmware/(整份上游 clone)不入库(见项目 .gitignore),避免把第三方
# 代码塞进 monorepo;入库的只有 patches/rnode_c6l.patch + 本脚本 + docs/RNODE_C6L_PORT.md。
# 上游若大改、想移植到新版而补丁打不上:见 docs/RNODE_C6L_PORT.md「重建与再移植」B 节。
set -euo pipefail

HERE="$(cd "$(dirname "$0")/.." && pwd)"     # 项目根
UPSTREAM="https://github.com/markqvist/RNode_Firmware.git"
PIN="d39339f8ecd5145b248c18bac7b6ea0f82faf85a"   # 本补丁对应的上游 commit
DEST="$HERE/RNode_Firmware"
PATCH="$HERE/patches/rnode_c6l.patch"

if [ -e "$DEST" ]; then
  echo "已存在 $DEST —— 先移走或删掉再跑本脚本。" >&2
  exit 1
fi
[ -f "$PATCH" ] || { echo "找不到补丁 $PATCH" >&2; exit 1; }

git clone "$UPSTREAM" "$DEST"
cd "$DEST"
git checkout "$PIN"
git apply --whitespace=nowarn "$PATCH"

echo
echo "OK:已克隆 @${PIN:0:8} 并打上 C6L 补丁。"
echo "下一步编译: cd RNode_Firmware && pio run -e m5stack-unitc6l-rnode"
echo "(UART 环境: pio run -e m5stack-unitc6l-rnode-uart)"
