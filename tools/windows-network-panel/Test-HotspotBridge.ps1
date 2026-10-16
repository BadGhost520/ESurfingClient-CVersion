$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\HotspotBridge.ps1"
$script:bridgeOrigin = 'http://127.0.0.1:8888'
$script:bridgeToken = 'test-token'
function Assert-Access($Expected, $Origin, $Address, $Method, $Token, $Body) {
    $actual = Test-BridgeAccess $Origin $Address $Method $Token $Body
    if ($actual -ne $Expected) { throw "Unexpected access for $Origin / $Method" }
}
Assert-Access $true $script:bridgeOrigin '127.0.0.1' 'GET' '' $false
Assert-Access $true $script:bridgeOrigin '127.0.0.1' 'OPTIONS' '' $false
Assert-Access $true $script:bridgeOrigin '127.0.0.1' 'POST' 'test-token' $false
Assert-Access $false 'https://example.com' '127.0.0.1' 'POST' 'test-token' $false
Assert-Access $false '' '127.0.0.1' 'GET' '' $false
Assert-Access $false $script:bridgeOrigin '192.168.1.2' 'POST' 'test-token' $false
Assert-Access $false $script:bridgeOrigin '127.0.0.1' 'POST' 'wrong' $false
Assert-Access $false $script:bridgeOrigin '127.0.0.1' 'DELETE' 'test-token' $false
Assert-Access $false $script:bridgeOrigin '127.0.0.1' 'POST' 'test-token' $true
Write-Output 'PASS: 9 hotspot bridge access checks; no network mutations.'
