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

Invoke-NativeTest 'demo13_scope_view' @(
    (Join-Path $PSScriptRoot 'test_scope_view.c'),
    (Join-Path $root 'Middle\scope_view.c')
) @((Join-Path $root 'Middle'))

Invoke-NativeTest 'demo13_signal_output' @(
    (Join-Path $PSScriptRoot 'test_signal_output.c'),
    (Join-Path $root 'APP\signal_output.c')
) @((Join-Path $PSScriptRoot 'stubs'), (Join-Path $root 'APP'), (Join-Path $root 'Middle'))

Invoke-NativeTest 'demo13_mid_pwm' @(
    (Join-Path $PSScriptRoot 'test_mid_pwm.c'),
    (Join-Path $root 'Middle\mid_pwm.c')
) @((Join-Path $PSScriptRoot 'stubs'), (Join-Path $root 'Middle'))

Write-Host 'Demo13 baseline-extension tests passed.'
