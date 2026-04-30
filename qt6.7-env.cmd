@echo off
rem Loads Qt MinGW into PATH for building Toide (Qt 6.11 + MinGW 13.1; filename kept for tasks/scripts).
set "QTDIR=E:\Qt\QTN\6.11.0\mingw_64"
set "QT_TOOLS=E:\Qt\QTN\Tools"
set "MINGW_DIR=%QT_TOOLS%\mingw1310_64"
set "CMAKE_DIR=%QT_TOOLS%\CMake_64"

set "PATH=%QTDIR%\bin;%MINGW_DIR%\bin;%CMAKE_DIR%\bin;%PATH%"

echo Qt environment loaded.
echo QTDIR=%QTDIR%
echo MINGW_DIR=%MINGW_DIR%
echo CMAKE_DIR=%CMAKE_DIR%
echo.
echo Try:
echo   qmake Toide.pro
echo   mingw32-make
echo   bin\Toide.exe
