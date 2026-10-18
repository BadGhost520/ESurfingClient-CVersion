# Optional local bridge. Loading this file alone has no network side effects.
function Get-BridgeOrigins([ValidateRange(1, 65535)][int]$WebPort = 8888) {
    foreach ($name in @('127.0.0.1', 'localhost')) { ([Uri]("http://${name}:$WebPort")).GetLeftPart([UriPartial]::Authority) }
}

function Get-BridgeFailure([int]$ErrorCode) {
    if ($ErrorCode -eq 5) {
        return @{ code = 'AccessDenied'; message = 'Windows 拒绝监听权限。网速仍可使用，请使用系统热点设置或独立热点窗口。' }
    }
    if ($ErrorCode -in @(32, 183)) {
        return @{ code = 'PortUnavailable'; message = '热点控制端口已被占用，请使用其他 BridgePort 后重新启动辅助程序。' }
    }
    return @{ code = 'Unavailable'; message = '热点控制启动失败。网速仍可使用，请检查端口或使用系统热点设置。' }
}

function Test-BridgeAccess($Origin, $Address, $Method, $Token, [bool]$HasBody) {
    if ($script:bridgeOrigins -cnotcontains $Origin -or $HasBody) { return $false }
    $parsedAddress = $null
    if (-not [Net.IPAddress]::TryParse($Address, [ref]$parsedAddress) -or -not [Net.IPAddress]::IsLoopback($parsedAddress)) { return $false }
    if ($Method -eq 'GET' -or $Method -eq 'OPTIONS') { return $true }
    return ($Method -eq 'POST' -and $Token -ceq $script:bridgeToken)
}

function Start-HotspotBridge {
    param([ValidateRange(1, 65535)][int]$WebPort = 8888, [ValidateRange(1, 65535)][int]$BridgePort = 18889)
    Add-Type -AssemblyName System.Runtime.WindowsRuntime
    [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager,Windows.Networking.NetworkOperators,ContentType=WindowsRuntime] > $null
    [Windows.Networking.NetworkOperators.NetworkOperatorTetheringOperationResult,Windows.Networking.NetworkOperators,ContentType=WindowsRuntime] > $null
    $method = [System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
        $_.Name -eq 'AsTask' -and $_.IsGenericMethod -and $_.GetGenericArguments().Count -eq 1 -and
        $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1'
    } | Select-Object -First 1
    $script:bridgeAsTask = $method.MakeGenericMethod([Windows.Networking.NetworkOperators.NetworkOperatorTetheringOperationResult])
    $script:bridgeOrigins = @(Get-BridgeOrigins $WebPort)
    $script:bridgeToken = [Guid]::NewGuid().ToString('N')
    $script:bridgeEntries = @{}
    $script:bridgeTask = $null
    $script:bridgeMessage = ''
    $script:bridgeListener = New-Object Net.HttpListener
    $script:bridgeListener.Prefixes.Add("http://127.0.0.1:$BridgePort/")
    $script:bridgeListener.Start()
    $script:bridgeRequest = $script:bridgeListener.GetContextAsync()
}

function Get-HotspotSnapshot {
    $profiles = @{}
    foreach ($profile in [Windows.Networking.Connectivity.NetworkInformation]::GetConnectionProfiles()) {
        if ($profile.NetworkAdapter) { $profiles[$profile.NetworkAdapter.NetworkAdapterId.ToString()] = $profile }
    }
    foreach ($entry in $script:bridgeEntries.Values) { $entry.connected = $false }
    foreach ($adapter in [Net.NetworkInformation.NetworkInterface]::GetAllNetworkInterfaces()) {
        $id = $adapter.Id.Trim('{}').ToLowerInvariant()
        if ($adapter.OperationalStatus -ne 'Up' -or -not $profiles.ContainsKey($id)) { continue }
        try {
            $profile = $profiles[$id]
            $manager = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager]::CreateFromConnectionProfile($profile)
            $capability = [string][Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager]::GetTetheringCapabilityFromConnectionProfile($profile)
            $script:bridgeEntries[$id] = @{ manager = $manager; name = $adapter.Name; connected = $true; capability = $capability }
        } catch { } # Unsupported adapters do not prevent other adapters from working.
    }
    $rows = @(foreach ($id in @($script:bridgeEntries.Keys)) {
        $entry = $script:bridgeEntries[$id]
        try {
            $state = [string]$entry.manager.TetheringOperationalState
            if (-not $entry.connected -and $state -eq 'Off') { $script:bridgeEntries.Remove($id); continue }
            @{ id = $id; name = $entry.name; state = $state; capability = $entry.capability
               connected = $entry.connected; ssid = $entry.manager.GetCurrentAccessPointConfiguration().Ssid }
        } catch { $script:bridgeEntries.Remove($id) }
    })
    $internet = [Windows.Networking.Connectivity.NetworkInformation]::GetInternetConnectionProfile()
    $default = if ($internet -and $internet.NetworkAdapter) { $internet.NetworkAdapter.NetworkAdapterId.ToString() } else { '' }
    return @{ adapters = $rows; defaultId = $default; token = $script:bridgeToken
              busy = ($null -ne $script:bridgeTask); message = $script:bridgeMessage }
}

function Send-BridgeResponse($Context, [int]$Status, $Data) {
    $Context.Response.StatusCode = $Status
    $Context.Response.ContentType = 'application/json; charset=utf-8'
    $Context.Response.Headers['Cache-Control'] = 'no-store'
    $Context.Response.Headers['X-Content-Type-Options'] = 'nosniff'
    $bytes = [Text.Encoding]::UTF8.GetBytes(($Data | ConvertTo-Json -Depth 5 -Compress))
    $Context.Response.ContentLength64 = $bytes.Length
    try { $Context.Response.OutputStream.Write($bytes, 0, $bytes.Length) }
    finally { $Context.Response.Close() }
}

function Poll-HotspotBridge {
    if ($null -eq $script:bridgeListener -or -not $script:bridgeListener.IsListening) { return }
    if ($null -ne $script:bridgeTask -and $script:bridgeTask.IsCompleted) {
        try {
            $result = $script:bridgeTask.GetAwaiter().GetResult()
            $script:bridgeMessage = if ([string]$result.Status -eq 'Success') { '热点操作成功。' }
                else { 'Windows 未完成操作：' + [string]$result.Status + '。请打开热点设置检查。' }
        } catch { $script:bridgeMessage = '热点操作失败，请打开 Windows 热点设置检查。' }
        $script:bridgeTask = $null
    }
    if (-not $script:bridgeRequest.IsCompleted) { return }
    $context = $script:bridgeRequest.GetAwaiter().GetResult()
    $script:bridgeRequest = $script:bridgeListener.GetContextAsync()
    try {
        $request = $context.Request
        if (-not (Test-BridgeAccess $request.Headers['Origin'] $request.RemoteEndPoint.Address.ToString() $request.HttpMethod $request.Headers['X-ESurfing-Token'] $request.HasEntityBody)) {
            Send-BridgeResponse $context 403 @{ message = '只允许本机校园网页面操作。' }; return
        }
        $context.Response.Headers['Access-Control-Allow-Origin'] = $request.Headers['Origin']
        $context.Response.Headers['Vary'] = 'Origin'
        $context.Response.Headers['Access-Control-Allow-Methods'] = 'GET, POST, OPTIONS'
        $context.Response.Headers['Access-Control-Allow-Headers'] = 'X-ESurfing-Token'
        $path = $request.Url.AbsolutePath
        if ($path -notin @('/status', '/start', '/stop')) { Send-BridgeResponse $context 404 @{}; return }
        if ($request.HttpMethod -eq 'OPTIONS') { Send-BridgeResponse $context 200 @{}; return }
        if ($request.HttpMethod -eq 'GET' -and $path -eq '/status') {
            Send-BridgeResponse $context 200 (Get-HotspotSnapshot); return
        }
        if ($request.HttpMethod -ne 'POST' -or $path -eq '/status') { Send-BridgeResponse $context 405 @{}; return }
        if ($null -ne $script:bridgeTask) { Send-BridgeResponse $context 409 @{ message = '热点正在切换，请稍候。' }; return }
        $snapshot = Get-HotspotSnapshot
        $id = $request.QueryString['adapter']
        $row = $snapshot.adapters | Where-Object { $_.id -eq $id } | Select-Object -First 1
        if (-not $row) { Send-BridgeResponse $context 400 @{ message = '所选网卡不可用，请刷新页面。' }; return }
        if ($path -eq '/start' -and ($row.state -ne 'Off' -or $row.capability -ne 'Enabled' -or -not $row.connected -or
            @($snapshot.adapters | Where-Object { $_.state -in @('On', 'InTransition') }).Count -gt 0)) {
            Send-BridgeResponse $context 409 @{ message = '当前状态不能开启热点，请检查已有热点或网卡。' }; return
        }
        if ($path -eq '/stop' -and $row.state -ne 'On') { Send-BridgeResponse $context 409 @{ message = '热点未开启或正在切换。' }; return }
        $manager = $script:bridgeEntries[$id].manager
        # The only network mutations, after loopback/origin/token/state validation.
        $operation = if ($path -eq '/start') { $manager.StartTetheringAsync() } else { $manager.StopTetheringAsync() }
        $script:bridgeTask = $script:bridgeAsTask.Invoke($null, @($operation))
        $script:bridgeMessage = 'Windows 正在切换热点，请稍候。'
        Send-BridgeResponse $context 202 @{ message = $script:bridgeMessage }
    } catch {
        try { Send-BridgeResponse $context 500 @{ message = '热点操作未完成，请打开 Windows 热点设置检查。' } } catch { }
    }
}

function Stop-HotspotBridge {
    if ($null -ne $script:bridgeListener) { $script:bridgeListener.Close() }
    # Do not stop tethering: closing the helper preserves the user's hotspot state.
}
