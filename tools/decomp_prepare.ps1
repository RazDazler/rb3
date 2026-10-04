param(
    [ValidateSet('Run', 'Status', 'Stop')][string]$Action = 'Status',
    [int]$Minutes = 60,
    [int]$Limit = 64,
    [double]$MinScore = 70,
    [switch]$IncludeUnmatched,
    [string]$M2cPath = ''
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$state = Join-Path $repo 'build\decomp\preparation\SZBE69_B8'
$scriptPath = Join-Path $PSScriptRoot 'decomp_prepare.py'
$pidPath = Join-Path $state 'worker.pid'
$stopPath = Join-Path $state 'STOP'
New-Item -ItemType Directory -Force -Path $state | Out-Null

function OwnedWorker {
    if (-not (Test-Path -LiteralPath $pidPath)) { return $null }
    $workerId = [int](Get-Content -LiteralPath $pidPath)
    $process = Get-CimInstance Win32_Process -Filter "ProcessId = $workerId"
    if ($null -eq $process) { return $null }
    if ($process.CommandLine -notlike ('*' + $scriptPath + '*')) {
        throw 'Saved PID belongs to another process; it will not be signaled or reused.'
    }
    return $process
}

if ($Action -eq 'Run') {
    if ($Minutes -le 0 -or $Limit -le 0 -or $MinScore -lt 0 -or $MinScore -ge 100) {
        throw 'Positive bounds and 0 <= MinScore < 100 are required.'
    }
    if ($null -ne (OwnedWorker)) { throw 'Preparation is already running.' }
    if (Test-Path -LiteralPath $stopPath) { Remove-Item -LiteralPath $stopPath }
    $python = (Get-Command python.exe).Source
    $argv = @(('"' + $scriptPath + '"'), '--minutes', "$Minutes", '--limit', "$Limit", '--min-score', "$MinScore")
    if ($IncludeUnmatched) { $argv += '--include-unmatched' }
    # Reuse the audited local tool when available; never download at startup.
    if (-not $M2cPath) {
        $pinnedM2c = Join-Path $repo 'build\references\m2c-708d2d2cb2698f091a92492b328f73b24209f72d\m2c.py'
        if (Test-Path -LiteralPath $pinnedM2c) { $M2cPath = $pinnedM2c }
    }
    if ($M2cPath) {
        $resolvedM2c = (Resolve-Path -LiteralPath $M2cPath).Path
        $argv += @('--m2c', ('"' + $resolvedM2c + '"'))
    }
    $worker = Start-Process -FilePath $python -ArgumentList $argv -WorkingDirectory $repo -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $state 'worker.stdout.log') -RedirectStandardError (Join-Path $state 'worker.stderr.log')
    $worker.Id | Set-Content -LiteralPath $pidPath
    Write-Output "Read-only preparation PID $($worker.Id); at most $Minutes minutes / $Limit fresh unit comparisons. No model or cloud API calls."
    Write-Output 'The queue stops when complete and may finish in seconds if inputs are already cached. Use -Action Status to see results.'
}
elseif ($Action -eq 'Stop') {
    'Graceful stop requested' | Set-Content -LiteralPath $stopPath
    Write-Output 'Stop requested. The current bounded native command may finish; no game-source edits are made.'
}
else {
    $worker = OwnedWorker
    Write-Output "Preparation running: $($null -ne $worker)"
    $latest = Join-Path $state 'latest.json'
    if (Test-Path -LiteralPath $latest) {
        $result = Get-Content -LiteralPath $latest -Raw | ConvertFrom-Json
        Write-Output "Last status: $($result.status); packets: $($result.packets.Count); fresh comparisons: $($result.units_compared); cache hits: $($result.cache_hits)"
        Write-Output "Elapsed: $($result.elapsed_seconds) seconds. Minutes is a maximum, not a required duration."
        if ($result.status -eq 'queue_complete' -and $result.units_compared -eq 0 -and $result.cache_hits -gt 0) {
            Write-Output 'Completed successfully using cached results. No new comparisons or model work were needed.'
        }
    }
    Write-Output "Handoff: $(Join-Path $state 'HANDOFF.md')"
}
