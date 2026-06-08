# 一键：CMake 构建后端+客户端 → 新窗口启动 toide_server → windeployqt 封装 Toide.exe
# 用法（在仓库根或任意目录）:
#   .\scripts\build-run-backend-package-client.ps1
#   .\scripts\build-run-backend-package-client.ps1 -QtDir "E:\Qt\QTN\6.11.0\mingw_64" -NoServer
param(
    [string]$BuildDir = "build-mingw",
    [string]$QtDir = $(if ($env:TOIDE_QT_DIR) { $env:TOIDE_QT_DIR } else { "C:\Qt\6.11.0\mingw_64" }),
    [string]$OutputDir = "dist\ToideClient",
    [switch]$NoServer,
    [switch]$KillServerBeforeBuild
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

if ($KillServerBeforeBuild) {
    Get-Process -Name "toide_server" -ErrorAction SilentlyContinue | Stop-Process -Force
}

Write-Host "=== CMake 构建 toide_server + toide_client ($BuildDir) ===" -ForegroundColor Cyan
$buildAll = Join-Path $root "scripts\build-toide-all.cmd"
Push-Location $root
try {
    if ($BuildDir -eq "build-mingw") {
        & cmd.exe /c "`"$buildAll`""
    } else {
        & cmd.exe /c "`"$buildAll`" `"$BuildDir`""
    }
    if ($LASTEXITCODE -ne 0) {
        throw "构建失败，退出码 $LASTEXITCODE"
    }
} finally {
    Pop-Location
}

if (-not $NoServer) {
    Write-Host "=== 启动后端（新控制台窗口）===" -ForegroundColor Cyan
    $startBackend = Join-Path $root "scripts\start-backend-window.cmd"
    Start-Process -FilePath "cmd.exe" -ArgumentList @("/c", "`"$startBackend`"") -WorkingDirectory $root
}

Write-Host "=== 封装客户端 (windeployqt) ===" -ForegroundColor Cyan
$pack = Join-Path $root "scripts\package-client.ps1"
& powershell -NoProfile -ExecutionPolicy Bypass -File $pack -BuildDir $BuildDir -QtDir $QtDir -OutputDir $OutputDir

Write-Host "=== 完成 ===" -ForegroundColor Green
Write-Host "后端: http://127.0.0.1:8848 （新窗口运行 toide_server 时）"
Write-Host "封装输出: $root\$OutputDir\Toide.exe （若原 dist 被占用，脚本会改用 dist\ToideClient-时间戳，见上方警告）"
