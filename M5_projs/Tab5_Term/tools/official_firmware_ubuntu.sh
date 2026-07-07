#!/usr/bin/env bash
set -euo pipefail

ACTION="${1:-check}"
FIRMWARE_REPO="${FIRMWARE_REPO:-https://github.com/m5stack/M5Tab5-UserDemo.git}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
WORK_ROOT="$REPO_ROOT/worktrees/ubuntu"
FIRMWARE_DIR="$WORK_ROOT/M5Tab5-UserDemo"
DESKTOP_BUILD_DIR="$WORK_ROOT/build-desktop"

run() {
    echo "> $*"
    "$@"
}

tool_status() {
    for name in git python3 cmake ninja gcc g++ clang idf.py; do
        if command -v "$name" >/dev/null 2>&1; then
            printf '%s: %s\n' "$name" "$(command -v "$name")"
        else
            printf '%s: missing\n' "$name"
        fi
    done
    printf 'IDF_PATH: %s\n' "${IDF_PATH:-not set}"
}

ensure_repo() {
    command -v git >/dev/null 2>&1 || {
        echo "git is required." >&2
        exit 1
    }
    mkdir -p "$WORK_ROOT"
    if [[ ! -d "$FIRMWARE_DIR" ]]; then
        run git clone "$FIRMWARE_REPO" "$FIRMWARE_DIR"
    elif [[ ! -d "$FIRMWARE_DIR/.git" ]]; then
        echo "Firmware path exists but is not a git repository: $FIRMWARE_DIR" >&2
        exit 1
    else
        echo "Firmware repository already present: $FIRMWARE_DIR"
    fi
}

ensure_deps() {
    command -v python3 >/dev/null 2>&1 || {
        echo "python3 is required." >&2
        exit 1
    }
    python3 - "$FIRMWARE_DIR" <<'PY'
import json
import pathlib
import subprocess
import sys

firmware = pathlib.Path(sys.argv[1])
repos_file = firmware / "repos.json"
repos = json.loads(repos_file.read_text(encoding="utf-8"))
for repo in repos:
    target = firmware / repo["path"]
    if (target / ".git").is_dir():
        print(f"Dependency already present: {repo['path']}")
        continue
    if target.exists():
        raise SystemExit(f"Dependency path exists but is not a git repository: {target}")
    target.parent.mkdir(parents=True, exist_ok=True)
    cmd = ["git", "clone", "--depth", "1", "-b", repo["branch"], repo["url"], str(target)]
    print("> " + " ".join(cmd))
    subprocess.check_call(cmd, cwd=str(firmware))
PY
}

setup() {
    ensure_repo
    ensure_deps
}

build_desktop() {
    setup
    command -v cmake >/dev/null 2>&1 || {
        echo "cmake is required. Ubuntu 24.04: sudo apt install build-essential cmake ninja-build libsdl2-dev" >&2
        exit 1
    }
    if ! command -v gcc >/dev/null 2>&1 && ! command -v clang >/dev/null 2>&1; then
        echo "No C compiler is visible in PATH. Ubuntu 24.04: sudo apt install build-essential cmake ninja-build libsdl2-dev" >&2
        exit 1
    fi
    if command -v pkg-config >/dev/null 2>&1 && ! pkg-config --exists sdl2; then
        echo "SDL2 development files are missing. Ubuntu 24.04: sudo apt install build-essential cmake ninja-build libsdl2-dev" >&2
        exit 1
    fi

    cmake_args=(-S "$FIRMWARE_DIR" -B "$DESKTOP_BUILD_DIR")
    if command -v ninja >/dev/null 2>&1; then
        cmake_args+=(-G Ninja)
    fi
    run cmake "${cmake_args[@]}"
    run cmake --build "$DESKTOP_BUILD_DIR" --parallel
}

build_idf() {
    setup
    if ! command -v idf.py >/dev/null 2>&1; then
        echo "idf.py is missing. Export ESP-IDF v5.4.2 before running build-idf." >&2
        exit 1
    fi
    (
        cd "$FIRMWARE_DIR/platforms/tab5"
        run idf.py build
    )
}

case "$ACTION" in
    check)
        tool_status
        if [[ -d "$FIRMWARE_DIR/.git" ]]; then
            run git -C "$FIRMWARE_DIR" rev-parse --short HEAD
        else
            echo "Firmware repository not present: $FIRMWARE_DIR"
        fi
        ;;
    setup)
        setup
        ;;
    build-desktop)
        build_desktop
        ;;
    build-idf)
        build_idf
        ;;
    *)
        echo "Usage: $0 [check|setup|build-desktop|build-idf]" >&2
        exit 2
        ;;
esac
