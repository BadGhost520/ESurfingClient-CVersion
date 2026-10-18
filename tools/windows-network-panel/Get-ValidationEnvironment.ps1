[CmdletBinding()]
param([string]$ClientPath)
# Read-only inventory. No client launch, listener, credential or hotspot access.
$ErrorActionPreference = 'Stop'
$windows = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion'
$nativeArchitecture = $env:PROCESSOR_ARCHITEW6432
if (-not $nativeArchitecture) { $nativeArchitecture = $env:PROCESSOR_ARCHITECTURE }
$launcherCheck = 'NotRequested'
if ($ClientPath) {
    try {
        $null = & "$PSScriptRoot\Start-Dashboard.ps1" -ClientPath $ClientPath -Check
        $launcherCheck = 'Passed'
    } catch { $launcherCheck = 'Failed: check client path, portal/assets and Windows PowerShell 5.1' }
}
[ordered]@{
    checkedAt = [DateTimeOffset]::Now.ToString('o')
    windowsBuild = [string]$windows.CurrentBuildNumber
    windowsRevision = $windows.UBR
    windowsDisplayVersion = $windows.DisplayVersion
    nativeArchitecture = $nativeArchitecture
    processBits = [IntPtr]::Size * 8
    powershellVersion = $PSVersionTable.PSVersion.ToString()
    powershellEdition = $PSVersionTable.PSEdition
    launcherCheck = $launcherCheck
    hardwareAcceptance = 'Pending manual checklist; environment inventory is not a pass'
} | ConvertTo-Json
