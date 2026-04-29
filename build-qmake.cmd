@echo off
call "%~dp0qt6.7-env.cmd"

qmake "%~dp0Toide.pro"
if errorlevel 1 exit /b %errorlevel%

mingw32-make
if errorlevel 1 exit /b %errorlevel%

echo.
echo Build finished. Run:
echo   bin\Toide.exe
