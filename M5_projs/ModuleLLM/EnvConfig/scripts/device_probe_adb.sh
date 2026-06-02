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
    echo "ERROR: adb not found. Ask user whether to install Android SDK Platform Tools, or set ADB_BIN to an existing adb." >&2
    return 2
  fi

  "$adb_bin" version >/dev/null
  "$adb_bin" start-server >/dev/null
  "$adb_bin" devices -l >&2

  serial="${1:-${ADB_SERIAL:-${ANDROID_SERIAL:-}}}"
  devices="$("$adb_bin" devices | awk 'NR>1 && $2=="device" {print $1}')"
  count="$(printf '%s\n' "$devices" | sed '/^$/d' | wc -l | tr -d ' ')"

  if [ "$count" = "0" ]; then
    echo "ERROR: no adb device in 'device' state. Check USB-C data cable, port, driver/permissions, and power." >&2
    return 3
  fi

  if [ -n "$serial" ]; then
    if printf '%s\n' "$devices" | grep -Fx "$serial" >/dev/null 2>&1; then
      printf '%s\n' "$serial"
      return 0
    fi
    echo "ERROR: requested serial '$serial' is not in current adb device list." >&2
    echo "Current devices:" >&2
    printf '%s\n' "$devices" >&2
    return 4
  fi

  if [ "$count" = "1" ]; then
    printf '%s\n' "$devices"
    return 0
  fi

  echo "ERROR: multiple adb devices found. Set ADB_SERIAL=<serial> or pass serial as first argument." >&2
  echo "Current devices:" >&2
  printf '%s\n' "$devices" >&2
  return 5
}


mkdir -p inventory/before
ts="$(date +%Y%m%d-%H%M%S)"
out="inventory/before/device-probe-adb-$ts.txt"
if ADB_RESOLVED="$(resolve_adb_bin)"; then
  ADB_BIN="$ADB_RESOLVED"
else
  echo "ERROR: adb not found. Ask user whether to install Android SDK Platform Tools, or set ADB_BIN to an existing adb." >&2
  exit 2
fi
serial="$(choose_adb_serial "${1:-}")"

remote_script='
echo "## date"; date
echo "## hostname"; hostname || true
echo "## uname"; uname -a
echo "## os-release"; cat /etc/os-release || true
echo "## cmdline"; cat /proc/cmdline || true
echo "## disk"; df -h; echo; lsblk || true
echo "## memory"; free -h || true
echo "## network"; ip addr; echo; ip route || true
echo "## mounts"; mount || true
echo "## apt sources"; find /etc/apt -maxdepth 2 -type f -print -exec sed -n "1,120p" {} \; || true
echo "## installed llm packages via apt"; apt list --installed 2>/dev/null | grep -E "(^lib-llm|^llm-)" || true
echo "## installed llm packages via dpkg"; dpkg -l | grep -E " lib-llm| llm-" || true
echo "## opt"; ls -la /opt || true
'

{
  echo "## Host timestamp"
  date
  echo
  echo "## adb path"
  echo "$ADB_BIN"
  echo "recommended env: export ADB_BIN=\"$ADB_BIN\""
  echo
  echo "## adb version"
  "$ADB_BIN" version
  echo
  echo "## adb devices -l"
  "$ADB_BIN" devices -l
  echo
  echo "## selected serial"
  echo "$serial"
  echo
  echo "## device probe"
  "$ADB_BIN" -s "$serial" shell "$remote_script"
} | tee "$out"

echo "Saved: $out"
