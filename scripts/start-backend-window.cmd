@echo off
rem Open a new console window and run the Toide HTTP/WebSocket backend (run-backend.cmd loads Qt/Drogon env).
setlocal EnableExtensions
cd /d "%~dp0.." || exit /b 1
start "Toide Server" cmd /k call "%~dp0run-backend.cmd"
exit /b 0
