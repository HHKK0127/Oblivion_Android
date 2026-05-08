# Mount ISO and copy assets directly
# This is faster than full extraction

param(
    [string]$IsoPath = 'I:\Closet\Oblivion\Oblivion GOTY 1.iso',
    [string]$TargetDir = 'C:\Users\E1192\Projects\oblivion-android\app\src\main\assets'
)

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "ISO Mount and Asset Copy" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# Mount the ISO
Write-Host ""
Write-Host "Mounting ISO: $IsoPath" -ForegroundColor Yellow

$mountResult = Mount-DiskImage -ImagePath $IsoPath -PassThru
$driveLetter = ($mountResult | Get-Volume).DriveLetter

if ($driveLetter) {
    Write-Host "✓ ISO mounted as drive: $($driveLetter):" -ForegroundColor Green

    $sourceRoot = "$($driveLetter):\Data"

    # Create asset directories
    mkdir "$TargetDir\meshes" -Force | Out-Null
    mkdir "$TargetDir\textures" -Force | Out-Null

    Write-Host ""
    Write-Host "Copying sample assets..." -ForegroundColor Yellow

    # Copy furniture (smallest and most useful)
    if (Test-Path "$sourceRoot\meshes\furniture") {
        Write-Host "  Copying furniture meshes..."
        Copy-Item -Path "$sourceRoot\meshes\furniture" -Destination "$TargetDir\meshes\furniture" -Recurse -Force
    }

    if (Test-Path "$sourceRoot\textures\furniture") {
        Write-Host "  Copying furniture textures..."
        Copy-Item -Path "$sourceRoot\textures\furniture" -Destination "$TargetDir\textures\furniture" -Recurse -Force
    }

    # Copy doors (good for testing)
    if (Test-Path "$sourceRoot\meshes\architecture\doors") {
        Write-Host "  Copying door meshes..."
        Copy-Item -Path "$sourceRoot\meshes\architecture\doors" -Destination "$TargetDir\meshes\doors" -Recurse -Force
    }

    if (Test-Path "$sourceRoot\textures\architecture\doors") {
        Write-Host "  Copying door textures..."
        Copy-Item -Path "$sourceRoot\textures\architecture\doors" -Destination "$TargetDir\textures\doors" -Recurse -Force
    }

    Write-Host "✓ Assets copied successfully" -ForegroundColor Green

    # Unmount
    Write-Host ""
    Write-Host "Unmounting ISO..." -ForegroundColor Yellow
    Dismount-DiskImage -ImagePath $IsoPath
    Write-Host "✓ ISO unmounted" -ForegroundColor Green

    # Summary
    Write-Host ""
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "Copy Summary" -ForegroundColor Cyan
    Write-Host "==========================================" -ForegroundColor Cyan

    $nifCount = (Get-ChildItem -Path "$TargetDir\meshes" -Recurse -Filter "*.nif" -ErrorAction SilentlyContinue | Measure-Object).Count
    $ddsCount = (Get-ChildItem -Path "$TargetDir\textures" -Recurse -Filter "*.dds" -ErrorAction SilentlyContinue | Measure-Object).Count

    Write-Host "Meshes copied: $nifCount .nif files"
    Write-Host "Textures copied: $ddsCount .dds files"
    Write-Host "Target directory: $TargetDir"
    Write-Host ""
    Write-Host "✓ Ready for Android build!" -ForegroundColor Green

} else {
    Write-Host "ERROR: Failed to mount ISO" -ForegroundColor Red
}
