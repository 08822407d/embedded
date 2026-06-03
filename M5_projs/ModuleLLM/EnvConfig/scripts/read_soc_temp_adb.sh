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

remote_script='
set -eu

find_soc_temp_path() {
  for z in /sys/class/thermal/thermal_zone*; do
    [ -r "$z/type" ] || continue
    [ -r "$z/temp" ] || continue
    type="$(cat "$z/type" 2>/dev/null || true)"
    case "$type" in
      soc_thm|*soc*|*SoC*|*SOC*)
        printf "%s\n" "$z/temp"
        return 0
        ;;
    esac
  done

  for z in /sys/class/thermal/thermal_zone*; do
    [ -r "$z/temp" ] || continue
    printf "%s\n" "$z/temp"
    return 0
  done

  return 1
}

path="$(find_soc_temp_path)" || {
  echo "ERROR: no readable thermal_zone temp node found." >&2
  exit 1
}

zone="$(dirname "$path")"
type="unknown"
[ -r "$zone/type" ] && type="$(cat "$zone/type" 2>/dev/null || true)"

i=0
while :; do
  raw="$(cat "$path")"
  ts="$(date "+%Y-%m-%d %H:%M:%S")"
  if [ "$raw" -ge 1000 ] 2>/dev/null; then
    c_int=$((raw / 1000))
    c_frac=$((raw % 1000))
    printf "%s zone=%s type=%s raw=%s temp=%d.%03d C\n" "$ts" "$zone" "$type" "$raw" "$c_int" "$c_frac"
  else
    printf "%s zone=%s type=%s raw=%s temp=%s C\n" "$ts" "$zone" "$type" "$raw" "$raw"
  fi

  i=$((i + 1))
  if [ "$COUNT" -gt 0 ] && [ "$i" -ge "$COUNT" ]; then
    exit 0
  fi
  sleep "$INTERVAL"
done
'

"$ADB_BIN" -s "$serial" shell "INTERVAL='$interval'; COUNT='$count'; $remote_script"
