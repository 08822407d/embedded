[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet(
        "build",
        "build-status",
        "build-wait",
        "flash",
        "probe",
        "boot-log",
        "app-smoke",
        "terminal-regression",
        "screenshot",
        "baud",
        "power-detect"
    )]
    [string]$Action,

    [Parameter(Position = 1)]
    [string]$Environment = "tab5_min_uart_terminal",

    [string]$Port = "COM3",
    [int]$Jobs = 2,
    [string]$Apps = "clear,reset,tput,less,nano,vim,htop",
    [int]$Baud = 0,
    [switch]$Persist,
    [switch]$Tab5Only,
    [switch]$DefaultBaud,
    [string]$LinuxTty = "/dev/ttyS1",
    [int]$BuildWaitMinutes = 30,
    [int]$BuildStallWarningSeconds = 120,
    [string]$OutputPath = "",
    [string]$Baseline = "",
    [int]$ChannelTolerance = 0,
    [int]$DurationSeconds = 180
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$logRoot = Join-Path $projectRoot ".logs"
$platformioRoot = Join-Path $env:USERPROFILE ".platformio"
$python = Join-Path $platformioRoot "penv\Scripts\python.exe"
$pio = Join-Path $platformioRoot "penv\Scripts\pio.exe"
$toolchainBin = Join-Path $platformioRoot "packages\toolchain-riscv32-esp\riscv32-esp-elf\bin"
$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"

New-Item -ItemType Directory -Force -Path $logRoot | Out-Null

function Invoke-LoggedCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Label,
        [Parameter(Mandatory = $true)]
        [string]$LogPath,
        [Parameter(Mandatory = $true)]
        [scriptblock]$Command
    )

    $started = Get-Date
    Write-Host "$Label ..."
    & $Command *> $LogPath
    $exitCode = $LASTEXITCODE
    $elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)

    if ($exitCode -ne 0) {
        Write-Host "$Label failed after ${elapsed}s. Log: $LogPath" -ForegroundColor Red
        Get-Content -Path $LogPath -Tail 80
        exit $exitCode
    }

    Write-Host "$Label succeeded in ${elapsed}s. Log: $LogPath" -ForegroundColor Green
}

function Assert-CommandPath {
    param([string]$Path, [string]$Name)
    if (-not (Test-Path -LiteralPath $Path)) {
        throw "$Name was not found at $Path"
    }
}

function Assert-PortPresent {
    param([string]$SerialPort)

    $portPattern = [regex]::Escape($SerialPort)
    $device = Get-CimInstance Win32_PnPEntity |
        Where-Object { $_.Name -match "\($portPattern\)" } |
        Select-Object -First 1

    if ($null -eq $device) {
        throw "Serial port $SerialPort is not currently present."
    }

    Write-Host "Port: $($device.Name)"
}

function Show-MatchingLogLines {
    param([string]$LogPath, [string[]]$Patterns)

    foreach ($pattern in $Patterns) {
        Select-String -Path $LogPath -Pattern $pattern |
            Select-Object -Last 1 |
            ForEach-Object { Write-Host $_.Line }
    }
}

function Get-BuildStatePath {
    param([string]$BuildEnvironment)
    return Join-Path $logRoot "build-state-$BuildEnvironment.json"
}

function Read-BuildState {
    param([string]$BuildEnvironment)
    $statePath = Get-BuildStatePath $BuildEnvironment
    if (-not (Test-Path -LiteralPath $statePath)) {
        return $null
    }
    return Get-Content -Raw -LiteralPath $statePath | ConvertFrom-Json
}

function Test-ProcessRunning {
    param([int]$ProcessId)
    return $null -ne (Get-Process -Id $ProcessId -ErrorAction SilentlyContinue)
}

function Show-BuildState {
    param([string]$BuildEnvironment)

    $state = Read-BuildState $BuildEnvironment
    if ($null -eq $state) {
        Write-Host "No recorded build for $BuildEnvironment."
        return $null
    }

    $result = $null
    if (Test-Path -LiteralPath $state.result_file) {
        $result = Get-Content -Raw -LiteralPath $state.result_file | ConvertFrom-Json
    }

    if ($null -ne $result) {
        $status = if ($result.exit_code -eq 0) { "succeeded" } else { "failed" }
        Write-Host "Build $BuildEnvironment $status in $($result.elapsed_seconds)s."
        Write-Host "Output: $($result.output_log)"
        if ((Get-Item -LiteralPath $result.error_log).Length -gt 0) {
            Write-Host "Warnings/errors: $($result.error_log)"
        }
        return $result
    }

    $running = Test-ProcessRunning ([int]$state.worker_pid)
    $started = [datetime]$state.started_at
    $elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)
    $lastActivity = $started
    foreach ($path in @($state.output_log, $state.error_log)) {
        if (Test-Path -LiteralPath $path) {
            $candidate = (Get-Item -LiteralPath $path).LastWriteTime
            if ($candidate -gt $lastActivity) {
                $lastActivity = $candidate
            }
        }
    }
    $idleSeconds = [math]::Round(((Get-Date) - $lastActivity).TotalSeconds, 1)
    $status = if ($running) { "running" } else { "orphaned" }

    Write-Host(
        "Build $BuildEnvironment is ${status}: worker=$($state.worker_pid) " +
        "elapsed=${elapsed}s log_idle=${idleSeconds}s"
    )
    Write-Host "Output: $($state.output_log)"
    if ($idleSeconds -ge $BuildStallWarningSeconds) {
        Write-Warning(
            "No build-log activity for ${idleSeconds}s. Follow " +
            "docs/build_troubleshooting.md before stopping the process."
        )
    }
    return $state
}

function Start-DetachedBuild {
    param([string]$BuildEnvironment)

    $existing = Read-BuildState $BuildEnvironment
    if ($null -ne $existing -and
        -not (Test-Path -LiteralPath $existing.result_file) -and
        (Test-ProcessRunning ([int]$existing.worker_pid))) {
        Write-Host "Reusing active build worker $($existing.worker_pid)."
        return
    }

    $worker = Join-Path $PSScriptRoot "platformio_build_worker.ps1"
    $outputLog = Join-Path $logRoot "build-$BuildEnvironment-$timestamp.out.log"
    $errorLog = Join-Path $logRoot "build-$BuildEnvironment-$timestamp.err.log"
    $resultFile = Join-Path $logRoot "build-$BuildEnvironment-$timestamp.result.json"
    $statePath = Get-BuildStatePath $BuildEnvironment
    $arguments = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", "`"$worker`"",
        "-ProjectRoot", "`"$projectRoot`"",
        "-PlatformIo", "`"$pio`"",
        "-ToolchainBin", "`"$toolchainBin`"",
        "-Environment", $BuildEnvironment,
        "-Jobs", $Jobs,
        "-OutputLog", "`"$outputLog`"",
        "-ErrorLog", "`"$errorLog`"",
        "-ResultFile", "`"$resultFile`""
    )
    $process = Start-Process `
        -FilePath "powershell.exe" `
        -ArgumentList $arguments `
        -WindowStyle Hidden `
        -PassThru

    $state = [ordered]@{
        environment = $BuildEnvironment
        worker_pid = $process.Id
        started_at = (Get-Date).ToString("o")
        jobs = $Jobs
        output_log = $outputLog
        error_log = $errorLog
        result_file = $resultFile
    }
    $state | ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding utf8
    Write-Host "Started detached build worker $($process.Id) for $BuildEnvironment."
}

function Wait-DetachedBuild {
    param([string]$BuildEnvironment)

    $deadline = (Get-Date).AddMinutes($BuildWaitMinutes)
    while ((Get-Date) -lt $deadline) {
        $state = Read-BuildState $BuildEnvironment
        if ($null -eq $state) {
            throw "No recorded build for $BuildEnvironment."
        }
        if (Test-Path -LiteralPath $state.result_file) {
            $result = Show-BuildState $BuildEnvironment
            if ($result.exit_code -ne 0) {
                Get-Content -LiteralPath $result.error_log -Tail 80
                exit [int]$result.exit_code
            }
            Show-MatchingLogLines -LogPath $result.output_log -Patterns @(
                "^RAM:",
                "^Flash:",
                "^=+ \[SUCCESS\]"
            )
            return
        }
        if (-not (Test-ProcessRunning ([int]$state.worker_pid))) {
            Show-BuildState $BuildEnvironment | Out-Null
            throw "Build worker exited without writing a result file."
        }
        Start-Sleep -Seconds 2
    }

    Show-BuildState $BuildEnvironment | Out-Null
    throw(
        "Build wait exceeded $BuildWaitMinutes minutes. The detached worker " +
        "was not stopped; use build-status to inspect it."
    )
}

Push-Location $projectRoot
try {
    switch ($Action) {
        "build" {
            Assert-CommandPath $pio "PlatformIO"
            Start-DetachedBuild $Environment
            Wait-DetachedBuild $Environment
        }

        "build-status" {
            Show-BuildState $Environment | Out-Null
        }

        "build-wait" {
            Wait-DetachedBuild $Environment
        }

        "flash" {
            Assert-CommandPath $python "PlatformIO Python"
            Assert-PortPresent $Port

            $esptool = Join-Path $platformioRoot "packages\tool-esptoolpy\esptool.py"
            $bootApp0 = Join-Path $platformioRoot "packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin"
            $buildDir = Join-Path $projectRoot ".pio\build\$Environment"
            $bootloader = Join-Path $buildDir "bootloader.bin"
            $partitions = Join-Path $buildDir "partitions.bin"
            $firmware = Join-Path $buildDir "firmware.bin"

            foreach ($artifact in @($esptool, $bootApp0, $bootloader, $partitions, $firmware)) {
                if (-not (Test-Path -LiteralPath $artifact)) {
                    throw "Required flash artifact is missing: $artifact"
                }
            }

            $env:PYTHONUTF8 = "1"
            $env:PYTHONIOENCODING = "utf-8"
            $log = Join-Path $logRoot "flash-$Environment-$timestamp.log"

            Invoke-LoggedCommand -Label "Flash $Environment to $Port" -LogPath $log -Command {
                & $python $esptool --chip esp32p4 --port $Port --baud 1500000 `
                    --before default-reset --after hard-reset write-flash -z `
                    0x2000 $bootloader `
                    0x8000 $partitions `
                    0xe000 $bootApp0 `
                    0x10000 $firmware
            }

            Show-MatchingLogLines -LogPath $log -Patterns @(
                "^Chip is ",
                "^MAC:",
                "^Hash of data verified",
                "^Hard resetting"
            )
        }

        "probe" {
            Assert-CommandPath $python "PlatformIO Python"
            Assert-PortPresent $Port
            $script = Join-Path $PSScriptRoot "send_login_shell_demo.py"
            $log = Join-Path $logRoot "probe-$timestamp.log"

            Invoke-LoggedCommand -Label "Login shell probe on $Port" -LogPath $log -Command {
                & $python $script --port $Port --demo probe
            }

            Get-Content -Path $log | Select-Object -Last 20
        }

        "boot-log" {
            Assert-CommandPath $python "PlatformIO Python"
            Assert-PortPresent $Port
            $script = Join-Path $PSScriptRoot "capture_boot_log.py"
            $esptool = Join-Path $platformioRoot "packages\tool-esptoolpy\esptool.py"
            Assert-CommandPath $esptool "esptool"
            $resetLog = Join-Path $logRoot "reset-$timestamp.log"
            $log = Join-Path $logRoot "boot-log-$timestamp.log"

            Invoke-LoggedCommand -Label "Reset Tab5 on $Port" -LogPath $resetLog -Command {
                & $python $esptool --chip esp32p4 --port $Port --after hard-reset run
            }
            Start-Sleep -Milliseconds 300

            $bootLogSeconds = if ($PSBoundParameters.ContainsKey("DurationSeconds")) {
                $DurationSeconds
            } else {
                5
            }
            Invoke-LoggedCommand -Label "Capture boot log on $Port" -LogPath $log -Command {
                & $python $script --port $Port --duration $bootLogSeconds
            }

            Select-String -Path $log -Pattern "Tab5|tab5-kbd|Login UART|keyboard" |
                ForEach-Object { Write-Host $_.Line }
        }

        "app-smoke" {
            Assert-CommandPath $python "PlatformIO Python"
            Assert-PortPresent $Port
            $script = Join-Path $PSScriptRoot "send_login_shell_app_smoke.py"
            $log = Join-Path $logRoot "app-smoke-$timestamp.log"

            Invoke-LoggedCommand -Label "Login shell app smoke on $Port" -LogPath $log -Command {
                & $python $script --port $Port --apps $Apps --rows 32 --cols 64 `
                    --chunk-size 32 --chunk-delay 0.08
            }

            Get-Content -Path $log | Select-Object -Last 30
        }

        "terminal-regression" {
            Assert-CommandPath $python "PlatformIO Python"
            Assert-PortPresent $Port
            $script = Join-Path $PSScriptRoot "run_terminal_regression.py"
            $rawLog = Join-Path $logRoot "terminal-regression-$timestamp.raw.log"
            $log = Join-Path $logRoot "terminal-regression-$timestamp.log"

            Invoke-LoggedCommand -Label "Terminal regression on $Port" -LogPath $log -Command {
                & $python $script --port $Port --raw-log $rawLog
            }

            Get-Content -Path $log | Select-Object -Last 20
        }

        "screenshot" {
            Assert-CommandPath $python "PlatformIO Python"
            Assert-PortPresent $Port
            $script = Join-Path $PSScriptRoot "capture_screen.py"
            $screenshotRoot = Join-Path $logRoot "screenshots"
            New-Item -ItemType Directory -Force -Path $screenshotRoot | Out-Null
            $capturePath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
                Join-Path $screenshotRoot "tab5-screen-$timestamp.png"
            } elseif ([System.IO.Path]::IsPathRooted($OutputPath)) {
                $OutputPath
            } else {
                Join-Path $projectRoot $OutputPath
            }
            $arguments = @(
                "--port", $Port,
                "--output", $capturePath,
                "--channel-tolerance", "$ChannelTolerance"
            )
            if (-not [string]::IsNullOrWhiteSpace($Baseline)) {
                $baselinePath = if ([System.IO.Path]::IsPathRooted($Baseline)) {
                    $Baseline
                } else {
                    Join-Path $projectRoot $Baseline
                }
                $arguments += @("--baseline", $baselinePath)
            }
            $log = Join-Path $logRoot "screenshot-$timestamp.log"

            Invoke-LoggedCommand -Label "Capture Tab5 screen on $Port" -LogPath $log -Command {
                & $python $script @arguments
            }

            Get-Content -Path $log | Select-Object -Last 12
        }

        "baud" {
            Assert-CommandPath $python "PlatformIO Python"
            Assert-PortPresent $Port
            $script = Join-Path $PSScriptRoot "login_uart_baud.py"
            $log = Join-Path $logRoot "baud-$timestamp.log"
            $arguments = @("--port", $Port, "--linux-tty", $LinuxTty)

            if ($DefaultBaud) {
                $arguments += "--default"
            }
            elseif ($Baud -gt 0) {
                $arguments += @("--baud", $Baud)
            }
            if ($Persist) {
                $arguments += "--persist"
            }
            if ($Tab5Only) {
                $arguments += "--tab5-only"
            }

            Invoke-LoggedCommand -Label "Login UART baud management on $Port" -LogPath $log -Command {
                & $python $script @arguments
            }

            Get-Content -Path $log | Select-Object -Last 10
        }

        "power-detect" {
            Assert-CommandPath $python "PlatformIO Python"
            Assert-PortPresent $Port
            $script = Join-Path $PSScriptRoot "power_detect_probe.py"
            $log = Join-Path $logRoot "power-detect-$timestamp.log"

            Write-Host "Power-detect capture on $Port for ${DurationSeconds}s ..."
            & $python $script --port $Port --duration $DurationSeconds --output-root $logRoot 2>&1 |
                Tee-Object -FilePath $log
            $exitCode = $LASTEXITCODE
            if ($exitCode -ne 0) {
                Write-Host "Power-detect capture failed. Log: $log" -ForegroundColor Red
                exit $exitCode
            }
            Write-Host "Power-detect capture succeeded. Log: $log" -ForegroundColor Green
        }
    }
}
finally {
    Pop-Location
}
