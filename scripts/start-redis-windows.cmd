@echo off
setlocal
set "ROOT=%~dp0..\tools\redis-windows"
set "EXE=%ROOT%\runtime\redis-server.exe"
if not exist "%EXE%" (
  echo Redis runtime missing: %EXE%
  echo Run: powershell -ExecutionPolicy Bypass -File "%~dp0bootstrap-redis-windows.ps1"
  exit /b 1
)
if not exist "%ROOT%\data" mkdir "%ROOT%\data"
cd /d "%ROOT%"
echo Starting Redis 127.0.0.1:6379 ...
start "Toide Redis" "%EXE%" "%ROOT%\redis.windows.conf"
echo Started. Stop: close the window or run scripts\stop-redis-windows.cmd
