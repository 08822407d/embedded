$ErrorActionPreference = "Continue"

function Resolve-AdbPath {
  $candidates = @()

  if ($env:ADB_BIN) {
    $candidates += [PSCustomObject]@{ Path = $env:ADB_BIN; Source = "ADB_BIN" }
  }

  $pathCmd = Get-Command adb -ErrorAction SilentlyContinue
  if ($pathCmd) {
    $candidates += [PSCustomObject]@{ Path = $pathCmd.Source; Source = "PATH" }
  }

  $common = @()
  if ($env:LOCALAPPDATA) {
    $common += Join-Path $env:LOCALAPPDATA "Android\Sdk\platform-tools\adb.exe"
  }
  if ($env:ProgramFiles) {
    $common += Join-Path $env:ProgramFiles "Android\android-sdk\platform-tools\adb.exe"
  }
  if (${env:ProgramFiles(x86)}) {
    $common += Join-Path ${env:ProgramFiles(x86)} "Android\android-sdk\platform-tools\adb.exe"
  }
  if ($env:ANDROID_HOME) {
    $common += Join-Path $env:ANDROID_HOME "platform-tools\adb.exe"
  }
  if ($env:ANDROID_SDK_ROOT) {
    $common += Join-Path $env:ANDROID_SDK_ROOT "platform-tools\adb.exe"
  }

  foreach ($path in $common) {
    if ($path) {
      $candidates += [PSCustomObject]@{ Path = $path; Source = "common-path" }
    }
  }

  foreach ($candidate in $candidates) {
    $cmd = Get-Command $candidate.Path -ErrorAction SilentlyContinue
    if ($cmd) {
      return [PSCustomObject]@{
        Found = $true
        Path = $cmd.Source
        Source = $candidate.Source
        RecommendedEnv = "`$env:ADB_BIN=`"$($cmd.Source)`""
      }
    }
    if (Test-Path -LiteralPath $candidate.Path -PathType Leaf) {
      $resolved = (Resolve-Path -LiteralPath $candidate.Path).Path
      return [PSCustomObject]@{
        Found = $true
        Path = $resolved
        Source = $candidate.Source
        RecommendedEnv = "`$env:ADB_BIN=`"$resolved`""
      }
    }
  }

  return [PSCustomObject]@{
    Found = $false
    Path = "adb"
    Source = "not-found"
    RecommendedEnv = $null
  }
}

New-Item -ItemType Directory -Force "inventory\before" | Out-Null
$ts = Get-Date -Format "yyyyMMdd-HHmmss"
$out = "inventory\before\host-tools-$ts.txt"
$AdbInfo = Resolve-AdbPath
$Adb = $AdbInfo.Path

& {
  "## Host tool check"
  Get-Date
  ""
  "## OS"
  Get-CimInstance Win32_OperatingSystem | Select-Object Caption, Version, BuildNumber, OSArchitecture | Format-List | Out-String
  "## PowerShell"
  $PSVersionTable | Out-String
  "## adb path"
  if ($AdbInfo.Found) {
    $AdbInfo.Path
    "source: $($AdbInfo.Source)"
    if (-not $env:ADB_BIN -or $env:ADB_BIN -ne $AdbInfo.Path) {
      "recommended env: $($AdbInfo.RecommendedEnv)"
    }
  } else {
    "ADB_NOT_FOUND"
    "ACTION_REQUIRED: ask user whether to install Android SDK Platform Tools, or set ADB_BIN to an existing adb.exe."
  }
  ""
  "## adb version"
  if ($AdbInfo.Found) { try { & $Adb version 2>&1 | Out-String } catch { $_ | Out-String } } else { "SKIPPED: adb not found" }
  ""
  "## adb start-server"
  if ($AdbInfo.Found) { try { & $Adb start-server 2>&1 | Out-String } catch { $_ | Out-String } } else { "SKIPPED: adb not found" }
  ""
  "## adb devices -l"
  if ($AdbInfo.Found) { try { & $Adb devices -l 2>&1 | Out-String } catch { $_ | Out-String } } else { "SKIPPED: adb not found" }
  ""
  "## ssh"
  try { & ssh -V 2>&1 | Out-String } catch { $_ | Out-String }
  ""
  "## USB inventory sample"
  try {
    Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match 'USB' } |
      Select-Object -First 50 Status,Class,FriendlyName,InstanceId | Format-Table -AutoSize | Out-String
  } catch { $_ | Out-String }
} | Tee-Object -FilePath $out

Write-Host "Saved: $out"
