@echo off
rem Loads Qt MinGW into PATH for building Toide.
rem Override paths via toide-env.local.cmd (copy from toide-env.local.cmd.example)
rem or environment variables TOIDE_QT_DIR / TOIDE_QT_TOOLS.

set "REPO=%~dp0"
if exist "%REPO%toide-env.local.cmd" call "%REPO%toide-env.local.cmd"

if not defined TOIDE_QT_DIR (
  if exist "C:\Qt\6.11.0\mingw_64" set "TOIDE_QT_DIR=C:\Qt\6.11.0\mingw_64"
)
if not defined TOIDE_QT_DIR (
  if exist "E:\Qt\6.11.0\mingw_64" set "TOIDE_QT_DIR=E:\Qt\6.11.0\mingw_64"
)
if not defined TOIDE_QT_DIR (
  if exist "E:\Qt\QTN\6.11.0\mingw_64" set "TOIDE_QT_DIR=E:\Qt\QTN\6.11.0\mingw_64"
)
if not defined TOIDE_QT_DIR (
  if exist "E:\QT\QTN\6.11.0\mingw_64" set "TOIDE_QT_DIR=E:\QT\QTN\6.11.0\mingw_64"
)
if not defined TOIDE_QT_DIR (
  echo ERROR: Qt MinGW not found. Set TOIDE_QT_DIR or create toide-env.local.cmd
  exit /b 1
)

if not defined TOIDE_QT_TOOLS (
  for %%D in ("%TOIDE_QT_DIR%\..\..\Tools" "%TOIDE_QT_DIR%\..\Tools" "C:\Qt\Tools" "E:\Qt\Tools" "E:\Qt\QTN\Tools" "E:\QT\QTN\Tools") do (
    if exist "%%~fD" set "TOIDE_QT_TOOLS=%%~fD"
  )
)
if not defined TOIDE_QT_TOOLS (
  echo ERROR: Qt Tools not found. Set TOIDE_QT_TOOLS (needs mingw1310_64 and CMake_64).
  exit /b 1
)

set "QTDIR=%TOIDE_QT_DIR%"
set "MINGW_DIR=%TOIDE_QT_TOOLS%\mingw1310_64"
set "CMAKE_DIR=%TOIDE_QT_TOOLS%\CMake_64"

if not exist "%MINGW_DIR%\bin\g++.exe" (
  echo ERROR: MinGW not found at %MINGW_DIR%
  exit /b 1
)

set "PATH=%QTDIR%\bin;%MINGW_DIR%\bin;%CMAKE_DIR%\bin;%PATH%"

echo Qt environment loaded.
echo QTDIR=%QTDIR%
echo MINGW_DIR=%MINGW_DIR%
echo CMAKE_DIR=%CMAKE_DIR%
exit /b 0
