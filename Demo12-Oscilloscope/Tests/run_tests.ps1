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

Invoke-NativeTest 'demo12_osc_window' @(
    (Join-Path $PSScriptRoot 'test_osc_window.c'),
    (Join-Path $root 'APP\osc_window.c')
) @((Join-Path $root 'APP'))

Invoke-NativeTest 'demo12_timer_math' @(
    (Join-Path $PSScriptRoot 'test_timer_math.c'),
    (Join-Path $root 'Middle\timer_math.c')
) @((Join-Path $root 'Middle'))

Invoke-NativeTest 'demo12_mid_pwm' @(
    (Join-Path $PSScriptRoot 'test_mid_pwm.c'),
    (Join-Path $root 'Middle\mid_pwm.c')
) @((Join-Path $PSScriptRoot 'stubs'), (Join-Path $root 'Middle'))

& (Join-Path $PSScriptRoot 'test_firmware_contract.ps1')
Write-Host 'Demo12 standalone tests passed.'
