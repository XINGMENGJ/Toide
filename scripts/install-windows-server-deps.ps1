# Build Drogon + native deps for toide_server (Windows MinGW).
# Installs under %USERPROFILE%\local\ (same layout as scripts\toide-server-env.cmd).
#
# Prerequisites:
#   - Git, CMake 3.24+
#   - Qt 6.x MinGW kit loaded (run qt6.7-env.cmd first, or set TOIDE_QT_DIR)
#
# Usage (from repo root):
#   .\scripts\install-windows-server-deps.ps1
#   .\scripts\install-windows-server-deps.ps1 -Prefix "D:\toide-deps"
param(
    [string]$Prefix = "$env:USERPROFILE\local",
    [string]$DrogonTag = "v1.9.6"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$thirdParty = Join-Path $root ".third_party"

function Require-Cmd($name) {
    if (-not (Get-Command $name -ErrorAction SilentlyContinue)) {
        throw "Required command not found: $name"
    }
}

function Cmake-Install($sourceDir, $buildDir, [string[]]$extraArgs) {
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
    $cfg = @(
        "-S", $sourceDir,
        "-B", $buildDir,
        "-G", "MinGW Makefiles",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_INSTALL_PREFIX=$Prefix"
    ) + $extraArgs
    & cmake @cfg
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed: $sourceDir" }
    & cmake --build $buildDir --parallel 8
    if ($LASTEXITCODE -ne 0) { throw "cmake build failed: $sourceDir" }
    & cmake --install $buildDir
    if ($LASTEXITCODE -ne 0) { throw "cmake install failed: $sourceDir" }
}

function Ensure-GitRepo($url, $dest, $tag) {
    if (-not (Test-Path (Join-Path $dest ".git"))) {
        New-Item -ItemType Directory -Force -Path (Split-Path $dest) | Out-Null
        & git clone $url $dest
        if ($LASTEXITCODE -ne 0) { throw "git clone failed: $url" }
    }
    Push-Location $dest
    try {
        & git fetch --tags --depth 1 origin $tag 2>$null
        & git checkout $tag
        if ($LASTEXITCODE -ne 0) { throw "git checkout $tag failed in $dest" }
    } finally {
        Pop-Location
    }
}

$envCmd = Join-Path $root "qt6.7-env.cmd"
if (Test-Path $envCmd) {
    cmd /c "`"$envCmd`" >nul"
}

Require-Cmd cmake
Require-Cmd git
Require-Cmd g++

New-Item -ItemType Directory -Force -Path $Prefix | Out-Null
Write-Host "=== Installing native deps to $Prefix ===" -ForegroundColor Cyan

# zlib
$zlibSrc = Join-Path $thirdParty "zlib-src"
Ensure-GitRepo "https://github.com/madler/zlib.git" $zlibSrc "v1.3.1"
Cmake-Install $zlibSrc (Join-Path $thirdParty "zlib-build") @()

# jsoncpp
$jsonSrc = Join-Path $thirdParty "jsoncpp-src"
Ensure-GitRepo "https://github.com/open-source-parsers/jsoncpp.git" $jsonSrc "1.9.5"
Cmake-Install $jsonSrc (Join-Path $thirdParty "jsoncpp-build") @(
    "-DJSONCPP_WITH_TESTS=OFF",
    "-DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF",
    "-DBUILD_SHARED_LIBS=ON",
    "-DBUILD_STATIC_LIBS=OFF"
)

# hiredis
$hiredisSrc = Join-Path $thirdParty "hiredis-src"
Ensure-GitRepo "https://github.com/redis/hiredis.git" $hiredisSrc "v1.2.0"
Cmake-Install $hiredisSrc (Join-Path $thirdParty "hiredis-build") @(
    "-DENABLE_SSL=OFF"
)

# MariaDB Connector/C (async API required by Drogon; connects to MySQL 5.7+)
$mariadbSrc = Join-Path $thirdParty "mariadb-connector-c-src"
Ensure-GitRepo "https://github.com/mariadb/mariadb-connector-c.git" $mariadbSrc "v3.4.0"
Cmake-Install $mariadbSrc (Join-Path $thirdParty "mariadb-build") @(
    "-DWITH_SSL=OPENSSL",
    "-DOPENSSL_ROOT_DIR=$Prefix",
    "-DINSTALL_PLUGINDIR=lib/mariadb/plugin"
)

# Drogon
$drogonSrc = Join-Path $thirdParty "drogon-src"
if (-not (Test-Path (Join-Path $drogonSrc ".git"))) {
    New-Item -ItemType Directory -Force -Path (Split-Path $drogonSrc) | Out-Null
    & git clone --recurse-submodules https://github.com/drogonframework/drogon.git $drogonSrc
    if ($LASTEXITCODE -ne 0) { throw "drogon clone failed" }
}
Push-Location $drogonSrc
try {
    & git fetch --tags --depth 1 origin $DrogonTag 2>$null
    & git checkout $DrogonTag
    & git submodule update --init --recursive
} finally {
    Pop-Location
}

$prefixPath = $Prefix -replace '\\', '/'
Cmake-Install $drogonSrc (Join-Path $thirdParty "drogon-build") @(
    "-DCMAKE_PREFIX_PATH=$prefixPath",
    "-DBUILD_EXAMPLES=OFF",
    "-DBUILD_CTL=OFF",
    "-DBUILD_ORM=ON",
    "-DBUILD_MYSQL=ON",
    "-DBUILD_REDIS=ON",
    "-DBUILD_SQLITE=OFF",
    "-DBUILD_POSTGRESQL=OFF"
)

Write-Host ""
Write-Host "=== Done ===" -ForegroundColor Green
Write-Host "Prefix: $Prefix"
Write-Host "Next:"
Write-Host "  1. mysql -u root -p < scripts\init-mysql.sql"
Write-Host "  2. mysql -u toide toide < server\migrations\001_users.sql"
Write-Host "  3. mysql -u toide toide < server\migrations\002_workspace_files.sql"
Write-Host "  4. mysql -u toide toide < server\migrations\003_workspaces.sql"
Write-Host "  5. scripts\build-toide-all.cmd"
Write-Host "Runtime PATH must include: $Prefix\mariadb-connector-c\lib\mariadb (see scripts\toide-server-env.cmd)"
