# 本地开发：可选启动 Redis、确保配置存在、启动 toide_server。
# 用法：
#   .\scripts\start-toide-dev.ps1
#   .\scripts\start-toide-dev.ps1 -SkipRedis
#   .\scripts\start-toide-dev.ps1 -BuildServer
param(
    [switch]$SkipRedis,
    [switch]$BuildServer
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

$cfg = Join-Path $RepoRoot "server\config\server.json"
$cfgEx = Join-Path $RepoRoot "server\config\server.json.example"
if (-not (Test-Path $cfg)) {
    Write-Host "创建 server\config\server.json（来自示例）..."
    Copy-Item $cfgEx $cfg -Force
}

function Test-TcpPortOpen {
    param([string]$HostName = "127.0.0.1", [int]$Port)
    try {
        $c = New-Object System.Net.Sockets.TcpClient
        $iar = $c.BeginConnect($HostName, $Port, $null, $null)
        $ok = $iar.AsyncWaitHandle.WaitOne(300, $false)
        if ($ok) { $c.EndConnect($iar) }
        $c.Close()
        return $ok
    } catch {
        return $false
    }
}

if (-not $SkipRedis) {
    if (Test-TcpPortOpen -Port 6379) {
        Write-Host "Redis 端口 6379 已在监听，跳过启动。"
    } elseif (Get-Command docker -ErrorAction SilentlyContinue) {
        $running = docker ps --filter "name=toide-redis" --format "{{.Names}}" 2>$null
        if ($running -eq "toide-redis") {
            Write-Host "容器 toide-redis 已在运行。"
        } else {
            Write-Host "尝试通过 Docker 启动 toide-redis（redis:7-alpine）..."
            docker run -d --rm --name toide-redis -p 6379:6379 redis:7-alpine 2>$null
            if ($LASTEXITCODE -ne 0) {
                Write-Warning "Docker 启动 Redis 失败，服务端将使用内存协作存储（若 Redis 不可用）。"
            }
        }
    } else {
        Write-Warning "未检测到 Docker，且 6379 未监听：请自行启动 Redis，否则服务端会回退到内存存储。"
    }
}

if ($BuildServer) {
    $buildCmd = Join-Path $RepoRoot "scripts\build-toide-server.cmd"
    Write-Host "构建服务端: $buildCmd"
    & cmd /c "`"$buildCmd`""
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$runCmd = Join-Path $RepoRoot "scripts\run-backend.cmd"
Write-Host "启动服务端: $runCmd"
& cmd /c "`"$runCmd`""
exit $LASTEXITCODE
