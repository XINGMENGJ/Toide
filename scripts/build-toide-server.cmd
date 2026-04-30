@echo off
setlocal
cd /d "%~dp0.." || exit /b 1
set "REPO=%CD%"

call "%REPO%\qt6.7-env.cmd" || exit /b 1
call "%REPO%\scripts\toide-server-env.cmd" || exit /b 1

cmake -S "%REPO%" -B "%REPO%\build-mingw" -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%"
if errorlevel 1 exit /b 1

cmake --build "%REPO%\build-mingw" --target toide_server -j 4
exit /b %ERRORLEVEL%
