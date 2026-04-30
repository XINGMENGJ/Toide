@echo off
rem 启动 Toide 后端（与 run-backend.cmd 一致：自动在 build-server / build-mingw 查找可执行文件）。
call "%~dp0run-backend.cmd" %*
exit /b %ERRORLEVEL%
