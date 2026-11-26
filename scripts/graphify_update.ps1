# graphify_update.ps1 - periodic incremental refresh of the Graphify graph.
#
# Runs `graphify update` (code re-extraction only; no LLM/API key needed) so the
# graph tracks changed source files without a full rebuild. Intended to be run by
# Windows Task Scheduler; safe to run by hand as well.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\graphify_update.ps1
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\graphify_update.ps1 -CheckOnly
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\graphify_update.ps1 -RepoRoot <path>
#
# Exit codes:
#   0 = updated, or nothing to do
#   1 = prerequisites failed (graphify missing, graph.json missing)
#   2 = update failed
#   3 = another rebuild is already running

[CmdletBinding()]
param(
    [string]$RepoRoot = '',
    [switch]$CheckOnly,
    [int]$KeepLogs = 14
)

$ErrorActionPreference = 'Stop'

if (-not $RepoRoot) {
    $scriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
    $RepoRoot = Split-Path -Parent $scriptDir
}

$outDir = Join-Path $RepoRoot 'graphify-out'
$graphJson = Join-Path $outDir 'graph.json'
$lockPath = Join-Path $outDir '.scheduled_update.lock'
$hookLockPath = Join-Path $outDir '.rebuild.lock'
$logDir = Join-Path $outDir 'logs'
$logPath = Join-Path $logDir ('graphify-update-{0}.log' -f (Get-Date).ToString('yyyyMMdd-HHmmss'))

function Write-Log {
    param([string]$Message)
    $line = '{0} {1}' -f (Get-Date).ToString('yyyy-MM-dd HH:mm:ss'), $Message
    Write-Host $line
    if (Test-Path -LiteralPath $logDir) {
        Add-Content -LiteralPath $logPath -Value $line -Encoding utf8
    }
}

function Get-GraphifyPath {
    $cmd = Get-Command graphify -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $candidates = @(
        (Join-Path $env:USERPROFILE '.local\bin\graphify.exe'),
        (Join-Path $env:APPDATA 'uv\tools\graphifyy\Scripts\graphify.exe')
    )
    foreach ($c in $candidates) {
        if (Test-Path -LiteralPath $c) { return $c }
    }
    return $null
}

function Test-LockOwner {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) { return $false }
    $raw = (Get-Content -LiteralPath $Path -Raw -ErrorAction SilentlyContinue)
    $pidValue = 0
    if (-not [int]::TryParse(([string]$raw).Trim(), [ref]$pidValue)) { return $false }
    return [bool](Get-Process -Id $pidValue -ErrorAction SilentlyContinue)
}

# --- prerequisites -----------------------------------------------------------
if (-not (Test-Path -LiteralPath $RepoRoot)) {
    Write-Host "graphify_update: repo root not found: $RepoRoot" -ForegroundColor Red
    exit 1
}
$graphify = Get-GraphifyPath
if (-not $graphify) {
    Write-Host "graphify_update: 'graphify' not found on PATH or in the uv tool dirs." -ForegroundColor Red
    exit 1
}
if (-not (Test-Path -LiteralPath $graphJson)) {
    Write-Host "graphify_update: no graph at $graphJson - run a full build first." -ForegroundColor Red
    exit 1
}

if ($CheckOnly) {
    Write-Host "graphify_update: OK"
    Write-Host "  repo:     $RepoRoot"
    Write-Host "  graphify: $graphify"
    Write-Host "  graph:    $graphJson"
    Write-Host "  logs:     $logDir"
    exit 0
}

# --- concurrency guard -------------------------------------------------------
if (Test-LockOwner -Path $lockPath) {
    Write-Host "graphify_update: another scheduled update is running (lock: $lockPath)." -ForegroundColor Yellow
    exit 3
}
if (Test-LockOwner -Path $hookLockPath) {
    Write-Host "graphify_update: a post-commit rebuild is running (lock: $hookLockPath)." -ForegroundColor Yellow
    exit 3
}
Remove-Item -LiteralPath $lockPath -Force -ErrorAction SilentlyContinue

New-Item -ItemType Directory -Path $logDir -Force | Out-Null
New-Item -ItemType Directory -Path $outDir -Force | Out-Null
Set-Content -LiteralPath $lockPath -Value $PID -Encoding ascii

$exitCode = 0
try {
    Write-Log "graphify update starting (repo: $RepoRoot)"
    # Pin the hash seed so Louvain clustering stays reproducible across runs.
    $env:PYTHONHASHSEED = '0'
    Push-Location -LiteralPath $RepoRoot
    try {
        $output = & $graphify update $RepoRoot 2>&1
        $exitCode = if ($null -ne $LASTEXITCODE) { [int]$LASTEXITCODE } else { 0 }
        $output | ForEach-Object { Write-Log ([string]$_) }
    }
    finally {
        Pop-Location
    }
    if ($exitCode -eq 0) {
        Write-Log 'graphify update finished (exit 0)'
    }
    else {
        Write-Log "graphify update FAILED (exit $exitCode)"
    }
}
catch {
    Write-Log ("graphify update FAILED: {0}" -f $_.Exception.Message)
    $exitCode = 2
}
finally {
    Remove-Item -LiteralPath $lockPath -Force -ErrorAction SilentlyContinue
    # Keep the newest $KeepLogs logs.
    if (Test-Path -LiteralPath $logDir) {
        Get-ChildItem -LiteralPath $logDir -Filter 'graphify-update-*.log' -File |
            Sort-Object LastWriteTime -Descending |
            Select-Object -Skip $KeepLogs |
            Remove-Item -Force -ErrorAction SilentlyContinue
    }
}

exit $exitCode
