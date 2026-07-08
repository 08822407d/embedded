param(
    [string]$PortName = 'COM4',
    [int]$BaudRate = 115200,
    [int]$Seconds = 60,
    [switch]$ResetCounters
)

$ErrorActionPreference = 'Stop'

$serial = [System.IO.Ports.SerialPort]::new($PortName, $BaudRate, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$serial.ReadTimeout = 500
$serial.NewLine = "`n"

try {
    $serial.Open()
    Write-Host "Opened $PortName at $BaudRate baud for $Seconds seconds. Press Ctrl+C to stop."
    if ($ResetCounters) {
        Start-Sleep -Milliseconds 200
        $serial.Write('r')
        Write-Host "Sent counter reset command."
    }
    $deadline = [DateTime]::UtcNow.AddSeconds($Seconds)

    while ([DateTime]::UtcNow -lt $deadline) {
        try {
            $line = $serial.ReadLine()
            Write-Host $line.TrimEnd("`r", "`n")
        } catch [System.TimeoutException] {
        }
    }
} finally {
    if ($serial.IsOpen) {
        $serial.Close()
    }
}
