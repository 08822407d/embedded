param(
    [switch]$Quiet
)

$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

$requiredPaths = @(
    "archive/platformio_tab5_terminal_20260707/platformio.ini",
    "archive/platformio_tab5_terminal_20260707/src/terminal_core.cpp",
    "third_party/libvterm-src/LICENSE",
    "third_party/libvterm-src/include/vterm.h",
    "third_party/libvterm-src/src/screen.c",
    "port/libvterm_adapter/include/tab5_terminal_libvterm_adapter.h",
    "port/libvterm_adapter/src/tab5_terminal_libvterm_adapter.cpp",
    "docs/current_work.md",
    "docs/requirements.md",
    "docs/decision_log.md",
    "docs/problem_log.md"
)

$missing = @()
foreach ($relative in $requiredPaths) {
    $path = Join-Path $root $relative
    if (-not (Test-Path -LiteralPath $path)) {
        $missing += $relative
    }
}

$nestedGit = Get-ChildItem -LiteralPath (Join-Path $root "third_party") `
    -Force -Recurse -Directory -Filter ".git" -ErrorAction SilentlyContinue

if ($missing.Count -gt 0) {
    Write-Error ("Missing required port paths:`n" + ($missing -join "`n"))
}

if ($nestedGit.Count -gt 0) {
    Write-Error ("Nested .git directories found under third_party:`n" +
        (($nestedGit | ForEach-Object { $_.FullName }) -join "`n"))
}

if (-not $Quiet) {
    Write-Host "Tab5 official-firmware port layout check passed."
    Write-Host "Root: $root"
    Write-Host "Checked paths: $($requiredPaths.Count)"
}
