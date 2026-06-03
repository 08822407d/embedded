#!/usr/bin/env bash
set -eu

resolve_adb_bin() {
  if [ -n "${ADB_BIN:-}" ]; then
    if command -v "$ADB_BIN" >/dev/null 2>&1; then
      command -v "$ADB_BIN"
      return 0
    fi
    if [ -x "$ADB_BIN" ]; then
      printf '%s\n' "$ADB_BIN"
      return 0
    fi
  fi

  if command -v adb >/dev/null 2>&1; then
    command -v adb
    return 0
  fi

  for p in \
    "${ANDROID_HOME:-}/platform-tools/adb" \
    "${ANDROID_SDK_ROOT:-}/platform-tools/adb" \
    "$HOME/Android/Sdk/platform-tools/adb" \
    "$HOME/.local/share/android-sdk/platform-tools/adb" \
    "/opt/android-sdk/platform-tools/adb"; do
    if [ -n "$p" ] && [ -x "$p" ]; then
      printf '%s\n' "$p"
      return 0
    fi
  done

  return 1
}

choose_adb_serial() {
  adb_bin="${ADB_BIN:-}"
  if [ -z "$adb_bin" ]; then
    adb_bin="$(resolve_adb_bin || true)"
  fi
  if ! command -v "$adb_bin" >/dev/null 2>&1; then
    echo "ERROR: adb not found. Set ADB_BIN or install Android SDK Platform Tools." >&2
    return 2
  fi

  "$adb_bin" version >/dev/null
  "$adb_bin" start-server >/dev/null
  "$adb_bin" devices -l >&2

  serial="${1:-${ADB_SERIAL:-${ANDROID_SERIAL:-}}}"
  devices="$("$adb_bin" devices | awk 'NR>1 && $2=="device" {print $1}')"
  count="$(printf '%s\n' "$devices" | sed '/^$/d' | wc -l | tr -d ' ')"

  if [ "$count" = "0" ]; then
    echo "ERROR: no adb device in 'device' state." >&2
    return 3
  fi

  if [ -n "$serial" ]; then
    if printf '%s\n' "$devices" | grep -Fx "$serial" >/dev/null 2>&1; then
      printf '%s\n' "$serial"
      return 0
    fi
    echo "ERROR: requested serial '$serial' is not in current adb device list." >&2
    printf '%s\n' "$devices" >&2
    return 4
  fi

  if [ "$count" = "1" ]; then
    printf '%s\n' "$devices"
    return 0
  fi

  echo "ERROR: multiple adb devices found. Set ADB_SERIAL=<serial> or pass serial as first argument." >&2
  printf '%s\n' "$devices" >&2
  return 5
}

if ADB_RESOLVED="$(resolve_adb_bin)"; then
  ADB_BIN="$ADB_RESOLVED"
else
  echo "ERROR: adb not found. Set ADB_BIN or install Android SDK Platform Tools." >&2
  exit 2
fi

serial="$(choose_adb_serial "${1:-}")"
interval="${INTERVAL:-5}"
count="${COUNT:-0}"
freq_index="${FREQ_INDEX:-2}"

case "$interval" in
  ''|*[!0-9]*)
    echo "ERROR: INTERVAL must be a positive integer number of seconds." >&2
    exit 6
    ;;
esac

case "$count" in
  ''|*[!0-9]*)
    echo "ERROR: COUNT must be a non-negative integer. COUNT=0 means forever." >&2
    exit 7
    ;;
esac

case "$freq_index" in
  0|1|2|3) ;;
  *)
    echo "ERROR: FREQ_INDEX must be 0, 1, 2, or 3." >&2
    exit 8
    ;;
esac

tmp="$(mktemp)"
remote="/tmp/m5_fan_temp_control_$$.sh"
trap 'rm -f "$tmp"' EXIT

cat > "$tmp" <<'REMOTE_SCRIPT'
#!/usr/bin/env bash
set -eu

addr="0x18"
up=(50000 60000 67500 75000)
down=(45000 55000 62500 70000)
duties=(0 30 50 70 100)
level=-1
last_duty=-1

find_fan_bus() {
  for dev in /dev/i2c-*; do
    [ -e "$dev" ] || continue
    bus="${dev##*-}"
    if i2cget -y "$bus" "$addr" 0x00 >/dev/null 2>&1; then
      printf '%s\n' "$bus"
      return 0
    fi
  done
  return 1
}

find_soc_temp_path() {
  for z in /sys/class/thermal/thermal_zone*; do
    [ -r "$z/type" ] || continue
    [ -r "$z/temp" ] || continue
    type="$(cat "$z/type" 2>/dev/null || true)"
    case "$type" in
      soc_thm|*soc*|*SoC*|*SOC*)
        printf '%s\n' "$z/temp"
        return 0
        ;;
    esac
  done

  for z in /sys/class/thermal/thermal_zone*; do
    [ -r "$z/temp" ] || continue
    printf '%s\n' "$z/temp"
    return 0
  done

  return 1
}

direct_level() {
  raw="$1"
  if [ "$raw" -ge 75000 ]; then
    echo 4
  elif [ "$raw" -ge 67500 ]; then
    echo 3
  elif [ "$raw" -ge 60000 ]; then
    echo 2
  elif [ "$raw" -ge 50000 ]; then
    echo 1
  else
    echo 0
  fi
}

update_level() {
  raw="$1"
  if [ "$level" -lt 0 ]; then
    level="$(direct_level "$raw")"
  else
    while [ "$level" -lt 4 ] && [ "$raw" -ge "${up[$level]}" ]; do
      level=$((level + 1))
    done
    while [ "$level" -gt 0 ] && [ "$raw" -lt "${down[$((level - 1))]}" ]; do
      level=$((level - 1))
    done
  fi
}

write_fan() {
  duty="$1"
  if [ "$duty" = "$last_duty" ]; then
    return 0
  fi

  status=1
  if [ "$duty" -eq 0 ]; then
    status=0
  fi

  i2cset -y "$fan_bus" "$addr" 0x10 "$FREQ_INDEX" b
  i2cset -y "$fan_bus" "$addr" 0x20 "$duty" b
  i2cset -y "$fan_bus" "$addr" 0x00 "$status" b
  last_duty="$duty"
}

read_rpm() {
  lo="$(i2cget -y "$fan_bus" "$addr" 0x30 2>/dev/null || echo 0x00)"
  hi="$(i2cget -y "$fan_bus" "$addr" 0x31 2>/dev/null || echo 0x00)"
  printf '%d\n' "$(( (hi << 8) | lo ))"
}

fan_bus="$(find_fan_bus)" || {
  echo "ERROR: Fan Module v1.1 address 0x18 not detected on /dev/i2c-*; no fan register writes performed." >&2
  exit 10
}

temp_path="$(find_soc_temp_path)" || {
  echo "ERROR: no readable SoC thermal temp node found." >&2
  exit 11
}

zone="$(dirname "$temp_path")"
temp_type="unknown"
[ -r "$zone/type" ] && temp_type="$(cat "$zone/type" 2>/dev/null || true)"

echo "fan_bus=i2c-$fan_bus addr=$addr temp_zone=$zone type=$temp_type curve=Pi5-like duties=0,30,50,70,100 hysteresis=5C"

i=0
while :; do
  raw="$(cat "$temp_path")"
  update_level "$raw"
  duty="${duties[$level]}"
  write_fan "$duty"
  rpm="$(read_rpm)"
  ts="$(date '+%Y-%m-%d %H:%M:%S')"
  printf '%s raw=%s temp=%d.%03dC level=%d duty=%d%% rpm=%s\n' \
    "$ts" "$raw" "$((raw / 1000))" "$((raw % 1000))" "$level" "$duty" "$rpm"

  i=$((i + 1))
  if [ "$COUNT" -gt 0 ] && [ "$i" -ge "$COUNT" ]; then
    exit 0
  fi
  sleep "$INTERVAL"
done
REMOTE_SCRIPT

"$ADB_BIN" -s "$serial" push "$tmp" "$remote" >/dev/null
"$ADB_BIN" -s "$serial" shell "chmod 700 '$remote' && INTERVAL='$interval' COUNT='$count' FREQ_INDEX='$freq_index' '$remote'; rc=\$?; rm -f '$remote'; exit \$rc"
