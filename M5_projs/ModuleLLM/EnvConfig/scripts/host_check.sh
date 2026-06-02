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

mkdir -p inventory/before
ts="$(date +%Y%m%d-%H%M%S)"
out="inventory/before/host-tools-$ts.txt"
if ADB_RESOLVED="$(resolve_adb_bin)"; then
  ADB_BIN="$ADB_RESOLVED"
  ADB_FOUND=1
else
  ADB_BIN="${ADB_BIN:-adb}"
  ADB_FOUND=0
fi

{
  echo "## Host tool check"
  date
  echo
  echo "## OS"
  uname -a || true
  [ -f /etc/os-release ] && cat /etc/os-release || true
  echo
  echo "## shell"
  echo "SHELL=${SHELL:-unknown}"
  echo
  echo "## adb path"
  if [ "$ADB_FOUND" = "1" ]; then
    printf '%s\n' "$ADB_BIN"
    echo "recommended env: export ADB_BIN=\"$ADB_BIN\""
  else
    echo "ADB_NOT_FOUND"
    echo "ACTION_REQUIRED: ask user whether to install Android SDK Platform Tools, or set ADB_BIN to an existing adb."
  fi
  echo
  echo "## adb version"
  if [ "$ADB_FOUND" = "1" ]; then "$ADB_BIN" version || true; else echo "SKIPPED: adb not found"; fi
  echo
  echo "## adb start-server"
  if [ "$ADB_FOUND" = "1" ]; then "$ADB_BIN" start-server || true; else echo "SKIPPED: adb not found"; fi
  echo
  echo "## adb devices -l"
  if [ "$ADB_FOUND" = "1" ]; then "$ADB_BIN" devices -l || true; else echo "SKIPPED: adb not found"; fi
  echo
  echo "## ssh"
  ssh -V 2>&1 || true
  echo
  echo "## python"
  python3 --version || true
  echo
  echo "## usb inventory"
  lsusb || true
} | tee "$out"

echo "Saved: $out"
