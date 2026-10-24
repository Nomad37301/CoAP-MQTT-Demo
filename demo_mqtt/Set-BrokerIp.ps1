param([string]$BrokerIp)
$ErrorActionPreference = 'Stop'

if (-not $BrokerIp) {
    $taskAddresses = @(
        foreach ($taskAdapter in [System.Net.NetworkInformation.NetworkInterface]::GetAllNetworkInterfaces()) {
            if ($taskAdapter.OperationalStatus -eq 'Up' -and $taskAdapter.NetworkInterfaceType -eq 'Wireless80211') {
                foreach ($taskAddress in $taskAdapter.GetIPProperties().UnicastAddresses) {
                    if ($taskAddress.Address.AddressFamily -eq 'InterNetwork' -and $taskAddress.Address.IPAddressToString -notlike '169.254.*') {
                        $taskAddress.Address.IPAddressToString
                    }
                }
            }
        }
    )
    if ($taskAddresses.Count -ne 1) {
        throw 'IP Wi-Fi tidak dapat dipilih otomatis. Gunakan -BrokerIp IP_LAPTOP pada jaringan A51.'
    }
    $BrokerIp = $taskAddresses[0]
}
$taskParsedIp = $null
if (-not [System.Net.IPAddress]::TryParse($BrokerIp, [ref]$taskParsedIp) -or $taskParsedIp.AddressFamily -ne 'InterNetwork') {
    throw 'BrokerIp harus berupa alamat IPv4.'
}
$taskConfigPaths = @(
    '01_button_publisher\NetworkConfig.h',
    '02_led_subscriber\NetworkConfig.h',
    '03_oled_monitor_s3\NetworkConfig.h'
)
foreach ($taskRelative in $taskConfigPaths) {
    $taskPath = Join-Path $PSScriptRoot $taskRelative
    $taskSource = [System.IO.File]::ReadAllText($taskPath)
    $taskSource = [regex]::Replace($taskSource, 'const char \*MQTT_HOST = "[^"]*";', ('const char *MQTT_HOST = "' + $BrokerIp + '";'))
    [System.IO.File]::WriteAllText($taskPath, $taskSource, (New-Object System.Text.UTF8Encoding($false)))
}
Write-Output ('MQTT_HOST ketiga sketch diatur ke ' + $BrokerIp + '. Upload ulang setelah perubahan IP.')
Write-Output 'Pastikan laptop dan semua ESP memakai hotspot A51 yang sama.'
return $BrokerIp
