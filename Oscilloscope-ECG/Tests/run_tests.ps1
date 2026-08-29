$ErrorActionPreference = 'Stop'

$gcc = Get-Command gcc.exe -ErrorAction Stop
$projectRoot = Split-Path -Parent $PSScriptRoot
$coreTestExe = Join-Path ([System.IO.Path]::GetTempPath()) 'gd32_ecg_core_tests.exe'
$taskTestExe = Join-Path ([System.IO.Path]::GetTempPath()) 'gd32_ecg_task_tests.exe'
$windowTestExe = Join-Path ([System.IO.Path]::GetTempPath()) 'gd32_osc_window_tests.exe'
$timerTestExe = Join-Path ([System.IO.Path]::GetTempPath()) 'gd32_timer_math_tests.exe'
$pwmTestExe = Join-Path ([System.IO.Path]::GetTempPath()) 'gd32_mid_pwm_tests.exe'

& $gcc.Source `
    -std=c11 `
    -O2 `
    -Wall `
    -Wextra `
    -Werror `
    -fanalyzer `
    -I (Join-Path $projectRoot 'Middle') `
    (Join-Path $PSScriptRoot 'test_ecg_core.c') `
    (Join-Path $projectRoot 'Middle\ecg_core.c') `
    -o $coreTestExe

if ($LASTEXITCODE -ne 0) {
    throw "ECG core compilation failed with exit code $LASTEXITCODE"
}

& $coreTestExe
if ($LASTEXITCODE -ne 0) {
    throw "ECG core tests failed with exit code $LASTEXITCODE"
}

& $gcc.Source `
    -std=c11 `
    -O2 `
    -Wall `
    -Wextra `
    -Werror `
    -fanalyzer `
    -I (Join-Path $PSScriptRoot 'stubs') `
    -I (Join-Path $projectRoot 'APP') `
    -I (Join-Path $projectRoot 'Middle') `
    (Join-Path $PSScriptRoot 'test_ecg_task.c') `
    (Join-Path $projectRoot 'APP\ecg_task.c') `
    (Join-Path $projectRoot 'Middle\ecg_core.c') `
    -o $taskTestExe

if ($LASTEXITCODE -ne 0) {
    throw "ECG task compilation failed with exit code $LASTEXITCODE"
}

& $taskTestExe
if ($LASTEXITCODE -ne 0) {
    throw "ECG task tests failed with exit code $LASTEXITCODE"
}

& $gcc.Source `
    -std=c11 `
    -O2 `
    -Wall `
    -Wextra `
    -Werror `
    -fanalyzer `
    -I (Join-Path $projectRoot 'APP') `
    (Join-Path $PSScriptRoot 'test_osc_window.c') `
    (Join-Path $projectRoot 'APP\osc_window.c') `
    -o $windowTestExe

if ($LASTEXITCODE -ne 0) {
    throw "Oscilloscope window compilation failed with exit code $LASTEXITCODE"
}

& $windowTestExe
if ($LASTEXITCODE -ne 0) {
    throw "Oscilloscope window tests failed with exit code $LASTEXITCODE"
}

& $gcc.Source `
    -std=c11 `
    -O2 `
    -Wall `
    -Wextra `
    -Werror `
    -fanalyzer `
    -I (Join-Path $projectRoot 'Middle') `
    (Join-Path $PSScriptRoot 'test_timer_math.c') `
    (Join-Path $projectRoot 'Middle\timer_math.c') `
    -o $timerTestExe

if ($LASTEXITCODE -ne 0) {
    throw "Timer math compilation failed with exit code $LASTEXITCODE"
}

& $timerTestExe
if ($LASTEXITCODE -ne 0) {
    throw "Timer math tests failed with exit code $LASTEXITCODE"
}

& $gcc.Source `
    -std=c11 `
    -O2 `
    -Wall `
    -Wextra `
    -Werror `
    -fanalyzer `
    -I (Join-Path $PSScriptRoot 'stubs') `
    -I (Join-Path $projectRoot 'Middle') `
    (Join-Path $PSScriptRoot 'test_mid_pwm.c') `
    (Join-Path $projectRoot 'Middle\mid_pwm.c') `
    -o $pwmTestExe

if ($LASTEXITCODE -ne 0) {
    throw "PWM compilation failed with exit code $LASTEXITCODE"
}

& $pwmTestExe
if ($LASTEXITCODE -ne 0) {
    throw "PWM tests failed with exit code $LASTEXITCODE"
}

& (Join-Path $PSScriptRoot 'run_signal_gen_tests.ps1')

& (Join-Path $PSScriptRoot 'test_firmware_contract.ps1')
