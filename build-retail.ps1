$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    $rb3PreviousPythonPath = $env:PYTHONPATH
    $env:PYTHONPATH = Join-Path $PSScriptRoot 'build/python'
    python configure.py --version SZBE69
    if ($LASTEXITCODE -ne 0) { throw 'Retail configuration failed.' }
    & './build/python/bin/ninja.exe' -j 4
    if ($LASTEXITCODE -ne 0) { throw 'Retail build failed.' }
    # Configuration-only changes can leave Ninja's cached report stale.
    & './build/tools/objdiff-cli.exe' report generate -o build/SZBE69/report.json
    if ($LASTEXITCODE -ne 0) { throw 'Retail comparison report failed.' }
    python configure.py --version SZBE69 progress
    if ($LASTEXITCODE -ne 0) { throw 'Retail progress calculation failed.' }
} finally {
    $env:PYTHONPATH = $rb3PreviousPythonPath
    Pop-Location
}
