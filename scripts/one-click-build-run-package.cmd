@echo off
rem 一键：构建后端+客户端 → 新窗口启动后端 → 打包 Toide.exe（详见同目录 build-run-backend-package-client.ps1）
setlocal EnableExtensions
set "ROOT=%~dp0.."
cd /d "%ROOT%" || exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-run-backend-package-client.ps1" %*
exit /b %ERRORLEVEL%
