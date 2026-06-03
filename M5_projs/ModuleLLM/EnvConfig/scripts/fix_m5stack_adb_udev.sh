#!/usr/bin/env bash
set -eu

RULE_FILE="/etc/udev/rules.d/51-m5stack-axera-adb.rules"
RULE='SUBSYSTEM=="usb", ATTR{idVendor}=="32c9", ATTR{idProduct}=="2003", MODE="0660", GROUP="plugdev", TAG+="uaccess"'

find_m5stack_usb_nodes() {
  for node in /dev/bus/usb/*/*; do
    [ -e "$node" ] || continue
    if udevadm info -q property -n "$node" 2>/dev/null \
      | grep -Eq '^(ID_VENDOR_ID=32c9|PRODUCT=32c9/2003/)'; then
      printf '%s\n' "$node"
    fi
  done
}

echo "## M5Stack / AXERA ADB udev fix"
echo "Rule file: $RULE_FILE"
echo "Rule: $RULE"
echo

echo "## Current matching USB nodes"
find_m5stack_usb_nodes || true
echo

if [ "${APPLY:-0}" != "1" ]; then
  echo "DRY RUN: no changes made."
  echo "To apply on the Host, run: APPLY=1 bash scripts/fix_m5stack_adb_udev.sh"
  exit 0
fi

if [ "$(id -u)" -ne 0 ]; then
  exec sudo env APPLY=1 bash "$0" "$@"
fi

if ! getent group plugdev >/dev/null; then
  echo "ERROR: plugdev group does not exist on this Host." >&2
  exit 1
fi

tmp="$(mktemp)"
{
  echo '# M5Stack Module LLM / AXERA AX620E ADB'
  echo "$RULE"
} > "$tmp"

install -o root -g root -m 0644 "$tmp" "$RULE_FILE"
rm -f "$tmp"

udevadm control --reload-rules

nodes="$(find_m5stack_usb_nodes || true)"
if [ -n "$nodes" ]; then
  printf '%s\n' "$nodes" | while IFS= read -r node; do
    [ -n "$node" ] || continue
    udevadm trigger --action=change "$node" 2>/dev/null || true
    chgrp plugdev "$node"
    chmod 0660 "$node"
    stat -c '%A %U %G %a %n' "$node"
  done
else
  echo "No current 32c9:2003 USB node found. Replug the device, then run adb devices -l."
fi

echo
echo "Done. Restart adb server as the normal user, then run: adb devices -l"
