[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$ClientPath,
    [switch]$StartClient,
    [switch]$HotspotControls,
    [ValidateRange(1, 65535)][int]$WebPort = 8888,
    [ValidateRange(1, 65535)][int]$BridgePort = 18889,
    [ValidateRange(0, 2147483647)][int]$Samples = 0,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSEdition -ne 'Desktop') { throw 'Use Windows PowerShell 5.1 to run TrafficCollector.ps1.' }
. "$PSScriptRoot\NetworkPanel.ps1" -LibraryOnly
$ClientPath = (Resolve-Path -LiteralPath $ClientPath).Path
$clientName = [IO.Path]::GetFileNameWithoutExtension($ClientPath)
function Find-Client {
    return Get-Process -Name $clientName -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $ClientPath }
}
if (-not $OutputDirectory) { $OutputDirectory = Join-Path ([IO.Path]::GetDirectoryName($ClientPath)) 'portal\assets' }
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
$output = Join-Path $OutputDirectory 'traffic.json'
$hash = [Security.Cryptography.SHA256]::Create()
try { $key = [BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes($output.ToLowerInvariant()))).Replace('-', '') }
finally { $hash.Dispose() }
$mutex = New-Object Threading.Mutex($false, ('Local\ESurfingTraffic-' + $key))
$owned = $false
$temporary = $output + '.' + $PID + '.tmp'
try {
    try { $owned = $mutex.WaitOne(0) } catch [Threading.AbandonedMutexException] { $owned = $true }
    if (-not $owned) { return }
    if (-not (Find-Client)) {
        if (-not $StartClient) { throw '校园网客户端未运行。' }
        Start-Process -FilePath $ClientPath -WorkingDirectory ([IO.Path]::GetDirectoryName($ClientPath)) -WindowStyle Hidden
    }
    [Windows.Networking.Connectivity.NetworkInformation,Windows.Networking.Connectivity,ContentType=WindowsRuntime] > $null
    $bridgeInfo = $null
    if ($HotspotControls) {
        . "$PSScriptRoot\HotspotBridge.ps1"
        $bridgeInfo = @{ url = "http://127.0.0.1:$BridgePort"; origins = @(Get-BridgeOrigins $WebPort); ready = $false; code = ''; message = '' }
        try {
            Start-HotspotBridge -WebPort $WebPort -BridgePort $BridgePort
            $bridgeInfo.ready = $true
        } catch {
            $failure = Get-BridgeFailure $_.Exception.GetBaseException().NativeErrorCode
            $bridgeInfo.code = $failure.code
            $bridgeInfo.message = $failure.message
            Stop-HotspotBridge
            Write-Warning $failure.message
        }
    }
    $clock = [Diagnostics.Stopwatch]::StartNew()
    $previous = @{}
    $iteration = 0
    while (Find-Client) {
        $profiles = @{}
        foreach ($profile in [Windows.Networking.Connectivity.NetworkInformation]::GetConnectionProfiles()) {
            if ($profile.NetworkAdapter) { $profiles[$profile.NetworkAdapter.NetworkAdapterId.ToString()] = $true }
        }
        $internet = [Windows.Networking.Connectivity.NetworkInformation]::GetInternetConnectionProfile()
        $defaultId = if ($internet -and $internet.NetworkAdapter) { $internet.NetworkAdapter.NetworkAdapterId.ToString() } else { '' }
        $next = @{}
        $rows = @([Net.NetworkInformation.NetworkInterface]::GetAllNetworkInterfaces() | ForEach-Object {
            $id = $_.Id.Trim('{}').ToLowerInvariant()
            if ($_.OperationalStatus -eq 'Up' -and $profiles.ContainsKey($id)) {
                try {
                    $stats = $_.GetIPStatistics()
                    $sample = @{ Received = $stats.BytesReceived; Sent = $stats.BytesSent; Time = $clock.Elapsed.TotalSeconds }
                    $rate = Get-TrafficRate $previous[$id] $sample
                    $next[$id] = $sample
                    @{ id = $id; name = $_.Name; down = $(if ($rate) { $rate.Down } else { $null }); up = $(if ($rate) { $rate.Up } else { $null }) }
                } catch { } # Adapter may disappear between enumeration and sampling.
            }
        })
        $previous = $next
        $json = @{ timestamp = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds(); defaultId = $defaultId; adapters = $rows; hotspotControls = [bool]$HotspotControls; hotspotBridge = $bridgeInfo } | ConvertTo-Json -Depth 4 -Compress
        [IO.File]::WriteAllText($temporary, $json, (New-Object Text.UTF8Encoding($false)))
        try {
            if ([IO.File]::Exists($output)) { [IO.File]::Replace($temporary, $output, [NullString]::Value) }
            else { [IO.File]::Move($temporary, $output) }
        } catch [IO.IOException] { } # A simultaneous static-file read can briefly hold the file; retry next sample.
        $iteration++
        if ($Samples -gt 0 -and $iteration -ge $Samples) { break }
        for ($poll = 0; $poll -lt 10; $poll++) {
            if ($HotspotControls) { Poll-HotspotBridge }
            Start-Sleep -Milliseconds 100
        }
    }
} finally {
    if ($HotspotControls -and (Get-Command Stop-HotspotBridge -ErrorAction SilentlyContinue)) { Stop-HotspotBridge }
    if ([IO.File]::Exists($temporary)) { [IO.File]::Delete($temporary) }
    if ($owned) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
}
