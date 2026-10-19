@echo off
setlocal
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\windows-network-panel\Start-Dashboard.ps1" %*
if errorlevel 1 pause
