$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$outputDirectory = Join-Path $projectRoot 'build/host-tests'
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

$cases = @(
    @{ Name = 'bms_protection'; Sources = @('Tests/test_bms_protection.c', 'Core/Src/bms_protection.c') },
    @{ Name = 'bms_injection'; Sources = @('Tests/test_bms_injection.c', 'Core/Src/bms_injection.c', 'Core/Src/bms_protection.c') },
    @{ Name = 'uart_protocol'; Sources = @('Tests/test_uart_protocol.c', 'Core/Src/uart_protocol.c') },
    @{ Name = 'can_protocol'; Sources = @('Tests/test_can_protocol.c', 'Core/Src/can_protocol.c') },
    @{ Name = 'can_rx_queue'; Sources = @('Tests/test_can_rx_queue.c', 'Core/Src/can_rx_queue.c') }
)

foreach ($case in $cases) {
    $exe = Join-Path $outputDirectory ($case.Name + '.exe')
    $sources = @($case.Sources | ForEach-Object { Join-Path $projectRoot $_ })
    & gcc -std=c11 -Wall -Wextra -Werror -I (Join-Path $projectRoot 'Core/Inc') @sources -o $exe
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $($case.Name)" }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Test failed: $($case.Name)" }
}
