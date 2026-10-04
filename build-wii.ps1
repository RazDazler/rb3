param(
    [ValidateSet('SZBE69_B8', 'SZBE69')]
    [string]$Version = 'SZBE69_B8',
    [ValidateRange(1, 32)]
    [int]$Jobs = 4
)

$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    $rb3PreviousPythonPath = $env:PYTHONPATH
    $env:PYTHONPATH = Join-Path $PSScriptRoot 'build/python'
    python configure.py --version $Version
    if ($LASTEXITCODE -ne 0) { throw "$Version configuration failed." }
    & './build/python/bin/ninja.exe' -j $Jobs
    if ($LASTEXITCODE -ne 0) { throw "$Version build failed." }
    & './build/tools/dtk.exe' shasum -c "config/$Version/build.sha1"
    if ($LASTEXITCODE -ne 0) { throw "$Version executable checksum failed." }
    # Regenerate comparisons even when Ninja considers a cached report current.
    & './build/tools/objdiff-cli.exe' report generate -o "build/$Version/report.json"
    if ($LASTEXITCODE -ne 0) { throw "$Version comparison report failed." }
    python configure.py --version $Version progress
    if ($LASTEXITCODE -ne 0) { throw "$Version progress calculation failed." }
} finally {
    $env:PYTHONPATH = $rb3PreviousPythonPath
    Pop-Location
}
