# Build Troubleshooting And Incident Log

This file is the durable record for unusually slow, stalled, or inconsistent
PlatformIO builds on the Windows development host. Update it when a new
symptom, discriminating test, confirmed cause, or effective workaround is
found.

Do not change firmware source merely because a compiler process is slow. First
separate source diagnostics from host-side process, filesystem, dependency,
and output-capture problems.

## Standard Build Workflow

Use the detached build wrapper:

```powershell
.\tools\tab5.ps1 build tab5_min_uart_terminal
```

The worker runs independently and writes stdout/stderr directly under
`.logs/`. If the caller or Codex tool times out, the build continues without
depending on the caller's output pipe.

Inspect or rejoin it with:

```powershell
.\tools\tab5.ps1 build-status tab5_min_uart_terminal
.\tools\tab5.ps1 build-wait tab5_min_uart_terminal
```

Starting `build` again for the same environment reuses a recorded active
worker instead of launching a duplicate build.

## Triage Procedure

1. Read `build-status` and the two recorded log paths.
2. Decide whether this is a full dependency rebuild or an incremental build.
   M5GFX, M5Unified, M5UnitUnified, M5Unit-KEYBOARD, and FrameworkArduino make
   a first/full build much longer than a project-source-only build.
3. Check whether the output/error log timestamp or size changes over 10-30
   seconds. Log activity is stronger evidence of progress than total elapsed
   time.
4. If logs do not move, sample the current compiler process twice:

```powershell
Get-CimInstance Win32_Process |
    Where-Object { $_.Name -match 'pio|python|riscv32|cc1plus|collect2' } |
    Select-Object Name,ProcessId,ParentProcessId,KernelModeTime,UserModeTime,CommandLine
```

5. Treat a build as suspected stalled only when the same compile target stays
   current, log size/time does not move, and compiler CPU plus I/O remain zero
   across multiple samples. A single quiet sample is insufficient.
6. Check the exact source named in the compiler command. If a modified project
   source emits a compiler error, fix the source. If an unchanged dependency
   or previously successful source stalls without a diagnostic, investigate
   the host/build process first.
7. Stop only the process tree belonging to the recorded build worker. Never
   kill all `python.exe` processes because unrelated PlatformIO, IDE, or user
   tasks may be running.
8. Retry the same environment once with its existing object cache. Do not
   delete `.pio/` by default; a clean rebuild discards useful evidence and
   makes the slow dependency path repeat.
9. If the issue recurs, append an incident below before changing build flags or
   packages.

## Evidence To Record

For each incident record:

- date and local time;
- PlatformIO environment and `-j` value;
- incremental or full build;
- caller style: foreground pipe, direct file redirect, or detached worker;
- last log line and unchanged duration;
- active process tree and CPU/I/O observations;
- whether the current source was modified;
- retry action and result;
- confidence level of any proposed cause.

## Known Nonfatal Output

The following warning is from the installed ESP-IDF headers and has not blocked
successful builds:

```text
periph_ctrl.h: warning: invalid suffix on literal
```

Do not classify a build as failed based only on this warning.

## Incident 2026-06-08: Caller Timeout And Orphaned Compiler

Context:

- Environment: `tab5_min_uart_terminal`.
- Initial foreground wrapper used `pio run ... *> log`.
- Several tool calls stopped waiting after 124-304 seconds.
- The child PlatformIO/compiler processes remained after the caller ended.

Observed evidence:

- The orphaned `cc1plus` process repeatedly showed zero CPU and zero I/O.
- Its temporary assembly output remained zero bytes.
- The log stopped for several minutes while compiling an unchanged
  `status_bar.cpp`.
- Retrying through an independent hidden process with stdout/stderr redirected
  directly to files passed `status_bar.cpp`, compiled all new UART-management
  sources, and completed successfully in 518.64 seconds.
- The persistent detached wrapper was then exercised on the cached environment;
  it completed in 15.5 seconds and `build-status` read the saved result.

Current conclusion:

- High confidence: caller timeout plus inherited output-pipe lifetime can leave
  the build child tree blocked and make a normal compile look like a source or
  toolchain stall.
- Confirmed workaround: detached worker with direct file logs.
- Not yet proven: whether antivirus, filesystem indexing, toolchain defects,
  or host resource pressure also contribute to unrelated slow builds.

Follow-up rule:

- Use the detached wrapper for all builds expected to exceed a tool-call
  timeout.
- Add later incidents here. Promote a suspected secondary cause to confirmed
  only after a discriminating test supports it.

## Observation 2026-06-12: New Environments Trigger Long Full Builds

Context:

- Screen-capture work added a source file and a new
  `tab5_screen_capture` PlatformIO environment.
- Detached builds continued to update their file-backed logs and completed
  without manual process intervention.

Observed successful durations:

- `tab5_terminal_regression`: 655.12 seconds;
- first `tab5_screen_capture`: 701.91 seconds;
- restored `tab5_min_uart_terminal`: 541.99 seconds.

Interpretation:

- Adding a source file or first building a new environment can compile the full
  M5Unified/M5GFX dependency graph separately for that environment.
- These runs were slow but not stalled: log activity continued and each worker
  wrote a successful result.
- Do not use elapsed time alone to classify a build as stuck. Apply the log,
  process, CPU, and I/O criteria in the triage procedure.

## Observation 2026-06-15: USB Keyboard Probe Rebuilds Dependencies

Context:

- USB-A keyboard preparation added a shared source file and changed both the
  `tab5_usb_keyboard_probe` and `tab5_min_uart_terminal` build graphs.
- Both builds used the detached wrapper and kept updating file-backed logs.

Observed successful durations:

- `tab5_usb_keyboard_probe`: 505.3 seconds;
- `tab5_min_uart_terminal`: 457.9 seconds.

Interpretation:

- The USB probe environment compiled the full M5GFX/M5Unified/M5Unit
  dependency graph and then the project sources. The formal environment also
  rebuilt after the shared HID mapper source was added.
- The caller timed out before the detached worker completed, but the worker
  stayed alive and wrote successful result files.
- Correct response is to use `build-status` / `build-wait` or inspect the
  latest `.logs/build-*.out.log` activity, not to start duplicate builds.
- The probe firmware was later flashed and basic USB-A keyboard input passed,
  so this slow build was not evidence of a USB host implementation failure.

## Incident 2026-06-08: Native stderr Misclassified As Build Failure

Context:

- A cached fixed-cell firmware rebuild emitted the known nonfatal ESP-IDF
  warning on stderr.
- The first detached worker invoked `pio.exe` directly from PowerShell while
  `$ErrorActionPreference` was `Stop`.

Observed evidence:

- The worker stopped after 16.5 seconds with PowerShell
  `NativeCommandError`.
- The captured text began with the known `periph_ctrl.h` warning rather than a
  compiler error.

Confirmed cause and fix:

- PowerShell promoted native stderr output into an error record before the
  worker could judge PlatformIO's process exit code.
- The initial fix used `Start-Process`; the 2026-06-12 incident below
  superseded that implementation. The worker now invokes PlatformIO directly
  with `$ErrorActionPreference = "Continue"`, redirects stdout and stderr to
  separate files, and uses only `$LASTEXITCODE` to decide success.

## Incident 2026-06-12: Duplicate PATH Keys Break Start-Process

Context:

- Environment: `tab5_terminal_regression`, `-j 2`.
- The standard detached worker started normally but failed before PlatformIO
  entered compilation.

Observed evidence:

- `Start-Process` raised an `ArgumentException` stating that dictionary key
  `Path` could not be added because key `PATH` already existed.
- The inherited Codex process environment contained both case variants with
  identical values.
- Output log remained empty and the result file recorded 0.2 seconds, proving
  this was not a compiler or firmware failure.

Confirmed cause and fix:

- `Start-Process` reconstructs the inherited environment as a
  case-insensitive dictionary on Windows and rejects the duplicate keys.
- The worker is already an independent hidden process with file-backed logs,
  so it now invokes PlatformIO directly inside that worker, redirects stdout
  and stderr to their existing separate files, temporarily uses
  `$ErrorActionPreference = "Continue"`, and decides success from
  `$LASTEXITCODE`.
- This preserves detached build lifetime while avoiding `Start-Process`
  environment normalization.
