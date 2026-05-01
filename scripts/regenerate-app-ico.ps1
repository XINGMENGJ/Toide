# Regenerate client/resources/app.ico from the first *.jpg in repo root (e.g. 图标.jpg).
# Requires: Python with Pillow (pip install pillow)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$jpg = Get-ChildItem -LiteralPath $root -File -Filter *.jpg -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $jpg) {
    throw "No .jpg in repo root. Place 图标.jpg (or any .jpg) in $root"
}
$ico = Join-Path $root "client\resources\app.ico"
& python (Join-Path $root "scripts\regenerate-app-ico.py") $jpg.FullName $ico
Write-Host "Updated $ico"
