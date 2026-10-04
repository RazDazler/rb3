param(
    [ValidateSet('Setup','Start','Run','Stop','Status','StopServer')]
    [string]$Action = 'Status',
    [int]$Minutes = 120,
    [int]$AttemptsPerTask = 8,
    [int]$Limit = 32
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$state = Join-Path $repo 'build\local-agent'
$runtime = Join-Path $state 'runtime'
$exe = Join-Path $runtime 'ollama.exe'
$endpoint = 'http://127.0.0.1:11435'
$model = 'qwen2.5-coder:7b-instruct-q4_K_M'
New-Item -ItemType Directory -Force $state | Out-Null
# Process-scoped settings; no PATH, login/startup or global environment changes.
$env:OLLAMA_HOST = '127.0.0.1:11435'
$env:OLLAMA_MODELS = Join-Path $state 'models'
$env:OLLAMA_NO_CLOUD = '1'
$env:OLLAMA_NUM_PARALLEL = '1'
$env:OLLAMA_MAX_LOADED_MODELS = '1'
$env:OLLAMA_CONTEXT_LENGTH = '6144'
$env:OLLAMA_FLASH_ATTENTION = '1'
$env:OLLAMA_KV_CACHE_TYPE = 'q8_0'

function ServerReady {
    try { $null = Invoke-RestMethod "$endpoint/api/version" -TimeoutSec 2; return $true }
    catch { return $false }
}
function StartServer {
    if (-not (Test-Path -LiteralPath $exe)) { throw 'Run Setup first.' }
    if (ServerReady) {
        if ($null -eq (OwnedProcess (Join-Path $state 'server.pid') $exe)) { throw 'Port 11435 is served by an unmanaged process; refusing to reuse it.' }
        return
    }
    $process = Start-Process -FilePath $exe -ArgumentList 'serve' -WorkingDirectory $runtime -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $state 'server.stdout.log') -RedirectStandardError (Join-Path $state 'server.stderr.log')
    $process.Id | Set-Content (Join-Path $state 'server.pid')
    for ($i = 0; $i -lt 30; $i++) {
        if (ServerReady) { return }
        if ($process.HasExited) { throw 'Ollama exited; inspect server.stderr.log.' }
        Start-Sleep -Seconds 1
    }
    throw 'Ollama did not become ready; inspect server.stderr.log.'
}
function OwnedProcess($pidFile, $expectedExe) {
    if (-not (Test-Path -LiteralPath $pidFile)) { return $null }
    $process = Get-Process -Id ([int](Get-Content -LiteralPath $pidFile)) -ErrorAction SilentlyContinue
    if ($null -ne $process -and $process.Path -ne $expectedExe) { throw 'Saved PID now belongs to another executable.' }
    return $process
}

if ($Action -eq 'Setup') {
    $zip = Join-Path $state 'ollama-v0.35.1.zip'
    $url = 'https://github.com/ollama/ollama/releases/download/v0.35.1/ollama-windows-amd64.zip'
    $expectedHash = 'dc50b9ca7f9023c86525012632cd1615b093d0407987444a7f62ecab617e8e93'
    if (-not (Test-Path -LiteralPath $zip)) {
        & curl.exe -L --fail --retry 3 --output $zip $url
        if ($LASTEXITCODE -ne 0) { throw 'Runtime download failed.' }
    }
    if ((Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLower() -ne $expectedHash) { throw 'Runtime archive checksum mismatch.' }
    if (-not (Test-Path -LiteralPath $exe)) { Expand-Archive -LiteralPath $zip -DestinationPath $runtime }
    @{ runtimeVersion = '0.35.1'; archiveSha256 = $expectedHash; source = $url; model = $model } | ConvertTo-Json | Set-Content (Join-Path $state 'installation.json')
    StartServer
    & $exe pull $model
    if ($LASTEXITCODE -ne 0) { throw 'Local model download failed; rerun Setup to resume.' }
    Invoke-RestMethod "$endpoint/api/tags" | ConvertTo-Json -Depth 12 | Set-Content (Join-Path $state 'models.json')
    Write-Output 'Local-only runtime and model are ready.'
}
elseif ($Action -eq 'Start') { StartServer; Write-Output "Ollama available at $endpoint" }
elseif ($Action -eq 'Run') {
    if ($Minutes -le 0 -or $AttemptsPerTask -le 0 -or $Limit -le 0) { throw 'Worker limits must be positive.' }
    StartServer
    $models = (Invoke-RestMethod "$endpoint/api/tags").models
    if (-not ($models | Where-Object name -eq $model)) { throw 'Model is not installed yet; finish Setup first.' }
    $python = (Get-Command python.exe).Source
    $worker = OwnedProcess (Join-Path $state 'worker.pid') $python
    if ($null -ne $worker) { throw 'A local worker is already running.' }
    $stop = Join-Path $state 'STOP'
    if (Test-Path -LiteralPath $stop) { Remove-Item -LiteralPath $stop }
    $scriptArgument = '"' + (Join-Path $PSScriptRoot 'local_decomp_pipeline.py') + '"'
    $argv = @($scriptArgument, '--minutes', "$Minutes", '--attempts-per-task', "$AttemptsPerTask", '--limit', "$Limit")
    $process = Start-Process -FilePath $python -ArgumentList $argv -WorkingDirectory $repo -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $state 'worker.stdout.log') -RedirectStandardError (Join-Path $state 'worker.stderr.log')
    $process.Id | Set-Content (Join-Path $state 'worker.pid')
    Write-Output "Local worker PID $($process.Id); finite queue, maximum $Minutes minutes. Use Stop for graceful shutdown."
}
elseif ($Action -eq 'Stop') {
    'Stop requested by user' | Set-Content (Join-Path $state 'STOP')
    Write-Output 'Stop requested; allow current inference/build and transactional recovery to finish.'
}
elseif ($Action -eq 'StopServer') {
    $python = (Get-Command python.exe).Source
    if ($null -ne (OwnedProcess (Join-Path $state 'worker.pid') $python)) { throw 'Stop the worker gracefully and wait for it to exit first.' }
    $process = OwnedProcess (Join-Path $state 'server.pid') $exe
    if ($null -ne $process) { Stop-Process -Id $process.Id }
    Write-Output 'Owned Ollama server stopped.'
}
else {
    Write-Output "Server ready: $(ServerReady)"
    if (ServerReady) { & $exe ps }
    $python = (Get-Command python.exe).Source
    $worker = OwnedProcess (Join-Path $state 'worker.pid') $python
    Write-Output "Worker running: $($null -ne $worker)"
    if (Test-Path (Join-Path $state 'pipeline-latest.json')) { Get-Content (Join-Path $state 'pipeline-latest.json') }
    if (Test-Path (Join-Path $state 'latest.json')) {
        $latest = Get-Content (Join-Path $state 'latest.json') -Raw | ConvertFrom-Json
        $latest | Select-Object started_at,elapsed_seconds,stop_reason,local_output_tokens,verified_dol_sha1 | Format-List
        Write-Output "Completed proposals: $($latest.attempts.Count); verified review candidates: $($latest.verified_proposals.Count)"
    }
    if (Test-Path (Join-Path $state 'error.json')) { Get-Content (Join-Path $state 'error.json') }
}
