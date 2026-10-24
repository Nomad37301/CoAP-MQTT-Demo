$ErrorActionPreference = 'Stop'
$taskGroups = @(
    @{
        Template = 'demo_tombol\NetworkConfig.template.h'
        Sketches = @('01_button_server', '02_controller_s3', '03_led_receiver')
        Folder = 'demo_tombol'
    },
    @{
        Template = 'demo_mqtt\NetworkConfig.template.h'
        Sketches = @('01_button_publisher', '02_led_subscriber', '03_oled_monitor_s3')
        Folder = 'demo_mqtt'
    }
)
foreach ($taskGroup in $taskGroups) {
    $taskTemplate = Join-Path $PSScriptRoot $taskGroup.Template
    foreach ($taskSketch in $taskGroup.Sketches) {
        $taskDestination = Join-Path $PSScriptRoot ($taskGroup.Folder + '\' + $taskSketch + '\NetworkConfig.h')
        if (Test-Path -LiteralPath $taskDestination) {
            Write-Output ('Sudah ada, dipertahankan: ' + $taskGroup.Folder + '/' + $taskSketch + '/NetworkConfig.h')
        } else {
            Copy-Item -LiteralPath $taskTemplate -Destination $taskDestination
            Write-Output ('Dibuat: ' + $taskGroup.Folder + '/' + $taskSketch + '/NetworkConfig.h')
        }
    }
}
Write-Output 'Isi SSID/password pada tab NetworkConfig.h setiap sketch sebelum upload. File tersebut diabaikan Git.'
