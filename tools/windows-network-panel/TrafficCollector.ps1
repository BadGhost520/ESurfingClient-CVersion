[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$ClientPath,
    [switch]$StartClient,
    [switch]$HotspotControls,
    [ValidateRange(1, 65535)][int]$WebPort = 8888,
    [ValidateRange(1, 65535)][int]$BridgePort = 18889,
    [ValidateRange(0, 2147483647)][int]$Samples = 0,
    [string]$OutputDirectory,
    [string]$StopFile
)
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSEdition -ne 'Desktop') { throw '请用 Windows PowerShell 5.1 运行本脚本 (Start-Dashboard.ps1 会自己选对解释器)。' }

# 输出被重定向到管道时 (主程序会逐行转发进自己的日志文件) 统一按 UTF-8 输出,
# 否则中文到了日志里会变成问号; 直接在控制台里跑时保持控制台编码, 免得显示乱码
if ([Console]::IsOutputRedirected) { [Console]::OutputEncoding = [Text.Encoding]::UTF8 }

# 生命周期日志: 主程序会把本脚本的 stdout 逐行转发进它自己的日志文件, 所以这里必须用
# [Console]::Out (Write-Host / Write-Warning 在无控制台的宿主里可能丢失) 并逐行 flush
function Write-Log([string]$Message) {
    [Console]::Out.WriteLine('[dashboard] ' + $Message)
    [Console]::Out.Flush()
}

# 主程序关闭时会先创建这个停止信号文件; 本脚本看到它就收尾退出, finally 照常清理
function Test-StopRequested {
    if (-not $StopFile) { return $false }
    return (Test-Path -LiteralPath $StopFile)
}

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
    if (-not $owned) {
        Write-Log '已有另一个采集实例在运行, 本实例退出'
        return
    }
    if (-not (Find-Client)) {
        if (-not $StartClient) { throw '校园网客户端未运行。' }
        Start-Process -FilePath $ClientPath -WorkingDirectory ([IO.Path]::GetDirectoryName($ClientPath)) -WindowStyle Hidden
    }

    Write-Log ('采集已启动: 客户端=' + $ClientPath + ', 输出=' + $output)
    [Windows.Networking.Connectivity.NetworkInformation,Windows.Networking.Connectivity,ContentType=WindowsRuntime] > $null
    $bridgeInfo = $null
    if (-not $HotspotControls) { Write-Log '热点控制: 未启用 (只采集网速)' }
    if ($HotspotControls) {
        . "$PSScriptRoot\HotspotBridge.ps1"
        $bridgeInfo = @{ url = "http://127.0.0.1:$BridgePort"; origins = @(Get-BridgeOrigins $WebPort); ready = $false; code = ''; message = '' }
        try {
            Start-HotspotBridge -WebPort $WebPort -BridgePort $BridgePort
            $bridgeInfo.ready = $true
            Write-Log ('热点桥接已启动: http://127.0.0.1:' + $BridgePort + ' (允许来源 http://127.0.0.1:' + $WebPort + ' 与 http://localhost:' + $WebPort + ')')
        } catch {
            $failure = Get-BridgeFailure $_.Exception.GetBaseException().NativeErrorCode
            $bridgeInfo.code = $failure.code
            $bridgeInfo.message = $failure.message
            Stop-HotspotBridge
            Write-Log ('热点控制未启用: ' + $failure.message)
        }
    }
    $clock = [Diagnostics.Stopwatch]::StartNew()
    $previous = @{}
    $iteration = 0
    $exitReason = '客户端进程已退出'
    while (Find-Client) {
        if (Test-StopRequested) {
            $exitReason = '收到主程序的停止请求'
            break
        }
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
        if ($Samples -gt 0 -and $iteration -ge $Samples) {
            $exitReason = '已达到指定采样次数'
            break
        }
        for ($poll = 0; $poll -lt 10; $poll++) {
            if ($HotspotControls) { Update-HotspotBridgeAvailability $bridgeInfo }
            Start-Sleep -Milliseconds 100
        }
    }

    Write-Log ('采集结束: ' + $exitReason)
} finally {
    if ($StopFile -and (Test-Path -LiteralPath $StopFile)) { Remove-Item -LiteralPath $StopFile -Force -ErrorAction SilentlyContinue }
    if ($HotspotControls -and (Get-Command Stop-HotspotBridge -ErrorAction SilentlyContinue)) { Stop-HotspotBridge }
    if ([IO.File]::Exists($temporary)) { [IO.File]::Delete($temporary) }
    if ($owned) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
}
