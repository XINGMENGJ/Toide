@echo off
setlocal
cd /d "%~dp0.." || exit /b 1
set "REPO=%CD%"

if not exist "server\config\server.json" (
  echo Creating server\config\server.json from example...
  copy /y "server\config\server.json.example" "server\config\server.json" >nul
)

call "%REPO%\qt6.7-env.cmd" || exit /b 1
call "%REPO%\scripts\toide-server-env.cmd" || exit /b 1

if not exist "build-server\server\toide_server.exe" (
  echo toide_server.exe not found. Run: scripts\build-toide-server.cmd
  exit /b 1
)

echo Run from: %REPO%
echo Listening: http://127.0.0.1:8848
echo.
"build-server\server\toide_server.exe"
exit /b %ERRORLEVEL%
