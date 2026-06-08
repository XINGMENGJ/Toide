# Regenerate app.ico from repo root, then build toide_client and run package-client.ps1.
# Prefer: 新图标.png, then any .png, then any .jpg. Requires Python + Pillow, Qt 6.11 MinGW kit paths below.
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

# MinGW g++/ar must come before Git usr\bin (sh.exe breaks CMake "MinGW Makefiles" recipes using cd /d).
$prepend = @(
    "E:\QT\QTN\Tools\mingw1310_64\bin",
    "E:\QT\QTN\Tools\CMake_64\bin"
) | Where-Object { Test-Path $_ }
$env:Path = ($prepend -join ";") + ";" + $env:Path

function Find-IconSource {
    $candidates = @(
        (Join-Path $root "三代图标.png"),
        (Join-Path $root "新图标.png"),
        (Join-Path $root "图标.jpg")
    )
    foreach ($p in $candidates) {
        if (Test-Path -LiteralPath $p) { return (Get-Item -LiteralPath $p) }
    }
    $png = Get-ChildItem -LiteralPath $root -File -Filter *.png -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($png) { return $png }
    $jpg = Get-ChildItem -LiteralPath $root -File -Filter *.jpg -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($jpg) { return $jpg }
    return $null
}

$src = Find-IconSource
if (-not $src) {
    throw "No icon source in repo root (expected 三代图标.png, 新图标.png, 图标.jpg, or other .png/.jpg)."
}
$ico = Join-Path $root "client\resources\app.ico"
Write-Host "Icon source: $($src.FullName)"
& python (Join-Path $root "scripts\regenerate-app-ico.py") $src.FullName $ico

$buildDir = Join-Path $root "build-mingw"
if (-not (Test-Path $buildDir)) {
    throw "Missing $buildDir. Configure CMake once: cmake -G MinGW Makefiles -B build-mingw -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=E:/QT/QTN/6.11.0/mingw_64 -DCMAKE_CXX_COMPILER=E:/QT/QTN/Tools/mingw1310_64/bin/g++.exe"
}
Push-Location $buildDir
try {
    & cmake --build . --target toide_client --parallel 8
    if ($LASTEXITCODE -ne 0) { throw "cmake --build failed with $LASTEXITCODE" }
} finally {
    Pop-Location
}

Stop-Process -Name "Toide" -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 300
$out = Join-Path $root "dist\ToideClient"
if (Test-Path $out) { Remove-Item -Recurse -Force $out }
& (Join-Path $root "scripts\package-client.ps1") -BuildDir "build-mingw" -QtDir "E:\QT\QTN\6.11.0\mingw_64" -OutputDir "dist\ToideClient"
Write-Host "Done: $(Join-Path $root 'dist\ToideClient\Toide.exe')"
