@echo off
rem Prep PATH and CMAKE_PREFIX_PATH for Drogon-based toide_server (MinGW + local prefix under %USERPROFILE%\local).
rem Includes MariaDB Connector/C + hiredis so CMake finds DB/Redis deps; runtime needs libmariadb.dll and plugin folder.
set "TOIDE_LOCAL=%USERPROFILE%\local"
set "MARIADB_LIB=%TOIDE_LOCAL%\mariadb-connector-c\lib\mariadb"
set "CMAKE_PREFIX_PATH=%TOIDE_LOCAL%\drogon;%TOIDE_LOCAL%\jsoncpp;%TOIDE_LOCAL%\zlib;%TOIDE_LOCAL%\mariadb-connector-c;%TOIDE_LOCAL%\hiredis"
if defined QTDIR set "CMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH%;%QTDIR%"
set "PATH=%MARIADB_LIB%;%TOIDE_LOCAL%\jsoncpp\bin;%TOIDE_LOCAL%\zlib\bin;%TOIDE_LOCAL%\drogon\bin;%PATH%"
