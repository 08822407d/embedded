[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot,

    [Parameter(Mandatory = $true)]
    [string]$PlatformIo,

    [Parameter(Mandatory = $true)]
    [string]$ToolchainBin,

    [Parameter(Mandatory = $true)]
    [string]$Environment,

    [Parameter(Mandatory = $true)]
    [int]$Jobs,

    [Parameter(Mandatory = $true)]
    [string]$OutputLog,

    [Parameter(Mandatory = $true)]
    [string]$ErrorLog,

    [Parameter(Mandatory = $true)]
    [string]$ResultFile
)

$ErrorActionPreference = "Stop"
$started = Get-Date
$exitCode = 1

try {
    $env:PATH = "$ToolchainBin;$env:PATH"
    Push-Location $ProjectRoot
    try {
        $process = Start-Process `
            -FilePath $PlatformIo `
            -ArgumentList @("run", "-e", $Environment, "-j", $Jobs) `
            -WorkingDirectory $ProjectRoot `
            -NoNewWindow `
            -RedirectStandardOutput $OutputLog `
            -RedirectStandardError $ErrorLog `
            -Wait `
            -PassThru
        $exitCode = $process.ExitCode
    }
    finally {
        Pop-Location
    }
}
catch {
    $_ | Out-String | Add-Content -LiteralPath $ErrorLog
    $exitCode = 1
}
finally {
    $result = [ordered]@{
        environment = $Environment
        exit_code = $exitCode
        started_at = $started.ToString("o")
        completed_at = (Get-Date).ToString("o")
        elapsed_seconds = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)
        output_log = $OutputLog
        error_log = $ErrorLog
    }
    $result | ConvertTo-Json | Set-Content -LiteralPath $ResultFile -Encoding utf8
}

exit $exitCode
