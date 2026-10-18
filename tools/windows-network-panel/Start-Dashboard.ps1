[CmdletBinding()]
param(
    [Parameter(Position=0)][string]$ClientPath,
    [Parameter(Position=1)][ValidateRange(1, 65535)][int]$WebPort = 8888,
    [Parameter(Position=2)][ValidateRange(1, 65535)][int]$BridgePort = 18889,
    [switch]$Check
)
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSEdition -ne 'Desktop') { throw 'Use Windows PowerShell 5.1 or Start-Dashboard.cmd.' }
if (-not $ClientPath) {
    # Release layout: <root>/tools/windows-network-panel/Start-Dashboard.ps1
    $root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
    $clients = @(Get-ChildItem -LiteralPath $root -Filter 'ESurfingClient*.exe' -File)
    if ($clients.Count -ne 1) { throw 'Pass one explicit client path: Start-Dashboard.cmd "C:\ESurfingClient\ESurfingClient.exe"' }
    $ClientPath = $clients[0].FullName
}
$ClientPath = (Resolve-Path -LiteralPath $ClientPath).Path
if ([IO.Path]::GetExtension($ClientPath) -ine '.exe') { throw 'ClientPath must point to the Windows client executable.' }
$assets = Join-Path ([IO.Path]::GetDirectoryName($ClientPath)) 'portal\assets'
if (-not (Test-Path -LiteralPath $assets -PathType Container)) { throw 'The client portal/assets directory is missing. Extract the full Windows release archive first.' }
if ($Check) {
    return [pscustomobject]@{ ClientPath = $ClientPath; WebPort = $WebPort; BridgePort = $BridgePort; PortalPath = $assets }
}
Write-Host "Dashboard: http://127.0.0.1:$WebPort/index.html or http://localhost:$WebPort/index.html"
& "$PSScriptRoot\TrafficCollector.ps1" -ClientPath $ClientPath -StartClient -HotspotControls -WebPort $WebPort -BridgePort $BridgePort
