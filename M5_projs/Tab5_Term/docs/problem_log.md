# Problem Log

## 2026-07-07: Official Firmware Dependency Fetch Can Look Stalled

Symptom: running the official `fetch_repos.py` on Windows produced a long
period with no useful progress output while fetching/checking out the `lvgl`
dependency. It was interrupted during investigation.

Action taken: switched local helper scripts to shallow clone each dependency
from `repos.json` only when the dependency directory is missing. The current
Windows worktree has all four dependencies present.

Status: mitigated for local work. Use `tools/official_firmware_windows.ps1` or
`tools/official_firmware_ubuntu.sh` instead of manually reasoning through the
official fetch step during routine setup.

## 2026-07-07: Windows ESP-IDF Build Blocked By Missing idf.py

Symptom: `.\tools\official_firmware_windows.ps1 -Action build-idf` stops before
compilation because `idf.py` is missing and `IDF_PATH` is not set.

Action taken: recorded the expected toolchain version in
`docs/official_firmware_build.md`. The script now gives a direct diagnostic.

Status: unresolved until ESP-IDF `v5.4.2` is installed and exported in the
current PowerShell session.

## 2026-07-07: Windows Desktop Smoke Build Blocked By Missing C/C++ Compiler

Symptom: the official desktop CMake build cannot configure because no C or C++
compiler is visible in PATH. The first CMake attempt reported missing
`CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER`.

Action taken: updated the Windows helper script to fail earlier with a clear
message when `cl`, `gcc`, and `clang` are all missing.

Status: unresolved until a Visual Studio Developer PowerShell, MinGW, or LLVM
environment is used.

## 2026-07-07: Ubuntu Script Syntax Check Blocked On This Windows Host

Symptom: invoking `bash` on the current Windows host starts a broken WSL
instance that fails before running the command. Therefore `bash -n
tools/official_firmware_ubuntu.sh` could not be used from this host.

Action taken: kept the Ubuntu script as a standalone file for a real Ubuntu
24.04 environment. Do not treat the Windows WSL failure as evidence about the
script or the official firmware source.

Status: pending validation on Ubuntu 24.04.

## 2026-07-07: Official libvterm Tarball Download Failed In PowerShell

Symptom: downloading
`https://www.leonerd.org.uk/code/libvterm/libvterm-0.3.3.tar.gz` with
`Invoke-WebRequest` failed with a TLS protocol alert:

`Authentication failed because the remote party sent a TLS alert: 'ProtocolVersion'.`

Action taken: cloned `https://github.com/neovim/libvterm.git` with depth 1,
removed its nested `.git`, and kept the resulting source snapshot in
`third_party/libvterm-src/`.

Status: workaround accepted for starting the port quickly. Revisit the source
provenance before release-quality integration.

## 2026-07-07: No Hardware Validation In Current Phase

Symptom: the Tab5 board is not currently available to the user.

Action taken: no build, flash, or runtime claims are made. New code is recorded
as source-level scaffold only.

Status: unresolved until a later device session.

## 2026-07-07: Adapter Callback Access Control

Symptom: the first C++ adapter pass placed a `VTermScreenCallbacks` table at
file scope while callback functions were private static methods. That would
violate C++ access rules.

Action taken: moved the callback table into `TerminalSession::begin()`, where
private static callbacks are accessible.

Status: fixed at source level; not yet compiler-validated.
