@echo off
rem Prep PATH and CMAKE_PREFIX_PATH for Drogon-based toide_server (MinGW + local prefix under %USERPROFILE%\local).
set "TOIDE_LOCAL=%USERPROFILE%\local"
set "CMAKE_PREFIX_PATH=%TOIDE_LOCAL%\drogon;%TOIDE_LOCAL%\jsoncpp;%TOIDE_LOCAL%\zlib"
if defined QTDIR set "CMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH%;%QTDIR%"
set "PATH=%TOIDE_LOCAL%\jsoncpp\bin;%TOIDE_LOCAL%\zlib\bin;%TOIDE_LOCAL%\drogon\bin;%PATH%"
