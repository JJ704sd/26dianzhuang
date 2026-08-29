$ErrorActionPreference = 'Stop'

$gcc = Get-Command gcc.exe -ErrorAction Stop
$projectRoot = Split-Path -Parent $PSScriptRoot
$testExe = Join-Path ([System.IO.Path]::GetTempPath()) 'gd32_signal_gen_task_tests.exe'

& $gcc.Source `
    -std=c11 `
    -O2 `
    -Wall `
    -Wextra `
    -Werror `
    -fanalyzer `
    -I (Join-Path $PSScriptRoot 'stubs') `
    -I (Join-Path $projectRoot 'APP') `
    (Join-Path $PSScriptRoot 'test_signal_gen_task.c') `
    (Join-Path $projectRoot 'APP\signal_gen_task.c') `
    -o $testExe

if ($LASTEXITCODE -ne 0) {
    throw "Signal generator compilation failed with exit code $LASTEXITCODE"
}

& $testExe
if ($LASTEXITCODE -ne 0) {
    throw "Signal generator tests failed with exit code $LASTEXITCODE"
}
