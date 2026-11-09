<#
.SYNOPSIS
  Run Ghidra headless with ApplyVmHandlers.java against Oblivion.exe.

.DESCRIPTION
  1. Exports vm_handlers_out.json via `oblivion-re vm --json`
  2. Runs analyzeHeadless with ApplyVmHandlers.java
  3. Optionally syncs annotate_db.json names

.EXAMPLE
  .\run-vm-annotate.ps1
  .\run-vm-annotate.ps1 -ExePath "C:\...\Oblivion.exe" -ProjectDir "D:\Cargo\Ghidra\Oblivion"
#>

param(
    [string]$ExePath = "C:\Program Files (x86)\Steam\steamapps\common\Oblivion\Oblivion.exe",
    [string]$ProjectDir = "D:\Cargo\Ghidra\Oblivion",
    [string]$ProjectName = "Oblivion",
    [string]$GhidraHome = "D:\Cargo\ghidra_12.1.2_PUBLIC_20260605\ghidra_12.1.2_PUBLIC",
    [string]$ScriptDir = "D:\Cargo\Ghidra\scripts",
    [string]$AnnotateDb = "C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\annotate_db.json",
    [string]$VmJsonOut = "$env:TEMP\opencode\vm_handlers_out.json",
    [switch]$SkipExport
)

$ErrorActionPreference = "Stop"

# --- Step 1: Export vm handlers JSON via Rust CLI ---
if (-not $SkipExport) {
    Write-Host "[1/3] Exporting vm_handlers_out.json ..."
    $cliDir = "C:\Users\hiroki.kogarumai\oblivion-re-tools"
    Push-Location $cliDir
    try {
        cargo run --offline -p cli -- vm $ExePath --json 2>$null | Set-Content $VmJsonOut -Encoding UTF8
    } finally {
        Pop-Location
    }
    $count = (Get-Content $VmJsonOut -Raw | ConvertFrom-Json).Count
    Write-Host "  exported $count handlers -> $VmJsonOut"
} else {
    Write-Host "[1/3] Skip export (using existing $VmJsonOut)"
}

# --- Step 2: Verify inputs ---
if (-not (Test-Path $VmJsonOut)) { throw "vm json not found: $VmJsonOut" }
if (-not (Test-Path $AnnotateDb)) { Write-Warning "annotate_db not found: $AnnotateDb (names won't be overridden)" }
$headless = Join-Path $GhidraHome "support\analyzeHeadless.bat"
if (-not (Test-Path $headless)) { throw "analyzeHeadless not found: $headless" }

# --- Step 3: Run Ghidra headless ---
Write-Host "[2/3] Running Ghidra headless ..."

# Build args: postScript with VM json + optional annotate db
$postScriptArgs = @("ApplyVmHandlers.java", $VmJsonOut)
if (Test-Path $AnnotateDb) { $postScriptArgs += $AnnotateDb }

# analyzeHeadless syntax:
#   analyzeHeadless <project_dir> <project_name> [-import <file>] [-postScript <script> <args...>]
& $headless $ProjectDir $ProjectName `
    -import $ExePath `
    -scriptPath $ScriptDir `
    -postScript $postScriptArgs `
    -deleteProject `
    -log "$env:TEMP\opencode\ghidra_vm_annotate.log"

if ($LASTEXITCODE -ne 0) {
    Write-Warning "analyzeHeadless exit code: $LASTEXITCODE (check log)"
}

Write-Host "[3/3] Done. Log: $env:TEMP\opencode\ghidra_vm_annotate.log"
