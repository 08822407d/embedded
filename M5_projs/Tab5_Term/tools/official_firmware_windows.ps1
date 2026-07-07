param(
    [ValidateSet("check", "setup", "build-desktop", "build-idf")]
    [string]$Action = "check",
    [string]$FirmwareRepo = "https://github.com/m5stack/M5Tab5-UserDemo.git"
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = [string](Resolve-Path (Join-Path $ScriptDir ".."))
$WorkRoot = Join-Path $RepoRoot "worktrees\windows"
$FirmwareDir = Join-Path $WorkRoot "M5Tab5-UserDemo"
$DesktopBuildDir = Join-Path $WorkRoot "build-desktop"

function Invoke-Checked {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [string]$WorkingDirectory
    )

    Write-Host ("> {0} {1}" -f $FilePath, ($Arguments -join " "))
    $process = Start-Process -FilePath $FilePath `
        -ArgumentList $Arguments `
        -WorkingDirectory $WorkingDirectory `
        -NoNewWindow `
        -Wait `
        -PassThru
    if ($process.ExitCode -ne 0) {
        throw ("Command failed with exit code {0}: {1}" -f $process.ExitCode, $FilePath)
    }
}

function Get-Tool {
    param([string]$Name)
    return Get-Command $Name -ErrorAction SilentlyContinue
}

function Show-ToolStatus {
    $toolNames = @("git", "python", "cmake", "ninja", "cl", "gcc", "clang", "idf.py")
    foreach ($name in $toolNames) {
        $tool = Get-Tool $name
        if ($tool) {
            Write-Host ("{0}: {1}" -f $name, $tool.Source)
        } else {
            Write-Host ("{0}: missing" -f $name)
        }
    }
    if ($env:IDF_PATH) {
        Write-Host ("IDF_PATH: {0}" -f $env:IDF_PATH)
    } else {
        Write-Host "IDF_PATH: not set"
    }
}

function Ensure-FirmwareRepo {
    New-Item -ItemType Directory -Force -Path $WorkRoot | Out-Null

    if (-not (Test-Path $FirmwareDir)) {
        Invoke-Checked "git" @("clone", $FirmwareRepo, $FirmwareDir) $WorkRoot
        return
    }

    if (-not (Test-Path (Join-Path $FirmwareDir ".git"))) {
        throw ("Firmware path exists but is not a git repository: {0}" -f $FirmwareDir)
    }

    Write-Host ("Firmware repository already present: {0}" -f $FirmwareDir)
}

function Ensure-Dependencies {
    $reposFile = Join-Path $FirmwareDir "repos.json"
    if (-not (Test-Path $reposFile)) {
        throw ("Missing dependency list: {0}" -f $reposFile)
    }

    $repos = Get-Content $reposFile -Raw | ConvertFrom-Json
    foreach ($repo in $repos) {
        $relativePath = $repo.path -replace "/", "\"
        $target = Join-Path $FirmwareDir $relativePath
        if (Test-Path (Join-Path $target ".git")) {
            Write-Host ("Dependency already present: {0}" -f $repo.path)
            continue
        }
        if (Test-Path $target) {
            throw ("Dependency path exists but is not a git repository: {0}" -f $target)
        }

        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
        Invoke-Checked "git" @("clone", "--depth", "1", "-b", $repo.branch, $repo.url, $target) $FirmwareDir
    }
}

function Setup-Firmware {
    if (-not (Get-Tool "git")) {
        throw "git is required."
    }
    Ensure-FirmwareRepo
    Ensure-Dependencies
}

function Build-Desktop {
    Setup-Firmware
    if (-not (Get-Tool "cmake")) {
        throw "cmake is required for the official desktop build."
    }
    if (-not (Get-Tool "cl") -and -not (Get-Tool "gcc") -and -not (Get-Tool "clang")) {
        throw "No C/C++ compiler is visible in PATH. Use a Visual Studio Developer PowerShell, MinGW, or LLVM before build-desktop."
    }

    $cmakeArgs = @("-S", $FirmwareDir, "-B", $DesktopBuildDir)
    if (Get-Tool "ninja") {
        $cmakeArgs += @("-G", "Ninja")
    }

    Invoke-Checked "cmake" $cmakeArgs $RepoRoot
    Invoke-Checked "cmake" @("--build", $DesktopBuildDir, "--parallel") $RepoRoot
}

function Build-Idf {
    Setup-Firmware
    $idf = Get-Tool "idf.py"
    if (-not $idf) {
        throw "idf.py is missing. Export ESP-IDF v5.4.2 before running build-idf."
    }

    $tab5Dir = Join-Path $FirmwareDir "platforms\tab5"
    Invoke-Checked $idf.Source @("build") $tab5Dir
}

try {
    switch ($Action) {
        "check" {
            Show-ToolStatus
            if (Test-Path $FirmwareDir) {
                Invoke-Checked "git" @("-C", $FirmwareDir, "rev-parse", "--short", "HEAD") $RepoRoot
            } else {
                Write-Host ("Firmware repository not present: {0}" -f $FirmwareDir)
            }
        }
        "setup" { Setup-Firmware }
        "build-desktop" { Build-Desktop }
        "build-idf" { Build-Idf }
    }
} catch {
    Write-Error $_.Exception.Message
    exit 1
}
