# Oblivion Assets Organization Script
# Purpose: Organize extracted assets into Android project structure

param(
    [string]$SourceDir = 'C:\Temp\Oblivion_Extract\Data',
    [string]$TargetDir = 'C:\Users\E1192\Projects\oblivion-android\app\src\main\assets'
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Organizing Oblivion Assets" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Check source
if (-not (Test-Path $SourceDir)) {
    Write-Host "ERROR: Source directory not found: $SourceDir" -ForegroundColor Red
    exit 1
}

# Create target directories
$meshesDir = "$TargetDir\meshes"
$texturesDir = "$TargetDir\textures"

Write-Host "Creating directories..."
mkdir $meshesDir -Force | Out-Null
mkdir $texturesDir -Force | Out-Null
Write-Host "✓ Directories created" -ForegroundColor Green

# Copy meshes (sample furniture first - smaller files)
Write-Host ""
Write-Host "Copying sample meshes..." -ForegroundColor Yellow

$sampleMeshes = @(
    'architecture\doors',
    'furniture',
    'weapons\weapons',
    'creatures'
)

foreach ($meshFolder in $sampleMeshes) {
    $source = "$SourceDir\meshes\$meshFolder"
    $target = "$meshesDir\$(Split-Path $meshFolder -Leaf)"

    if (Test-Path $source) {
        Write-Host "  Copying: $meshFolder"
        Copy-Item -Path $source -Destination $target -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "✓ Meshes copied" -ForegroundColor Green

# Copy textures (matching the mesh folders)
Write-Host ""
Write-Host "Copying sample textures..." -ForegroundColor Yellow

$sampleTextures = @(
    'architecture\doors',
    'furniture',
    'weapons',
    'creatures'
)

foreach ($textureFolder in $sampleTextures) {
    $source = "$SourceDir\textures\$textureFolder"
    $target = "$texturesDir\$(Split-Path $textureFolder -Leaf)"

    if (Test-Path $source) {
        Write-Host "  Copying: $textureFolder"
        Copy-Item -Path $source -Destination $target -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "✓ Textures copied" -ForegroundColor Green

# Summary
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Asset Organization Complete!" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$meshCount = (Get-ChildItem -Path $meshesDir -Recurse -Filter "*.nif" -ErrorAction SilentlyContinue | Measure-Object).Count
$texCount = (Get-ChildItem -Path $texturesDir -Recurse -Filter "*.dds" -ErrorAction SilentlyContinue | Measure-Object).Count

Write-Host "Summary:" -ForegroundColor Yellow
Write-Host "  Meshes (.nif): $meshCount files"
Write-Host "  Textures (.dds): $texCount files"
Write-Host "  Target: $TargetDir"
Write-Host ""
Write-Host "✓ Assets are now ready for Android APK!" -ForegroundColor Green
