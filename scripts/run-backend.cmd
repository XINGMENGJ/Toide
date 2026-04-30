@echo off
rem Run Toide HTTP/WebSocket backend (toide_server).
rem Prerequisite: build first, e.g. scripts\build-toide-server.cmd
rem Working directory must stay repo root so server/config/server.json is found.

setlocal EnableExtensions
cd /d "%~dp0.." || exit /b 1
set "REPO=%CD%"

if not exist "server\config\server.json" (
  echo Creating server\config\server.json from example...
  copy /y "server\config\server.json.example" "server\config\server.json" >nul
)

call "%REPO%\qt6.7-env.cmd" || exit /b 1
call "%REPO%\scripts\toide-server-env.cmd" || exit /b 1

set "EXE="
if exist "%REPO%\build-mingw\server\toide_server.exe" set "EXE=%REPO%\build-mingw\server\toide_server.exe"
if not defined EXE if exist "%REPO%\build-server\server\toide_server.exe" (
  echo Warning: using legacy build-server output. Prefer scripts\build-toide-server.cmd to rebuild build-mingw.
  set "EXE=%REPO%\build-server\server\toide_server.exe"
)

if not defined EXE (
  echo toide_server.exe not found in:
  echo   build-mingw\server\
  echo   build-server\server\  ^(legacy fallback^)
  echo Build with: scripts\build-toide-server.cmd
  exit /b 1
)

echo Repository: %REPO%
echo Executable: %EXE%
echo Listening:  http://127.0.0.1:8848  ^(see server\config\server.json^)
echo Stop:       Ctrl+C
echo.
"%EXE%"
exit /b %ERRORLEVEL%
