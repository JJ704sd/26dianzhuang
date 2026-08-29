$ErrorActionPreference = 'Stop'
$gcc = Get-Command gcc.exe -ErrorAction Stop
$root = Split-Path -Parent $PSScriptRoot

function Invoke-NativeTest($name, $sources, $includes) {
    $exe = Join-Path ([System.IO.Path]::GetTempPath()) ($name + '.exe')
    $compilerArgs = @('-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-fanalyzer')
    foreach ($include in $includes) { $compilerArgs += @('-I', $include) }
    $compilerArgs += $sources
    $compilerArgs += @('-o', $exe)
    & $gcc.Source @compilerArgs
    if ($LASTEXITCODE -ne 0) { throw "$name compilation failed" }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "$name failed" }
}

Write-Host '[1/6] ECG core host tests'
Invoke-NativeTest 'demo14_ecg_acq_core' @(
    (Join-Path $PSScriptRoot 'test_ecg_acq_core.c'),
    (Join-Path $root 'Middle\ecg_acq_core.c')
) @((Join-Path $root 'Middle'))

Write-Host '[2/6] ECG monitor UI host tests'
Invoke-NativeTest 'demo14_ecg_monitor_ui' @(
    (Join-Path $PSScriptRoot 'test_ecg_monitor_ui.c'),
    (Join-Path $root 'APP\ecg_monitor_ui.c')
) @((Join-Path $PSScriptRoot 'stubs'),
    (Join-Path $root 'APP'),
    (Join-Path $root 'Middle'))

Write-Host '[3/6] Chinese UI font tests'
Invoke-NativeTest 'demo14_chinese_font' @(
    (Join-Path $PSScriptRoot 'test_chinese_font.c')
) @((Join-Path $root 'Middle'))

Write-Host '[4/6] PWM host tests'
Invoke-NativeTest 'demo14_mid_pwm' @(
    (Join-Path $PSScriptRoot 'test_mid_pwm.c'),
    (Join-Path $root 'Middle\mid_pwm.c')
) @((Join-Path $PSScriptRoot 'stubs'), (Join-Path $root 'Middle'))

Write-Host '[5/6] Timer math and timeout tests'
Invoke-NativeTest 'demo14_timer_math' @(
    (Join-Path $PSScriptRoot 'test_timer_math.c'),
    (Join-Path $root 'Middle\timer_math.c')
) @((Join-Path $root 'Middle'))

Write-Host '[6/6] Hospital monitor firmware contract'
& (Join-Path $PSScriptRoot 'test_firmware_contract.ps1')
Write-Host 'Demo14 ECG, PWM frequency monitor, and UI tests passed.'
