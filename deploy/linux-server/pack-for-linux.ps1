# 在仓库根目录执行，生成 dist/ToideLinuxServer（仅服务端 + Linux 脚本），便于上传到 Linux。
# 用法:  powershell -ExecutionPolicy Bypass -File deploy/linux-server/pack-for-linux.ps1
param(
    [string]$OutputRel = "dist/ToideLinuxServer"
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "../..")
$bundleRoot = Join-Path $repoRoot $OutputRel
$serverSrc = Join-Path $repoRoot "server"
$linuxDeploy = Join-Path $repoRoot "deploy/linux-server"

if (-not (Test-Path $serverSrc)) {
    throw "server/ not found at $serverSrc"
}

if (Test-Path $bundleRoot) {
    Remove-Item -Recurse -Force $bundleRoot
}
New-Item -ItemType Directory -Path $bundleRoot | Out-Null

Write-Host "Copying server/ -> $bundleRoot/server"
Copy-Item -Path $serverSrc -Destination (Join-Path $bundleRoot "server") -Recurse -Force

# 不要附带开发者本机的 server.json（口令）；仅保留 example
$cfgJson = Join-Path $bundleRoot "server/config/server.json"
if (Test-Path $cfgJson) {
    Remove-Item -Force $cfgJson
    Write-Host "Removed bundled server/config/server.json (use .example on server)."
}

Copy-Item -Path (Join-Path $linuxDeploy "CMakeLists.txt") -Destination (Join-Path $bundleRoot "CMakeLists.txt") -Force
Copy-Item -Path (Join-Path $linuxDeploy "README.md") -Destination (Join-Path $bundleRoot "README.md") -Force
Copy-Item -Path (Join-Path $linuxDeploy "scripts") -Destination (Join-Path $bundleRoot "scripts") -Recurse -Force

Write-Host "Done. Upload this folder to Linux: $bundleRoot"
