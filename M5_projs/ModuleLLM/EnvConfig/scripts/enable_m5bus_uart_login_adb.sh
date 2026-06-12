#!/usr/bin/env bash
set -eu

usage() {
  cat <<'EOF'
Usage:
  bash scripts/enable_m5bus_uart_login_adb.sh [serial]
  BAUD=921600 APPLY=1 bash scripts/enable_m5bus_uart_login_adb.sh [serial]
  TERM_TYPE=xterm-256color ROWS=32 COLS=64 APPLY=1 \
    bash scripts/enable_m5bus_uart_login_adb.sh [serial]
  MODE=rollback APPLY=1 bash scripts/enable_m5bus_uart_login_adb.sh [serial]

Default mode is dry-run. This script reclaims Module LLM M5-Bus UART
(/dev/ttyS1) from llm-sys and enables serial login on ttyS1 at BAUD 8N1.
Defaults: BAUD=921600, TERM_TYPE=xterm-256color, COLORTERM_VALUE=truecolor,
ROWS=32, COLS=64.
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
TERM_TYPE=${TERM_TYPE:-xterm-256color}
COLORTERM_VALUE=${COLORTERM_VALUE:-truecolor}
ROWS=${ROWS:-32}
COLS=${COLS:-64}
SERIAL=${1:-${ADB_SERIAL:-}}

case "$BAUD" in
  9600|19200|38400|57600|115200|230400|460800|500000|576000|921600|1000000|1152000|1500000|2000000|2500000|3000000|3500000|4000000) ;;
  *)
    echo "ERROR: unsupported BAUD value: $BAUD" >&2
    exit 1
    ;;
esac

case "$TERM_TYPE" in
  ""|*[!A-Za-z0-9._+-]*)
    echo "ERROR: invalid TERM_TYPE value: $TERM_TYPE" >&2
    exit 1
    ;;
esac

case "$COLORTERM_VALUE" in
  ""|*[!A-Za-z0-9._+-]*)
    echo "ERROR: invalid COLORTERM_VALUE value: $COLORTERM_VALUE" >&2
    exit 1
    ;;
esac

validate_positive_integer() {
  value_name=$1
  value=$2
  case "$value" in
    ""|*[!0-9]*)
      echo "ERROR: $value_name must be a positive integer." >&2
      exit 1
      ;;
  esac
  if [ "$value" -le 0 ]; then
    echo "ERROR: $value_name must be greater than zero." >&2
    exit 1
  fi
}

validate_positive_integer ROWS "$ROWS"
validate_positive_integer COLS "$COLS"

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
echo "MODE=$MODE APPLY=$APPLY BAUD=$BAUD TERM_TYPE=$TERM_TYPE"
echo "COLORTERM_VALUE=$COLORTERM_VALUE ROWS=$ROWS COLS=$COLS"

"$ADB_BIN" -s "$SERIAL" shell \
  "MODE='$MODE' APPLY='$APPLY' BAUD='$BAUD' TERM_TYPE='$TERM_TYPE' COLORTERM_VALUE='$COLORTERM_VALUE' ROWS='$ROWS' COLS='$COLS' HOST_TS='$HOST_TS' sh -s" <<'EOS'
set -eu

backup_dir="/root/m5bus-uart-login-backup-${HOST_TS}"
override_dir="/etc/systemd/system/serial-getty@ttyS1.service.d"
override_file="${override_dir}/override.conf"
profile_file="/etc/profile.d/m5bus-ttyS1-tab5.sh"

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
    echo
    echo "## ttyS1 Tab5 profile"
    if [ -f "$profile_file" ]; then
      cat "$profile_file"
    else
      echo "<none>"
    fi
    echo
    echo "## terminal capabilities"
    infocmp "$TERM_TYPE" >/dev/null 2>&1 \
      && echo "$TERM_TYPE terminfo: PRESENT" \
      || echo "$TERM_TYPE terminfo: MISSING"
    printf 'tput colors='
    TERM="$TERM_TYPE" tput colors 2>/dev/null || true
    command -v htop 2>/dev/null || true
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
    echo "DRY-RUN: would require $TERM_TYPE terminfo"
    echo "DRY-RUN: would write $override_file for $BAUD ttyS1 autologin root using $TERM_TYPE"
    echo "DRY-RUN: would write $profile_file for TERM/COLORTERM and ${ROWS}x${COLS}, conditional on /dev/ttyS1"
    echo "DRY-RUN: would enable and restart serial-getty@ttyS1.service"
  else
    echo "DRY-RUN: would disable serial-getty@ttyS1.service"
    echo "DRY-RUN: would remove $profile_file"
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
if [ -f "$profile_file" ]; then
  cp -a "$profile_file" "${backup_dir}/m5bus-ttyS1-tab5.sh.before"
else
  : > "${backup_dir}/m5bus-ttyS1-tab5.sh.absent"
fi

if [ "$MODE" = "enable" ]; then
  if ! infocmp "$TERM_TYPE" >/dev/null 2>&1; then
    echo "ERROR: terminfo entry is missing: $TERM_TYPE" >&2
    exit 1
  fi

  systemctl stop llm-sys.service
  systemctl disable llm-sys.service
  systemctl mask llm-sys.service

  mkdir -p "$override_dir"
  printf '%s\n' \
    '[Service]' \
    'ExecStart=' \
    "ExecStart=-/sbin/agetty -L --autologin root --noclear $BAUD %I $TERM_TYPE" \
    > "$override_file"

  cat > "$profile_file" <<EOF
# Managed by enable_m5bus_uart_login_adb.sh.
# Apply Tab5 terminal settings only to login shells attached to /dev/ttyS1.
if _m5bus_tty=\$(tty 2>/dev/null) && [ "\$_m5bus_tty" = /dev/ttyS1 ]; then
  export TERM=$TERM_TYPE
  export COLORTERM=$COLORTERM_VALUE
  stty rows $ROWS cols $COLS 2>/dev/null || true
fi
unset _m5bus_tty
EOF
  chmod 0644 "$profile_file"

  systemctl daemon-reload
  systemctl enable serial-getty@ttyS1.service
  systemctl restart serial-getty@ttyS1.service
else
  systemctl disable --now serial-getty@ttyS1.service || true
  rm -f "$profile_file"
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
