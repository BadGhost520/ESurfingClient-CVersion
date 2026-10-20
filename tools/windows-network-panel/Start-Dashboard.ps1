[CmdletBinding()]
param(
    [Parameter(Position=0)][string]$ClientPath,
    [Parameter(Position=1)][ValidateRange(1, 65535)][int]$WebPort = 8888,
    [Parameter(Position=2)][ValidateRange(1, 65535)][int]$BridgePort = 18889,
    [switch]$Check
)
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSEdition -ne 'Desktop') { throw '请用 Windows PowerShell 5.1 运行本脚本 (双击 Start-Dashboard.cmd 即可)。' }
if (-not $ClientPath) {
    # 没指定客户端时自动定位, 依次尝试两种布局:
    #   1. 发布包布局  <包根>\tools\windows-network-panel\Start-Dashboard.ps1, 客户端在 <包根>
    #   2. 临时布局    把本目录整个复制到客户端旁边, 即 <客户端目录>\windows-network-panel\
    $roots = @(
        [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..')),
        [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
    )
    $searched = @()
    $picked = $null
    $ambiguous = 0
    foreach ($root in $roots) {
        $searched += (Join-Path $root 'ESurfingClient*.exe')
        $clients = @(Get-ChildItem -LiteralPath $root -Filter 'ESurfingClient*.exe' -File -ErrorAction SilentlyContinue)
        if ($clients.Count -eq 1) { $picked = $clients[0].FullName; break }
        if ($clients.Count -gt 1 -and $ambiguous -eq 0) { $ambiguous = $clients.Count }
    }
    if (-not $picked) {
        throw ('没找到唯一的校园网客户端 (找过: {0}; 共找到 {1} 个)。请显式指定客户端路径, 例如: Start-Dashboard.cmd "C:\ESurfingClient\ESurfingClient.exe"' -f ($searched -join '; '), $ambiguous)
    }
    $ClientPath = $picked
}
$ClientPath = (Resolve-Path -LiteralPath $ClientPath).Path
if ([IO.Path]::GetExtension($ClientPath) -ine '.exe') { throw ('传进来的路径不是 .exe 文件: {0}' -f $ClientPath) }
$assets = Join-Path ([IO.Path]::GetDirectoryName($ClientPath)) 'portal\assets'
if (-not (Test-Path -LiteralPath $assets -PathType Container)) { throw ('客户端旁边没有 portal\assets 目录 ({0})。如果是从压缩包里直接双击运行的, 请先完整解压。' -f $assets) }
if ($Check) {
    return [pscustomobject]@{ ClientPath = $ClientPath; WebPort = $WebPort; BridgePort = $BridgePort; PortalPath = $assets }
}
Write-Host "Dashboard: http://127.0.0.1:$WebPort/index.html or http://localhost:$WebPort/index.html"
& "$PSScriptRoot\TrafficCollector.ps1" -ClientPath $ClientPath -StartClient -HotspotControls -WebPort $WebPort -BridgePort $BridgePort
