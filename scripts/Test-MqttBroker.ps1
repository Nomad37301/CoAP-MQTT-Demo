param([string]$MosquittoDirectory)
$ErrorActionPreference = 'Stop'
$taskRoot = Join-Path (Split-Path -Parent $PSScriptRoot) '.build'
$taskBin = if ($MosquittoDirectory) { $MosquittoDirectory } elseif (Test-Path -LiteralPath (Join-Path $taskRoot 'mosquitto\mosquitto.exe')) { Join-Path $taskRoot 'mosquitto' } else { 'C:\Program Files\mosquitto' }
$taskWork = Join-Path $taskRoot 'mqtt_broker_test'
New-Item -ItemType Directory -Path $taskWork -Force | Out-Null
$taskProcesses = New-Object System.Collections.Generic.List[System.Diagnostics.Process]
$taskPort = 18883

function Start-TestProcess([string]$Name, [string]$Program, [string[]]$Arguments) {
    $taskProcess = Start-Process -FilePath (Join-Path $taskBin $Program) -ArgumentList $Arguments -WorkingDirectory $taskWork -PassThru -WindowStyle Hidden -RedirectStandardOutput (Join-Path $taskWork ($Name + '.out')) -RedirectStandardError (Join-Path $taskWork ($Name + '.err'))
    $taskProcesses.Add($taskProcess)
    return $taskProcess
}
function Wait-TestProcess([System.Diagnostics.Process]$Process) {
    if (-not $Process.WaitForExit(10000)) { throw ('Process timeout: ' + $Process.Id) }
    $Process.WaitForExit()
    $Process.Refresh()
    return $Process.ExitCode
}
function Read-TestFile([string]$Path) {
    $taskStream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    $taskReader = New-Object System.IO.StreamReader($taskStream)
    try { return $taskReader.ReadToEnd() } finally { $taskReader.Dispose() }
}
function Wait-Subscription([string]$ClientId) {
    $taskDeadline = [DateTime]::UtcNow.AddSeconds(8)
    while ([DateTime]::UtcNow -lt $taskDeadline) {
        $taskLogPath = Join-Path $taskWork 'broker.log'
        $taskLog = if (Test-Path -LiteralPath $taskLogPath) { Read-TestFile $taskLogPath } else { '' }
        if ($taskLog.Contains('Received SUBSCRIBE from ' + $ClientId)) { return }
        Start-Sleep -Milliseconds 100
    }
    throw ('Subscription timeout: ' + $ClientId)
}
function Assert-TestOutput([string]$Name, [string]$Expected) {
    $taskActual = (Read-TestFile (Join-Path $taskWork ($Name + '.out'))).Trim()
    if ($taskActual -ne $Expected) { throw ($Name + ': unexpected output: ' + $taskActual) }
}

try {
    $taskConfig = Join-Path $taskWork 'mosquitto.conf'
    [System.IO.File]::WriteAllLines($taskConfig, @('listener 18883 127.0.0.1','allow_anonymous true','persistence false','log_dest file broker.log','log_type all'))
    [System.IO.File]::WriteAllText((Join-Path $taskWork 'broker.log'), '')
    $taskBroker = Start-TestProcess 'broker' 'mosquitto.exe' @('-c', ('"' + $taskConfig + '"'), '-v')
    $taskReady = $false
    for ($taskAttempt = 0; $taskAttempt -lt 40; $taskAttempt++) {
        if ($taskBroker.HasExited) { throw (Read-TestFile (Join-Path $taskWork 'broker.err')) }
        $taskSocket = New-Object System.Net.Sockets.TcpClient
        try { $taskSocket.Connect('127.0.0.1', $taskPort); $taskReady = $true } catch {} finally { $taskSocket.Dispose() }
        if ($taskReady) { break }
        Start-Sleep -Milliseconds 100
    }
    if (-not $taskReady) { throw 'Broker startup timeout' }
    $taskCommon = @('-h','127.0.0.1','-p',"$taskPort",'-q','1')
    $taskFirst = Start-TestProcess 'led' 'mosquitto_sub.exe' ($taskCommon + @('-i','smoke-led','-t','demo/button/press','-C','1','-W','8'))
    $taskSecond = Start-TestProcess 'oled' 'mosquitto_sub.exe' ($taskCommon + @('-i','smoke-oled','-t','demo/button/press','-C','1','-W','8'))
    Wait-Subscription 'smoke-led'
    Wait-Subscription 'smoke-oled'
    & (Join-Path $taskBin 'mosquitto_pub.exe') @taskCommon -t 'demo/button/press' -m 'deadbeef:1'
    if ($LASTEXITCODE -ne 0) { throw 'Event publish failed' }
    if ((Wait-TestProcess $taskFirst) -ne 0 -or (Wait-TestProcess $taskSecond) -ne 0) { throw 'Fanout subscriber failed' }
    Assert-TestOutput 'led' 'deadbeef:1'
    Assert-TestOutput 'oled' 'deadbeef:1'
    Write-Output 'PASS QoS1 fanout: LED + OLED receive the same event'

    & (Join-Path $taskBin 'mosquitto_pub.exe') @taskCommon -t 'demo/button/count' -m '5' -r
    if ($LASTEXITCODE -ne 0) { throw 'Count publish failed' }
    $taskRetained = Start-TestProcess 'retained' 'mosquitto_sub.exe' ($taskCommon + @('-t','demo/button/count','-C','1','-W','5'))
    if ((Wait-TestProcess $taskRetained) -ne 0) { throw 'Retained subscriber failed' }
    Assert-TestOutput 'retained' '5'
    Write-Output 'PASS retained count: new subscriber receives 5'

    $taskNoOldEvent = Start-TestProcess 'no-old-event' 'mosquitto_sub.exe' ($taskCommon + @('-t','demo/button/press','-C','1','-W','1'))
    if ((Wait-TestProcess $taskNoOldEvent) -eq 0) { throw 'Unexpected retained press event' }
    Assert-TestOutput 'no-old-event' ''
    Write-Output 'PASS press is not retained: new subscriber receives no old event'

    $taskWill = Start-TestProcess 'will' 'mosquitto_sub.exe' ($taskCommon + @('-i','smoke-will','-t','unused','--will-topic','demo/led/status','--will-payload','offline','--will-qos','1','--will-retain'))
    Wait-Subscription 'smoke-will'
    Stop-Process -Id $taskWill.Id -Force
    $taskWillResult = Start-TestProcess 'will-result' 'mosquitto_sub.exe' ($taskCommon + @('-t','demo/led/status','-C','1','-W','5'))
    if ((Wait-TestProcess $taskWillResult) -ne 0) { throw 'Will subscriber failed' }
    Assert-TestOutput 'will-result' 'offline'
    Write-Output 'PASS retained Last Will: disconnected client becomes offline'
} finally {
    foreach ($taskProcess in $taskProcesses) {
        if (-not $taskProcess.HasExited) { Stop-Process -Id $taskProcess.Id -Force -ErrorAction SilentlyContinue }
    }
}
