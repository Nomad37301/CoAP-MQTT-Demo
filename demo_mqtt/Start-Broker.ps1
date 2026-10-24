param(
    [string]$MosquittoPath,
    [string]$BrokerIp
)
$ErrorActionPreference = 'Stop'
if (-not $MosquittoPath) {
    $taskPortable = Join-Path (Split-Path -Parent $PSScriptRoot) '.build\mosquitto\mosquitto.exe'
    $MosquittoPath = if (Test-Path -LiteralPath $taskPortable) { $taskPortable } else { 'C:\Program Files\mosquitto\mosquitto.exe' }
}
if (-not (Test-Path -LiteralPath $MosquittoPath -PathType Leaf)) {
    $taskCommand = Get-Command mosquitto.exe -ErrorAction SilentlyContinue
    if ($taskCommand) { $MosquittoPath = $taskCommand.Source }
    else { throw 'Mosquitto belum ditemukan. Pasang dari https://mosquitto.org/download/ lalu jalankan ulang, atau isi -MosquittoPath.' }
}

$taskResult = @(& (Join-Path $PSScriptRoot 'Set-BrokerIp.ps1') -BrokerIp $BrokerIp)
$taskBrokerIp = $taskResult[-1]
$taskResult | Select-Object -SkipLast 1 | Write-Output
$taskConfigPath = Join-Path $PSScriptRoot 'mosquitto.runtime.conf'
$taskConfig = @(
    '# Demo lokal: listener dibatasi pada IP Wi-Fi laptop yang dipilih.',
    ('listener 1883 ' + $taskBrokerIp),
    'allow_anonymous true',
    'persistence false',
    'log_dest stdout',
    'log_type error',
    'log_type warning',
    'log_type notice',
    'log_type information'
)
[System.IO.File]::WriteAllLines($taskConfigPath, $taskConfig, (New-Object System.Text.UTF8Encoding($false)))
Write-Output ('Broker MQTT: ' + $taskBrokerIp + ':1883. Biarkan terminal ini terbuka. Ctrl+C untuk berhenti.')
& $MosquittoPath -c $taskConfigPath -v
if ($LASTEXITCODE -ne 0) { throw 'Broker berhenti dengan error. Periksa pesan Mosquitto di atas (misalnya port sudah digunakan).' }
