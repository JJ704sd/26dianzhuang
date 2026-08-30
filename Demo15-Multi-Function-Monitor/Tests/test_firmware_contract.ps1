$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Project\Demo15_Multi_Function_Monitor.uvprojx'
$main = Get-Content -LiteralPath (Join-Path $root 'User\main.c') -Raw
$header = Get-Content -LiteralPath (Join-Path $root 'APP\osc_task.h') -Raw
$task = Get-Content -LiteralPath (Join-Path $root 'APP\osc_task.c') -Raw
$projectText = Get-Content -LiteralPath $project -Raw

if ((Split-Path -Leaf $root) -ne 'Demo15-Multi-Function-Monitor') {
    throw 'Demo15 directory does not follow DemoNN-Title-Case naming'
}
if ($projectText -notmatch '<OutputName>Demo15_Multi_Function_Monitor</OutputName>') {
    throw 'Keil output name is not Demo15_Multi_Function_Monitor'
}
if ($projectText -notmatch 'GigaDevice\.GD32E23x_DFP\.1\.1\.0') {
    throw 'Demo15 must use the current GD32E23x DFP 1.1.0 pack'
}
if ($projectText -notmatch '-Oz') {
    throw 'Demo15 must optimize for the Community linker image limit'
}
foreach ($source in 'scope_view.c','scope_metrics.c','ecg_acq_core.c','hess_analyzer.c','hess_splash.c') {
    if ($projectText -notmatch [regex]::Escape($source)) {
        throw "Project does not link $source"
    }
}
foreach ($mode in 'DEMO15_MODE_OSCILLOSCOPE','DEMO15_MODE_ECG_MONITOR','DEMO15_MODE_SPO2_MONITOR') {
    if ($header -notmatch $mode) { throw "Missing mode $mode" }
}
if ($main -notmatch 'HESS_Splash_Show\(\)') { throw 'Startup animation is not retained' }
if ($main -notmatch 'Demo15_SelectNextMode\(\)') { throw 'Three-mode selector is not wired' }
if (($main -notmatch 'led_turn_on\(&led_handle\[led1\]\)') -or
    ($main -notmatch 'update_status_led\(\)')) {
    throw 'Power/alive and PWM status LED feedback is not wired'
}
if (($main -notmatch 'key2_hold_period = get_pwm_period\(\)') -or
    ($main -notmatch 'set_pwm_period\(key2_hold_period\)')) {
    throw 'SW2 long press does not suppress the PWM short-press side effect'
}
if ($task -notmatch '#define FAST_HISTORY_SAMPLES\s+800U') {
    throw 'Fast history RAM budget changed unexpectedly'
}
if ($task -notmatch '#define SLOW_HISTORY_SAMPLES\s+1250U') {
    throw 'Slow scope history must retain a five-second 250 Sa/s window'
}
if ($task -notmatch '#define ECG_HISTORY_SAMPLES\s+1250U') {
    throw 'ECG history must retain the full five-second window'
}
if (($task -notmatch '250U, 500U, 1000U, 1250U') -or
    ($task -notmatch '"1\.0s", "2\.0s", "4\.0s", "5\.0s"') -or
    ($task -match '"6\.0s"')) {
    throw 'ECG timebase must not exceed its five-second history'
}
if ($task -match '\bAPP_MODE_(SCOPE|ECG)\b') {
    throw 'Stale two-mode identifiers remain in Demo15'
}
if (($task -notmatch '\? "SpO2" : "ECG "') -or
    ($task -notmatch '\? "SIM" : "PWM"') -or
    ($task -notmatch '\(const uint8_t \*\)"PPG"')) {
    throw 'Simulated SpO2/PPG mode is not clearly identified in the UI'
}
if ($task -match 'DEMO15_MODE_HESS_ANALYZER') {
    throw 'Retired HESS mode identifier remains in the application task'
}
if (($task -notmatch 'ScopeMetrics_Analyze\(') -or
    ($task -notmatch '\(const uint8_t \*\)"DIN"')) {
    throw 'Input duty measurement is not connected to the scope UI'
}
if (($task -notmatch '#define SCOPE_STARTUP_SETTLE_SAMPLES\s+\(ADC_SAMPLE_RATE_HZ / 2U\)') -or
    ($task -notmatch 'fast_sample_count >= SCOPE_STARTUP_SETTLE_SAMPLES') -or
    ($task -notmatch 'scope_startup_ready == 0U') -or
    ($task -notmatch '\(const uint8_t \*\)"WAIT"')) {
    throw 'Scope startup transients are not gated behind a visible settle state'
}

Write-Host 'Demo15 firmware contract passed.'
