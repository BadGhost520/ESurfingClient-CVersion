$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\HotspotBridge.ps1"
$script:bridgeOrigins = @(Get-BridgeOrigins 9999)
$script:bridgeToken = 'test-token'
function Assert-Access($Expected, $Origin, $Address, $Method, $Token, $Body) {
    $actual = Test-BridgeAccess $Origin $Address $Method $Token $Body
    if ($actual -ne $Expected) { throw "Unexpected access for $Origin / $Method" }
}
$origin = 'http://127.0.0.1:9999'
Assert-Access $true $origin '127.0.0.1' 'GET' '' $false
Assert-Access $true $origin '127.0.0.1' 'OPTIONS' '' $false
Assert-Access $true $origin '127.0.0.1' 'POST' 'test-token' $false
Assert-Access $false 'https://example.com' '127.0.0.1' 'POST' 'test-token' $false
Assert-Access $false '' '127.0.0.1' 'GET' '' $false
Assert-Access $false $origin '192.168.1.2' 'POST' 'test-token' $false
Assert-Access $false $origin '127.0.0.1' 'POST' 'wrong' $false
Assert-Access $false $origin '127.0.0.1' 'DELETE' 'test-token' $false
Assert-Access $false $origin '127.0.0.1' 'POST' 'test-token' $true
Assert-Access $true 'http://localhost:9999' '127.0.0.1' 'GET' '' $false
Assert-Access $false 'http://127.0.0.1:8888' '127.0.0.1' 'GET' '' $false
Assert-Access $false 'http://localhost.evil.test:9999' '127.0.0.1' 'GET' '' $false
Assert-Access $false 'http://localhost:9999/' '127.0.0.1' 'GET' '' $false
Assert-Access $false 'http://localhost:9999' 'not-an-ip' 'GET' '' $false
$defaultOrigins = @(Get-BridgeOrigins 80)
if ($defaultOrigins -notcontains 'http://localhost' -or $defaultOrigins -notcontains 'http://127.0.0.1') { throw 'Default HTTP port must be canonicalized.' }
if ((Get-BridgeFailure 5).code -ne 'AccessDenied') { throw 'Permission failure must be actionable.' }
if ((Get-BridgeFailure 183).code -ne 'PortUnavailable') { throw 'Port conflict must be distinguished.' }
Write-Output 'PASS: custom origins, localhost, access checks and startup diagnostics; no network mutations.'

# A failed listener must be isolated from the telemetry loop and disabled, without retrying mutations.
$script:pollCount = 0
$script:closedAfterFailure = $false
function Poll-HotspotBridge { $script:pollCount++; throw 'Simulated listener failure' }
function Stop-HotspotBridge { $script:closedAfterFailure = $true }
$availability = @{ ready = $true; code = ''; message = '' }
Update-HotspotBridgeAvailability $availability -WarningAction SilentlyContinue
Update-HotspotBridgeAvailability $availability -WarningAction SilentlyContinue
if ($availability.ready -or $availability.code -ne 'RuntimeFailure' -or -not $script:closedAfterFailure -or $script:pollCount -ne 1) {
    throw 'A failed bridge must stop polling and leave its caller running.'
}
Write-Output 'PASS: runtime listener failure containment; no network mutations.'
