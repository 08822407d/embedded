#!/usr/bin/env bash
set -eu

usage() {
  cat <<'EOF'
Usage:
  bash scripts/enable_m5bus_uart_login_adb.sh [serial]
  BAUD=921600 APPLY=1 bash scripts/enable_m5bus_uart_login_adb.sh [serial]
  MODE=rollback APPLY=1 bash scripts/enable_m5bus_uart_login_adb.sh [serial]

Default mode is dry-run. This script reclaims Module LLM M5-Bus UART
(/dev/ttyS1) from llm-sys and enables serial login on ttyS1 at BAUD 8N1.
BAUD defaults to 921600.
EOF
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
  usage
  exit 0
fi

ADB_BIN=${ADB_BIN:-adb}
MODE=${MODE:-enable}
APPLY=${APPLY:-0}
BAUD=${BAUD:-921600}
SERIAL=${1:-${ADB_SERIAL:-}}

case "$BAUD" in
  9600|19200|38400|57600|115200|230400|460800|500000|576000|921600|1000000|1152000|1500000|2000000|2500000|3000000|3500000|4000000) ;;
  *)
    echo "ERROR: unsupported BAUD value: $BAUD" >&2
    exit 1
    ;;
esac

if ! command -v "$ADB_BIN" >/dev/null 2>&1; then
  echo "ERROR: adb not found. Set ADB_BIN or install Android Platform Tools." >&2
  exit 1
fi

"$ADB_BIN" start-server >/dev/null

if [ -z "$SERIAL" ]; then
  devices=$("$ADB_BIN" devices | awk 'NR > 1 && $2 == "device" { print $1 }')
  count=$(printf '%s\n' "$devices" | sed '/^$/d' | wc -l)
  if [ "$count" -ne 1 ]; then
    echo "ERROR: expected exactly one ADB device; set ADB_SERIAL or pass serial." >&2
    "$ADB_BIN" devices -l >&2
    exit 1
  fi
  SERIAL=$devices
fi

case "$MODE" in
  enable|rollback|status) ;;
  *)
    echo "ERROR: MODE must be enable, rollback, or status." >&2
    exit 1
    ;;
esac

HOST_TS=$(date +%Y%m%d-%H%M%S)

echo "ADB serial: $SERIAL"
echo "MODE=$MODE APPLY=$APPLY BAUD=$BAUD"

"$ADB_BIN" -s "$SERIAL" shell \
  "MODE='$MODE' APPLY='$APPLY' BAUD='$BAUD' HOST_TS='$HOST_TS' sh -s" <<'EOS'
set -eu

backup_dir="/root/m5bus-uart-login-backup-${HOST_TS}"
override_dir="/etc/systemd/system/serial-getty@ttyS1.service.d"
override_file="${override_dir}/override.conf"

snapshot() {
  {
    echo "## timestamp"
    date
    echo
    echo "## cmdline"
    cat /proc/cmdline 2>/dev/null || true
    echo
    echo "## serial driver"
    cat /proc/tty/driver/serial 2>/dev/null || true
    echo
    echo "## systemd"
    systemctl show llm-sys.service serial-getty@ttyS1.service \
      -p LoadState -p ActiveState -p SubState -p UnitFileState -p FragmentPath \
      --no-pager 2>/dev/null || true
    echo
    echo "## ttyS1 line settings"
    stty -F /dev/ttyS1 -a 2>/dev/null || true
    echo
    echo "## ttyS1 users"
    for fd in /proc/[0-9]*/fd/*; do
      target=$(readlink "$fd" 2>/dev/null) || continue
      [ "$target" = "/dev/ttyS1" ] || continue
      pid=${fd#/proc/}; pid=${pid%%/*}
      cmd=$(tr '\000' ' ' < "/proc/$pid/cmdline" 2>/dev/null || true)
      printf 'pid=%s fd=%s target=%s cmd=%s\n' "$pid" "${fd##*/}" "$target" "$cmd"
    done
    echo
    echo "## existing ttyS1 override"
    if [ -f "$override_file" ]; then
      cat "$override_file"
    else
      echo "<none>"
    fi
  }
}

if [ "$MODE" = "status" ]; then
  snapshot
  exit 0
fi

if [ "$APPLY" != "1" ]; then
  echo "DRY-RUN: would save snapshot to $backup_dir"
  if [ "$MODE" = "enable" ]; then
    echo "DRY-RUN: would stop, disable, and mask llm-sys.service"
    echo "DRY-RUN: would write $override_file for $BAUD ttyS1 autologin root"
    echo "DRY-RUN: would enable and restart serial-getty@ttyS1.service"
  else
    echo "DRY-RUN: would disable serial-getty@ttyS1.service"
    echo "DRY-RUN: would unmask, enable, and restore llm-sys.service"
  fi
  snapshot
  exit 0
fi

mkdir -p "$backup_dir"
snapshot > "${backup_dir}/before.txt"

if [ -f "$override_file" ]; then
  cp -a "$override_file" "${backup_dir}/override.conf.before"
fi

if [ "$MODE" = "enable" ]; then
  systemctl stop llm-sys.service
  systemctl disable llm-sys.service
  systemctl mask llm-sys.service

  mkdir -p "$override_dir"
  printf '%s\n' \
    '[Service]' \
    'ExecStart=' \
    "ExecStart=-/sbin/agetty -L --autologin root --noclear $BAUD %I vt102" \
    > "$override_file"

  systemctl daemon-reload
  systemctl enable serial-getty@ttyS1.service
  systemctl restart serial-getty@ttyS1.service
else
  systemctl disable --now serial-getty@ttyS1.service || true
  systemctl unmask llm-sys.service
  systemctl enable llm-sys.service
  systemctl restart llm-sys.service
fi

sleep 1
snapshot > "${backup_dir}/after.txt"
cat "${backup_dir}/after.txt"
echo
echo "Backup: $backup_dir"
exit 0
EOS
