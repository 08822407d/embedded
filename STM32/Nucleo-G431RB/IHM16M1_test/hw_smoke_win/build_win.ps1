param(
    [switch]$SkipCubeMx
)

$ErrorActionPreference = 'Stop'

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Join-Path $ScriptDir 'g431_hw_smoke_win'
$CubeMxScript = Join-Path $ScriptDir 'cubemx_generate_win.txt'
$CubeMxRoot = 'C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeMX'
$CubeMxJava = Join-Path $CubeMxRoot 'jre\bin\java.exe'
$CubeMxLauncher = Join-Path $CubeMxRoot 'STM32CubeMX.exe'
$G4FwRoot = 'C:\Users\cheyh\STM32Cube\Repository\STM32Cube_FW_G4_V1.6.2'
$TemplateUserDir = Join-Path $G4FwRoot 'Projects\NUCLEO-G431RB\Templates\STM32CubeIDE\Example\User'
$MakeExe = 'C:\Program Files\STMicroelectronics\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.2.0.202409170845\tools\bin\make.exe'
$GccBin = 'C:\Program Files\STMicroelectronics\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin'

foreach ($path in @($CubeMxJava, $CubeMxLauncher, $CubeMxScript, $G4FwRoot, $TemplateUserDir, $MakeExe, $GccBin)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Required path not found: $path"
    }
}

if (-not $SkipCubeMx) {
    $cubeMxArgs = @(
        '-Djavax.net.ssl.trustStoreType=WINDOWS-ROOT',
        '-Dsun.java2d.d3d=false',
        '--add-exports', 'java.desktop/sun.awt=ALL-UNNAMED',
        '--add-opens', 'java.desktop/java.awt=ALL-UNNAMED',
        '-Dfile.encoding=UTF8',
        '-classpath', "$CubeMxLauncher;anything",
        'com.st.microxplorer.maingui.STM32CubeMX',
        '-q', $CubeMxScript
    )
    & $CubeMxJava @cubeMxArgs
    if ($LASTEXITCODE -ne 0) {
        throw "STM32CubeMX generation failed with exit code $LASTEXITCODE"
    }
}

foreach ($stub in @('sysmem.c', 'syscalls.c')) {
    $src = Join-Path $TemplateUserDir $stub
    $dst = Join-Path (Join-Path $ProjectDir 'Src') $stub
    if (-not (Test-Path -LiteralPath $src)) {
        throw "Template not found: $src"
    }
    Copy-Item -LiteralPath $src -Destination $dst -Force
}

$makefile = Join-Path $ProjectDir 'Makefile'
$lines = Get-Content -LiteralPath $makefile
$start = [Array]::IndexOf($lines, 'C_SOURCES =  \')
$asmIndex = [Array]::IndexOf($lines, '# ASM sources')
if ($start -lt 0 -or $asmIndex -le $start) {
    throw 'Could not locate C_SOURCES block in generated Makefile.'
}

$cSources = @(
    'Src/main.c',
    'Src/stm32g4xx_it.c',
    'Src/stm32g4xx_hal_msp.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr_ex.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc_ex.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ex.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ramfunc.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_exti.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma_ex.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr.c',
    'Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c',
    'Src/system_stm32g4xx.c',
    'Src/sysmem.c',
    'Src/syscalls.c'
)

$newBlock = @('C_SOURCES =  \')
for ($i = 0; $i -lt $cSources.Count; $i++) {
    $suffix = if ($i -lt ($cSources.Count - 1)) { ' \' } else { '' }
    $newBlock += "$($cSources[$i])$suffix"
}

$before = if ($start -gt 0) { $lines[0..($start - 1)] } else { @() }
$after = $lines[$asmIndex..($lines.Count - 1)]
Set-Content -LiteralPath $makefile -Value ($before + $newBlock + '' + $after) -Encoding ascii

$env:PATH = "$GccBin;$env:PATH"
& $MakeExe -C $ProjectDir -j4
if ($LASTEXITCODE -ne 0) {
    throw "make failed with exit code $LASTEXITCODE"
}
