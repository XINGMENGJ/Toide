@echo off
rem Full MinGW build: toide_server (needs Drogon + deps via toide-server-env.cmd) + toide_client (Qt).
setlocal EnableExtensions
cd /d "%~dp0.." || exit /b 1

call "%~dp0..\qt6.7-env.cmd" || exit /b 1
call "%~dp0toide-server-env.cmd" || exit /b 1

set "BUILD_DIR=build-mingw"
if not "%~1"=="" set "BUILD_DIR=%~1"

echo CMake configure: %BUILD_DIR%
cmake -S . -B "%BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release || exit /b 1

echo.
echo Building toide_server toide_client...
cmake --build "%BUILD_DIR%" -j 8 --target toide_server toide_client || exit /b 1

echo.
echo OK: %BUILD_DIR%\server\toide_server.exe
echo OK: %BUILD_DIR%\client\toide_client.exe
exit /b 0
