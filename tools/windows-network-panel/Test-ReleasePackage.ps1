[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Root,
      [Parameter(Mandatory=$true)][ValidateSet('x86','x86_64','aarch64')][string]$Architecture)
$ErrorActionPreference = 'Stop'
$required = @('Start-Dashboard.cmd','ESurfingClient.json','portal/index.html',
    'portal/assets/js/traffic.js','portal/assets/js/hotspot.js',
    'tools/windows-network-panel/Start-Dashboard.ps1','tools/windows-network-panel/TrafficCollector.ps1',
    'tools/windows-network-panel/HotspotBridge.ps1','tools/windows-network-panel/NetworkPanel.ps1',
    'tools/windows-network-panel/Get-ValidationEnvironment.ps1','tools/windows-network-panel/HARDWARE-VALIDATION.md')
foreach ($file in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $Root $file) -PathType Leaf)) { throw "Missing package file: $file" }
}
$clients = @(Get-ChildItem -LiteralPath $Root -Filter 'ESurfingClient*.exe')
if ($clients.Count -ne 1) { throw 'Expected one client executable' }
$bytes = [IO.File]::ReadAllBytes($clients[0].FullName)
if ($bytes.Length -lt 64 -or [BitConverter]::ToUInt16($bytes,0) -ne 0x5a4d) { throw 'Invalid DOS header' }
$pe = [BitConverter]::ToInt32($bytes,0x3c)
if ($pe -lt 0 -or $pe -gt $bytes.Length - 6 -or [BitConverter]::ToUInt32($bytes,$pe) -ne 0x4550) { throw 'Invalid PE header' }
$machines = @{x86=0x14c; x86_64=0x8664; aarch64=0xaa64}
if ([BitConverter]::ToUInt16($bytes,$pe+4) -ne $machines[$Architecture]) { throw 'Wrong executable architecture' }
$html = Get-Content -LiteralPath (Join-Path $Root 'portal/index.html') -Raw
if ($html -match '(?i)<(?:script|link)\b[^>]*(?:src|href)\s*=\s*["''](?:https?:)?//') { throw 'Portal requires an external asset' }
foreach ($match in [regex]::Matches($html,'(?i)(?:src|href)\s*=\s*["''](assets/[^"''?#]+)')) {
    if (-not (Test-Path -LiteralPath (Join-Path (Join-Path $Root 'portal') $match.Groups[1].Value))) { throw 'Missing local portal asset' }
}
$null = & (Join-Path $Root 'tools/windows-network-panel/Start-Dashboard.ps1') -Check
$rejected = $false
try { $null = & (Join-Path $Root 'tools/windows-network-panel/Start-Dashboard.ps1') -Check -WebPort 0 } catch { $rejected = $true }
if (-not $rejected) { throw 'Launcher accepted invalid port' }
Write-Output "PASS: $Architecture PE, package files, offline portal assets and launcher discovery; client not started"
