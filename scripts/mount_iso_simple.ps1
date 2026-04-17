# シンプル ISO マウント スクリプト

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Oblivion ISO マウント" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$isoPath = 'I:\Closet\Oblivion\Oblivion GOTY 1.iso'

Write-Host "マウント処理を開始します..." -ForegroundColor Yellow
Write-Host "ISO: $isoPath"
Write-Host ""

# ISO をマウント
$mount = Mount-DiskImage -ImagePath $isoPath -PassThru -ErrorAction Stop

# ドライブレターを取得
Start-Sleep -Seconds 2
$volume = Get-DiskImage -ImagePath $isoPath | Get-Volume
$driveLetter = $volume.DriveLetter

Write-Host "✓ マウント完了！" -ForegroundColor Green
Write-Host "ドライブレター: $($driveLetter):" -ForegroundColor Green
Write-Host ""
Write-Host "エクスプローラーで以下の場所を開いてください：" -ForegroundColor Cyan
Write-Host "  $($driveLetter):\Data\meshes\furniture"
Write-Host "  $($driveLetter):\Data\textures\furniture"
Write-Host ""
Write-Host "これらのフォルダを以下にコピーしてください：" -ForegroundColor Cyan
Write-Host "  C:\Users\E1192\Projects\oblivion-android\app\src\main\assets\meshes"
Write-Host "  C:\Users\E1192\Projects\oblivion-android\app\src\main\assets\textures"
Write-Host ""
Write-Host "コピー完了後、以下を実行してアンマウントしてください：" -ForegroundColor Cyan
Write-Host "  Dismount-DiskImage -ImagePath '$isoPath'"
Write-Host ""
Write-Host "詳細は ASSET_EXTRACTION_MANUAL.md を参照してください。"
