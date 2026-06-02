param(
  [string]$Serial
)

$ErrorActionPreference = "Stop"

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
    Path = $null
    Source = "not-found"
    RecommendedEnv = $null
  }
}

function Resolve-AdbSerial {
  param(
    [string]$SerialArg,
    [string]$Adb
  )

  $cmd = Get-Command $Adb -ErrorAction SilentlyContinue
  if (-not $cmd) {
    throw "adb not found. Ask user whether to install Android SDK Platform Tools, or set ADB_BIN to an existing adb.exe."
  }

  & $Adb version | Out-Null
  & $Adb start-server | Out-Null
  & $Adb devices -l | Out-Null

  $requested = $SerialArg
  if (-not $requested) { $requested = $env:ADB_SERIAL }
  if (-not $requested) { $requested = $env:ANDROID_SERIAL }

  $lines = & $Adb devices
  $devices = @()
  foreach ($line in $lines) {
    if ($line -match '^(\S+)\s+device(\s|$)') {
      $devices += $Matches[1]
    }
  }

  if ($devices.Count -eq 0) {
    throw "no adb device in 'device' state. Check USB-C data cable, port, driver/permissions, and power."
  }

  if ($requested) {
    if ($devices -contains $requested) {
      return $requested
    }
    throw "requested serial '$requested' is not in current adb device list: $($devices -join ', ')"
  }

  if ($devices.Count -eq 1) {
    return $devices[0]
  }

  throw "multiple adb devices found: $($devices -join ', '). Set `$env:ADB_SERIAL='<serial>' or pass -Serial <serial>."
}

New-Item -ItemType Directory -Force "backups", "inventory\before" | Out-Null
$ts = Get-Date -Format "yyyyMMdd-HHmmss"
$AdbInfo = Resolve-AdbPath
if (-not $AdbInfo.Found) {
  throw "adb not found. Ask user whether to install Android SDK Platform Tools, or set ADB_BIN to an existing adb.exe."
}
$Adb = $AdbInfo.Path
$SelectedSerial = Resolve-AdbSerial -SerialArg $Serial -Adb $Adb
$remote = "/tmp/etc-apt-backup-$ts.tgz"
$local = "backups\etc-apt-backup-$ts.tgz"
$log = "inventory\before\etc-apt-backup-adb-$ts.txt"

& {
  "## Host timestamp"
  Get-Date
  ""
  "## adb path"
  $Adb
  "source: $($AdbInfo.Source)"
  if (-not $env:ADB_BIN -or $env:ADB_BIN -ne $Adb) {
    "recommended env: $($AdbInfo.RecommendedEnv)"
  }
  ""
  "## adb devices -l"
  & $Adb devices -l
  ""
  "## selected serial"
  $SelectedSerial
  ""
  "## create remote tar"
  & $Adb -s $SelectedSerial shell "tar -czf '$remote' /etc/apt 2>/dev/null || true"
  ""
  "## pull"
  & $Adb -s $SelectedSerial pull $remote $local
  ""
  "## cleanup"
  & $Adb -s $SelectedSerial shell "rm -f '$remote'" 2>&1
  ""
  "Saved backup: $local"
} | Tee-Object -FilePath $log

Write-Host "Saved log: $log"
