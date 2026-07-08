$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir '..')
$cubeG4Dir = Join-Path $env:USERPROFILE 'STM32Cube\Repository\STM32Cube_FW_G4_V1.6.2'
$gccBin = 'C:\Program Files\STMicroelectronics\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin'
$cubeIdeRoot = 'C:\Program Files\STMicroelectronics\STM32CubeIDE_1.19.0\STM32CubeIDE'
$makeExe = Get-ChildItem -LiteralPath $cubeIdeRoot -Recurse -Filter make.exe -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -like '*externaltools.make.win32*' } |
    Select-Object -First 1 -ExpandProperty FullName

if (-not (Test-Path -LiteralPath $cubeG4Dir)) {
    throw "STM32CubeG4 repository not found: $cubeG4Dir"
}

if (-not (Test-Path -LiteralPath $gccBin)) {
    throw "GNU Arm toolchain not found: $gccBin"
}

if (-not $makeExe -or -not (Test-Path -LiteralPath $makeExe)) {
    throw "CubeIDE make not found: $makeExe"
}

$env:Path = "$gccBin;$env:Path"
Push-Location $scriptDir
try {
    & $makeExe CUBE_G4_DIR="$cubeG4Dir" all
    if ($LASTEXITCODE -ne 0) {
        throw "make failed with exit code $LASTEXITCODE"
    }

    Write-Host "Built $scriptDir\build\hall_probe_fw.elf"
} finally {
    Pop-Location
}
