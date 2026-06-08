param(
    [string]$BuildDir = "build-mingw",
    [string]$QtDir = $(if ($env:TOIDE_QT_DIR) { $env:TOIDE_QT_DIR } else { "C:\Qt\6.11.0\mingw_64" }),
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
    try {
        Remove-Item -Recurse -Force $target -ErrorAction Stop
    } catch {
        $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
        $fallback = Join-Path $root "dist\ToideClient-$stamp"
        Write-Warning "无法删除占用中的目录，改用: $fallback"
        $OutputDir = "dist\ToideClient-$stamp"
        $target = Join-Path $root $OutputDir
        if (Test-Path $target) {
            Remove-Item -Recurse -Force $target
        }
    }
}
New-Item -ItemType Directory -Path $target -Force | Out-Null
Copy-Item $exe (Join-Path $target "Toide.exe") -Force
$toideExe = Join-Path $target "Toide.exe"
# 刷新时间戳，避免资源管理器仍显示很久以前的「修改日期」（与是否成功替换二进制无关，便于确认已重新打包）
(Get-Item -LiteralPath $toideExe).LastWriteTime = Get-Date

$windeployqt = Join-Path $QtDir "bin\windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    throw "windeployqt not found: $windeployqt"
}
$eap = $ErrorActionPreference
$ErrorActionPreference = "Continue"
$p = Start-Process -FilePath $windeployqt -ArgumentList @("--release", "--compiler-runtime", $toideExe) -Wait -PassThru -NoNewWindow
$ErrorActionPreference = $eap
if ($p.ExitCode -ne 0) {
    throw "windeployqt exited with code $($p.ExitCode)"
}
(Get-Item -LiteralPath $toideExe).LastWriteTime = Get-Date

Copy-Item (Join-Path $root "server\config\server.json.example") (Join-Path $target "server.json.example") -ErrorAction SilentlyContinue
$i = Get-Item -LiteralPath $toideExe
Write-Host "Packaged Toide client to $target"
Write-Host ("Toide.exe: {0}, {1} bytes, LastWriteTime {2:o}" -f $i.FullName, $i.Length, $i.LastWriteTime)