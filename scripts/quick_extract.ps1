# Quick Oblivion Asset Extraction
# 高速版: よく使うアセットだけを抽出

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Quick Oblivion Asset Extraction" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$isoPath = 'I:\Closet\Oblivion\Oblivion GOTY 1.iso'
$targetDir = 'C:\Users\E1192\Projects\oblivion-android\app\src\main\assets'
$sevenZip = 'C:\Program Files\7-Zip\7z.exe'

# ターゲットディレクトリ作成
mkdir "$targetDir\meshes" -Force | Out-Null
mkdir "$targetDir\textures" -Force | Out-Null

Write-Host ""
Write-Host "Extracting key assets from ISO..." -ForegroundColor Yellow
Write-Host ""

# リスト：抽出するフォルダ
$meshFolders = @(
    'Data/meshes/architecture/doors',
    'Data/meshes/furniture',
    'Data/meshes/weapons/weapons',
    'Data/meshes/creatures'
)

$textureFolders = @(
    'Data/textures/architecture/doors',
    'Data/textures/furniture',
    'Data/textures/weapons',
    'Data/textures/creatures'
)

# メッシュ抽出
foreach ($folder in $meshFolders) {
    $folderName = Split-Path $folder -Leaf
    Write-Host "Extracting: $folder"

    & $sevenZip x -o"$targetDir\meshes" "$isoPath" "$folder" | Out-Null
}

Write-Host ""

# テクスチャ抽出
foreach ($folder in $textureFolders) {
    $folderName = Split-Path $folder -Leaf
    Write-Host "Extracting: $folder"

    & $sevenZip x -o"$targetDir\textures" "$isoPath" "$folder" | Out-Null
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Extraction Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan

# Summary
$nifCount = (Get-ChildItem -Path "$targetDir\meshes" -Recurse -Filter "*.nif" -ErrorAction SilentlyContinue | Measure-Object).Count
$ddsCount = (Get-ChildItem -Path "$targetDir\textures" -Recurse -Filter "*.dds" -ErrorAction SilentlyContinue | Measure-Object).Count

Write-Host ""
Write-Host "Results:" -ForegroundColor Yellow
Write-Host "  Meshes: $nifCount .nif files"
Write-Host "  Textures: $ddsCount .dds files"
Write-Host ""
Write-Host "✓ Assets ready for Android build!" -ForegroundColor Green
