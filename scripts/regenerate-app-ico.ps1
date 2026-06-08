# Regenerate client/resources/app.ico from an image in repo root.
# Order: 三代图标.png, 新图标.png, 图标.jpg, then newest *.png / *.jpg.
# Requires: pip install pillow
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

$ico = Join-Path $root "client\resources\app.ico"
$pathV3Png = Join-Path $root "三代图标.png"
$pathNewPng = Join-Path $root "新图标.png"
$pathOldJpg = Join-Path $root "图标.jpg"

$src = $null
if (Test-Path -LiteralPath $pathV3Png) {
    $src = Get-Item -LiteralPath $pathV3Png
} elseif (Test-Path -LiteralPath $pathNewPng) {
    $src = Get-Item -LiteralPath $pathNewPng
} elseif (Test-Path -LiteralPath $pathOldJpg) {
    $src = Get-Item -LiteralPath $pathOldJpg
} else {
    $png = Get-ChildItem -LiteralPath $root -File -Filter *.png -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    $jpg = Get-ChildItem -LiteralPath $root -File -Filter *.jpg -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($png) { $src = $png }
    elseif ($jpg) { $src = $jpg }
}

if (-not $src) {
    throw "No .png or .jpg in repo root. Add 三代图标.png, 新图标.png, or 图标.jpg."
}

Write-Host "Source: $($src.FullName)"
& python (Join-Path $root "scripts\regenerate-app-ico.py") $src.FullName $ico
Write-Host "Wrote $ico"
