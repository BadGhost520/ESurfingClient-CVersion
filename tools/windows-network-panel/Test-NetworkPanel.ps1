$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\NetworkPanel.ps1" -LibraryOnly

function Assert-True($Condition, $Message) {
    if (-not $Condition) { throw $Message }
}

$first = @{ Received = 1000L; Sent = 500L; Time = 10.0 }
$next = @{ Received = 5096L; Sent = 2548L; Time = 12.0 }
$rate = Get-TrafficRate $first $next
Assert-True ($rate.Down -eq 2048 -and $rate.Up -eq 1024) 'Rates must use elapsed seconds.'
Assert-True ($null -eq (Get-TrafficRate $null $first)) 'First sample is not a zero speed.'
Assert-True ($null -eq (Get-TrafficRate $first $first)) 'Reject zero elapsed time.'
Assert-True ($null -eq (Get-TrafficRate $next $first)) 'Reject backwards time/counter reset.'
$reset = @{ Received = 0L; Sent = 0L; Time = 14.0 }
Assert-True ($null -eq (Get-TrafficRate $next $reset)) 'Adapter reset must reset baseline.'
$idle = @{ Received = 1000L; Sent = 500L; Time = 11.0 }
Assert-True ((Get-TrafficRate $first $idle).Down -eq 0) 'Idle traffic is zero.'
Assert-True ((Format-Rate 1024) -eq '1.00 KiB/s') 'Binary units must match the divisor.'
Assert-True ((Format-Rate 1048576) -eq '1.00 MiB/s') 'MiB conversion failed.'
Assert-True ((Format-Rate 0) -eq '0 B/s') 'Zero formatting failed.'
Write-Output 'PASS: traffic sampling, counter reset, idle and units (9 assertions).'
