param(
    [string]$BuildDir = "build-mingw",
    [string]$QtDir = "E:\QT\QTN\6.11.0\mingw_64",
    [string]$OutputDir = "dist\ToideClient"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root "$BuildDir\client\toide_client.exe"
if (-not (Test-Path $exe)) {
    throw "Client executable not found: $exe. Build first with CMake/MinGW."
}

$target = Join-Path $root $OutputDir
if (Test-Path $target) {
    Remove-Item -Recurse -Force $target
}
New-Item -ItemType Directory -Path $target | Out-Null
Copy-Item $exe (Join-Path $target "Toide.exe")

$windeployqt = Join-Path $QtDir "bin\windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    throw "windeployqt not found: $windeployqt"
}
& $windeployqt --release --compiler-runtime (Join-Path $target "Toide.exe")

Copy-Item (Join-Path $root "server\config\server.json.example") (Join-Path $target "server.json.example") -ErrorAction SilentlyContinue
Write-Host "Packaged Toide client to $target"
