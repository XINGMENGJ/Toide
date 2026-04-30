@echo off
echo Stopping redis-server.exe ...
taskkill /IM redis-server.exe /F >nul 2>&1
if %ERRORLEVEL%==0 (echo Redis stopped.) else (echo No redis-server.exe process found.)
