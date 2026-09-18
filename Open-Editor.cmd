@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Launch-Demo.ps1" -Editor
if errorlevel 1 pause
