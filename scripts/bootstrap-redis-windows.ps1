# 下载并解压 Windows 版 Redis（tporadowski/redis，开发用途）。
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

$destRoot = Join-Path $repoRoot "tools\redis-windows"
$version = "5.0.14.1"
$zipName = "Redis-x64-$version.zip"
$uri = "https://github.com/tporadowski/redis/releases/download/v$version/$zipName"
$zipPath = Join-Path $destRoot $zipName
$runtime = Join-Path $destRoot "runtime"

New-Item -ItemType Directory -Force -Path $destRoot | Out-Null

if (-not (Test-Path (Join-Path $runtime "redis-server.exe"))) {
    $proxy = "http://127.0.0.1:7890"
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Write-Host "Downloading Redis $version ..."
    try {
        Invoke-WebRequest -Uri $uri -OutFile $zipPath -Proxy $proxy -ProxyUseDefaultCredentials
    } catch {
        Invoke-WebRequest -Uri $uri -OutFile $zipPath
    }
    if (Test-Path $runtime) { Remove-Item $runtime -Recurse -Force }
    Expand-Archive -LiteralPath $zipPath -DestinationPath $runtime -Force
    Remove-Item $zipPath -Force -ErrorAction SilentlyContinue
}

if (-not (Test-Path (Join-Path $runtime "redis-server.exe"))) {
    throw "redis-server.exe not found under $runtime"
}
Write-Host "Redis ready: $runtime\redis-server.exe"
Write-Host "Start: ..\scripts\start-redis-windows.cmd"
