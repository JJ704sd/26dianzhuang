$ErrorActionPreference = 'Stop'
$gcc = Get-Command gcc.exe -ErrorAction Stop
$root = Split-Path -Parent $PSScriptRoot

function Invoke-NativeTest($name, $sources, $includes) {
    $exe = Join-Path ([System.IO.Path]::GetTempPath()) ($name + '.exe')
    $args = @('-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-fanalyzer')
    foreach ($include in $includes) { $args += @('-I', $include) }
    $args += $sources
    $args += @('-o', $exe)
    & $gcc.Source @args
    if ($LASTEXITCODE -ne 0) { throw "$name compilation failed" }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "$name failed" }
}

Write-Host '[1/5] Oscilloscope view tests'
Invoke-NativeTest 'demo15_scope_view' @(
    (Join-Path $PSScriptRoot 'test_scope_view.c'),
    (Join-Path $root 'Middle\scope_view.c')
) @((Join-Path $root 'Middle'))

Write-Host '[2/5] Retained signal-output module tests'
Invoke-NativeTest 'demo15_signal_output' @(
    (Join-Path $PSScriptRoot 'test_signal_output.c'),
    (Join-Path $root 'APP\signal_output.c')
) @((Join-Path $PSScriptRoot 'stubs'), (Join-Path $root 'APP'), (Join-Path $root 'Middle'))

Write-Host '[3/5] PWM tests'
Invoke-NativeTest 'demo15_mid_pwm' @(
    (Join-Path $PSScriptRoot 'test_mid_pwm.c'),
    (Join-Path $root 'Middle\mid_pwm.c')
) @((Join-Path $PSScriptRoot 'stubs'), (Join-Path $root 'Middle'))

Write-Host '[4/5] HESS ECG analysis core tests'
Invoke-NativeTest 'demo15_ecg_acq_core' @(
    (Join-Path $PSScriptRoot 'test_ecg_acq_core.c'),
    (Join-Path $root 'Middle\ecg_acq_core.c')
) @((Join-Path $root 'Middle'))

Write-Host '[5/5] Demo15 firmware contract'
& (Join-Path $PSScriptRoot 'test_firmware_contract.ps1')

Write-Host 'Demo15 multi-function monitor tests passed.'
