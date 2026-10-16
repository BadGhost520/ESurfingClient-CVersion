@echo off
setlocal
if "%~1"=="" (
  echo Usage: Start-Dashboard.cmd "C:\path\ESurfingClient.exe"
  exit /b 1
)
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0TrafficCollector.ps1" -ClientPath "%~f1" -StartClient -HotspotControls
