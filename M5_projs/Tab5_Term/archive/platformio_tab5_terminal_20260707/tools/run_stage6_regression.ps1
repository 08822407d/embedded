[CmdletBinding()]
param(
    [string]$Port = "COM3",
    [int]$Jobs = 2,
    [string]$Apps = "clear,reset,tput,less,nano,vim,htop",
    [int]$BuildWaitMinutes = 30,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$tab5 = Join-Path $PSScriptRoot "tab5.ps1"
$regressionEnvironment = "tab5_terminal_regression"
$formalEnvironment = "tab5_min_uart_terminal"
$regressionFlashed = $false

function Invoke-Tab5 {
    param([string[]]$Arguments)

    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $tab5 @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "tab5.ps1 failed: $($Arguments -join ' ')"
    }
}

try {
    if (-not $SkipBuild) {
        Invoke-Tab5 @(
            "build",
            $regressionEnvironment,
            "-Jobs", "$Jobs",
            "-BuildWaitMinutes", "$BuildWaitMinutes"
        )
    }
    Invoke-Tab5 @("flash", $regressionEnvironment, "-Port", $Port)
    $regressionFlashed = $true
    Invoke-Tab5 @("terminal-regression", "-Port", $Port)
}
finally {
    if ($regressionFlashed) {
        if (-not $SkipBuild) {
            Invoke-Tab5 @(
                "build",
                $formalEnvironment,
                "-Jobs", "$Jobs",
                "-BuildWaitMinutes", "$BuildWaitMinutes"
            )
        }
        Invoke-Tab5 @("flash", $formalEnvironment, "-Port", $Port)
    }
}

Invoke-Tab5 @("probe", "-Port", $Port)
Invoke-Tab5 @("app-smoke", "-Port", $Port, "-Apps", $Apps)

Write-Host "Stage 6 automated regression passed; formal firmware restored." -ForegroundColor Green
