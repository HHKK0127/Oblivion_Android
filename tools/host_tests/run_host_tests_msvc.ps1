# Build and run the native C++ host test suites with MSVC (x64) and AddressSanitizer.
#
# This is the local-Windows counterpart of run_host_tests.sh (which stays the CI
# path and keeps using g++). It exists for two reasons the MinGW runner cannot
# cover:
#   * MSVC's AddressSanitizer (/fsanitize=address) catches heap/stack misuse in
#     the suites that g++ on Windows cannot instrument.
#   * The produced binary can be opened in the Visual Studio debugger
#     (devenv /debugexe tmp\host_tests\host_tests_msvc.exe).
#
# The source/include/define lists are parsed out of run_host_tests.sh so the two
# runners cannot drift apart; edit the shell script to change the build set.
#
# Usage (from the repo root or anywhere):
#   powershell -File tools/host_tests/run_host_tests_msvc.ps1
#   powershell -File tools/host_tests/run_host_tests_msvc.ps1 -NoAsan
#   powershell -File tools/host_tests/run_host_tests_msvc.ps1 -ShowBuildOutput
#
# Exit code is 0 only when the build and every wired suite succeed.

[CmdletBinding()]
param(
    # Build without AddressSanitizer (roughly 2x faster, no memory checking).
    [switch]$NoAsan,
    # Force a fresh zlib download + build even when the cached library exists.
    [switch]$RebuildZlib,
    # Prebuilt zlib to link against, for example from vcpkg. Both are required.
    [string]$ZlibLib,
    [string]$ZlibInclude,
    # Print the whole compiler output instead of only errors and the summary.
    [switch]$ShowBuildOutput,
    # Backstop for a test run that never returns; the captured output is printed
    # either way. The suites finish in seconds, so this only fires on a wedge.
    [int]$RunTimeoutSeconds = 300
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$outDir = Join-Path $repoRoot 'tmp\host_tests'
$objDir = Join-Path $outDir 'obj'
$exePath = Join-Path $outDir 'host_tests_msvc.exe'
$buildLog = Join-Path $outDir 'msvc_build.log'
$shellScript = Join-Path $PSScriptRoot 'run_host_tests.sh'

# Writing a log must never fail the run: on Windows a stale compiler or shell
# process can hold the file open, and the build result matters more than the log.
function Write-LogFile {
    param([string]$Path, [object[]]$Content)

    for ($attempt = 1; $attempt -le 5; $attempt++) {
        try {
            $Content | Set-Content -LiteralPath $Path -Encoding UTF8 -ErrorAction Stop
            return
        }
        catch {
            if ($attempt -eq 5) {
                Write-Warning "Could not write '$Path': $($_.Exception.Message)"
                return
            }
            Start-Sleep -Milliseconds 500
        }
    }
}

# ---------------------------------------------------------------------------
# Locate Visual Studio and the x64 native build environment.
# ---------------------------------------------------------------------------

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found at '$vswhere'. Install Visual Studio with the Desktop development with C++ workload."
}

$vsPath = & $vswhere -products '*' -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    # Fallback: VS 18 (2026) Professional at the well-known path
    $fallback = 'C:\Program Files\Microsoft Visual Studio\18\Professional'
    if (Test-Path (Join-Path $fallback 'VC\Auxiliary\Build\vcvars64.bat')) {
        $vsPath = $fallback
        Write-Host "vswhere returned no results; using fallback path '$fallback'"
    } else {
        throw 'No Visual Studio installation with the MSVC x64 toolset was found.'
    }
}
$vsPath = $vsPath.Trim()

$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) {
    throw "vcvars64.bat not found at '$vcvars'."
}

# ---------------------------------------------------------------------------
# Parse the build set out of run_host_tests.sh (single source of truth).
# ---------------------------------------------------------------------------

function Get-BashArrayTokens {
    param([string]$Text, [string]$Name)

    $match = [regex]::Match($Text, "(?ms)^\s*$Name=\(\s*(?<body>.*?)^\s*\)\s*$")
    if (-not $match.Success) {
        throw "Could not parse the '$Name' array out of run_host_tests.sh."
    }

    $tokens = @()
    foreach ($line in ($match.Groups['body'].Value -split "`n")) {
        $stripped = ($line -replace '#.*$', '').Trim()
        if ($stripped) {
            $tokens += ($stripped -split '\s+')
        }
    }
    return $tokens
}

$shellText = Get-Content -LiteralPath $shellScript -Raw

$includeFlags = @()
foreach ($token in (Get-BashArrayTokens -Text $shellText -Name 'INCLUDES')) {
    if ($token.StartsWith('-I')) {
        $includeFlags += "/I$(Join-Path $repoRoot $token.Substring(2))"
    }
}

$defines = @()
$forcedIncludes = @()
$defineTokens = Get-BashArrayTokens -Text $shellText -Name 'DEFINES'
for ($i = 0; $i -lt $defineTokens.Count; $i++) {
    $token = $defineTokens[$i]
    if ($token.StartsWith('-D')) {
        $defines += "/D$($token.Substring(2))"
    }
    elseif ($token -eq '-include') {
        $i++
        if ($i -ge $defineTokens.Count) { throw "Dangling '-include' in run_host_tests.sh." }
        $forcedIncludes += "/FI$($defineTokens[$i])"
    }
    else {
        throw "Unsupported entry '$token' in the DEFINES array of run_host_tests.sh."
    }
}

$sources = @()
foreach ($token in (Get-BashArrayTokens -Text $shellText -Name 'SOURCES')) {
    if ($token.StartsWith('-')) { continue }
    $sources += (Join-Path $repoRoot $token)
}
if ($sources.Count -eq 0) { throw 'No sources parsed out of run_host_tests.sh.' }

# ---------------------------------------------------------------------------
# zlib: the asset readers decompress with it, and MSVC has no system copy.
# Build upstream zlib once into tmp\host_tests and reuse it afterwards.
# ---------------------------------------------------------------------------

function Get-Zlib {
    if ($ZlibLib -or $ZlibInclude) {
        if (-not ($ZlibLib -and $ZlibInclude)) {
            throw 'Pass both -ZlibLib and -ZlibInclude, or neither.'
        }
        if (-not (Test-Path $ZlibLib)) { throw "zlib library not found: $ZlibLib" }
        if (-not (Test-Path (Join-Path $ZlibInclude 'zlib.h'))) { throw "zlib.h not found in: $ZlibInclude" }
        return [pscustomobject]@{ Lib = $ZlibLib; Include = $ZlibInclude }
    }

    $zlibVersion = '1.3.1'
    $zlibRoot = Join-Path $outDir "zlib-$zlibVersion"
    $zlibBuild = Join-Path $outDir 'zlib-build'
    $zlibLibPath = Join-Path $zlibBuild 'zlib.lib'

    if ((Test-Path $zlibLibPath) -and -not $RebuildZlib) {
        return [pscustomobject]@{ Lib = $zlibLibPath; Include = $zlibRoot }
    }

    if (-not (Test-Path (Join-Path $zlibRoot 'zlib.h'))) {
        $tarball = Join-Path $outDir "zlib-$zlibVersion.tar.gz"
        Write-Host "Downloading zlib $zlibVersion source..."
        $url = "https://zlib.net/fossils/zlib-$zlibVersion.tar.gz"
        try {
            Invoke-WebRequest -Uri $url -OutFile $tarball -UseBasicParsing -TimeoutSec 120
        }
        catch {
            throw "Could not download $url. Re-run with -ZlibLib/-ZlibInclude pointing at an existing zlib build. ($($_.Exception.Message))"
        }
        & tar -xf $tarball -C $outDir
        if ($LASTEXITCODE -ne 0) { throw "tar failed to extract $tarball." }
    }

    Write-Host "Building zlib with MSVC..."
    New-Item -ItemType Directory -Force -Path $zlibBuild | Out-Null
    Get-ChildItem -Path $zlibBuild -Filter '*.obj' | Remove-Item -Force

    $zlibSources = @(
        'adler32.c', 'compress.c', 'crc32.c', 'deflate.c', 'gzclose.c', 'gzlib.c',
        'gzread.c', 'gzwrite.c', 'infback.c', 'inffast.c', 'inflate.c',
        'inftrees.c', 'trees.c', 'uncompr.c', 'zutil.c'
    )
    $zlibCmd = @(
        "call `"$vcvars`" >nul 2>nul",
        "cd /d `"$zlibRoot`"",
        "cl /nologo /c /O2 /MD /W0 /D_CRT_SECURE_NO_WARNINGS /Fo:`"$zlibBuild\\`" $($zlibSources -join ' ')",
        "lib /nologo /OUT:`"$zlibLibPath`" `"$zlibBuild\\*.obj`""
    ) -join ' && '

    $zlibOutput = & $env:ComSpec /c $zlibCmd 2>&1
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $zlibLibPath)) {
        $zlibOutput | ForEach-Object { Write-Host $_ }
        throw 'Building zlib with MSVC failed.'
    }

    return [pscustomobject]@{ Lib = $zlibLibPath; Include = $zlibRoot }
}

$zlib = Get-Zlib
$includeFlags += "/I$($zlib.Include)"

# Add stubs directory for MSVC-only headers (e.g. dirent.h)
$includeFlags += "/I$(Join-Path $PSScriptRoot 'stubs')"

# ---------------------------------------------------------------------------
# Compile and link.
# ---------------------------------------------------------------------------

New-Item -ItemType Directory -Force -Path $objDir | Out-Null
Get-ChildItem -Path $objDir -Filter '*.obj' | Remove-Item -Force

# /utf-8 keeps the bilingual source literals byte-identical to the GCC builds,
# and NOMINMAX/WIN32_LEAN_AND_MEAN keep the Windows headers out of the way.
# /Zi + /DEBUG produce a PDB next to the objects: ASAN needs it to symbolize its
# reports, and the Visual Studio debugger needs it to show source lines.
# C4005 is suppressed because the engine deliberately redefines LOG_TAG/LOGD
# per header, which GCC and Clang accept silently.
$clFlags = @(
    '/nologo', '/std:c++17', '/EHsc', '/O2', '/MD', '/W3', '/MP', '/utf-8',
    '/Zi', "/Fd`"$objDir\host_tests_msvc.pdb`"", '/wd4005',
    '/DNOMINMAX', '/DWIN32_LEAN_AND_MEAN', '/D_CRT_SECURE_NO_WARNINGS',
    '/D_CRT_NONSTDC_NO_DEPRECATE', '/D_USE_MATH_DEFINES',
    "/FI`"$PSScriptRoot\msvc_compat.h`"",
    "/Fo`"$objDir\\`""
)
if (-not $NoAsan) {
    $clFlags += '/fsanitize=address'
}
$clFlags += $defines
$clFlags += $forcedIncludes
$clFlags += $includeFlags

# cmd.exe caps a command line at 8191 characters and the translation unit list
# crosses that as the suites grow; hand cl.exe a response file instead
# (cl expands @file in place into its argument list).
$rspPath = Join-Path $objDir 'host_tests_msvc.rsp'
@($clFlags + $sources) | Set-Content -LiteralPath $rspPath -Encoding ascii

$buildCmd = @(
    "call `"$vcvars`" >nul 2>nul",
    "cd /d `"$repoRoot`"",
    "cl @`"$rspPath`" /Fe:`"$exePath`" /link /DEBUG /INCREMENTAL:NO `"$($zlib.Lib)`""
) -join ' && '

Write-Host "Compiling $($sources.Count) translation units with MSVC$(if ($NoAsan) { '' } else { ' + AddressSanitizer' })..."
$buildOutput = & $env:ComSpec /c $buildCmd 2>&1
$buildExit = $LASTEXITCODE
Write-LogFile -Path $buildLog -Content $buildOutput

if ($ShowBuildOutput) {
    $buildOutput | ForEach-Object { Write-Host $_ }
}
else {
    $buildOutput | Where-Object { $_ -match 'error C|error LNK|error D|: fatal error' } | ForEach-Object { Write-Host $_ }
    $warningCount = ($buildOutput | Where-Object { $_ -match 'warning C' }).Count
    if ($warningCount -gt 0) {
        Write-Host "$warningCount compiler warnings (see the log)."
    }
}

if ($buildExit -ne 0 -or -not (Test-Path $exePath)) {
    Write-Host ''
    Write-Host "Build failed (exit $buildExit). Full compiler output: $buildLog"
    exit 1
}

Write-Host "Build OK: $exePath"
Write-Host "Full compiler output: $buildLog"

# ---------------------------------------------------------------------------
# Run. vcvars is re-called so the MSVC runtime and, under ASAN, the
# clang_rt.asan_dynamic-x86_64.dll directory are on PATH.
# ---------------------------------------------------------------------------

if (-not $NoAsan) {
    # halt_on_error=0 makes ASAN report and keep running: the suites run to
    # completion, the findings all land in the log, and the run still exits
    # non-zero. With the default halt_on_error=1 MSVC ASAN aborts the process,
    # which can wedge behind a crash dialog and leave the run hanging.
    $env:ASAN_OPTIONS = 'halt_on_error=0'

    # ASAN shells out to llvm-symbolizer to turn addresses into source lines;
    # the MSVC toolset ships it next to cl.exe, which vcvars already put on PATH.
    $symbolizer = Join-Path $vsPath 'VC\Tools\MSVC'
    $symbolizer = Get-ChildItem -Path $symbolizer -Recurse -Filter 'llvm-symbolizer.exe' -ErrorAction SilentlyContinue |
        Where-Object { $_.DirectoryName -like '*Hostx64\x64' } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if ($symbolizer) {
        $env:PATH = "$($symbolizer.DirectoryName);$env:PATH"
    }
}

Write-Host ''
Write-Host '::group::Run host tests'
# The test process writes to a log file rather than to a pipe: an ASAN report
# can abort the process in a way that leaves a pipe open forever, and the log
# keeps the whole report (including the symbolizer output) for later reading.
$runLog = Join-Path $outDir 'msvc_run.log'
$runScript = Join-Path $outDir 'run_msvc_tests.cmd'
# A command line full of quotes survives cmd's parsing badly, so the run lives
# in a small script file instead.
Set-Content -Path $runScript -Encoding ASCII -Value @(
    '@echo off',
    "call `"$vcvars`" >nul 2>nul",
    "`"$exePath`" > `"$runLog`" 2>&1",
    'exit /b %ERRORLEVEL%'
)

# The process is built by hand rather than with Start-Process -PassThru: without
# -Wait, Windows PowerShell 5.1 hands back an object whose ExitCode reads as
# empty, which would report every green run as a failure.
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $env:ComSpec
$psi.Arguments = "/c `"$runScript`""
$psi.UseShellExecute = $false
$proc = New-Object System.Diagnostics.Process
$proc.StartInfo = $psi
$null = $proc.Start()
$timedOut = -not $proc.WaitForExit($RunTimeoutSeconds * 1000)
$runExit = if ($timedOut) { 1 } else { $proc.ExitCode }
if ($timedOut) {
    # /T is required: the wrapper cmd is not the process that wedges, so stopping
    # it alone would leave the test binary running and holding the log.
    & taskkill /PID $proc.Id /T /F 2>&1 | Out-Null
}
if (Test-Path $runLog) {
    Get-Content $runLog
}
Write-Host '::endgroup::'

$asanFindings = (Test-Path $runLog) -and
    [bool](Select-String -Path $runLog -Pattern 'AddressSanitizer' -SimpleMatch -Quiet)

if ($timedOut) {
    Write-Host "Host tests did not terminate within $RunTimeoutSeconds s; the run was stopped and the output above is everything captured first."
    exit 1
}
if ($asanFindings) {
    Write-Host "AddressSanitizer reported memory errors. The full report is above and in $runLog"
    exit 1
}
if ($runExit -ne 0) {
    Write-Host "Host tests failed (exit $runExit)."
    exit 1
}

Write-Host 'Host tests passed.'
exit 0
