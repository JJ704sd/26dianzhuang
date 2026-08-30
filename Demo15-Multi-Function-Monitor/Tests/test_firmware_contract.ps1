$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Project\Demo15_Multi_Function_Monitor.uvprojx'
$main = Get-Content -LiteralPath (Join-Path $root 'User\main.c') -Raw
$header = Get-Content -LiteralPath (Join-Path $root 'APP\osc_task.h') -Raw
$task = Get-Content -LiteralPath (Join-Path $root 'APP\osc_task.c') -Raw
$scopeView = Get-Content -LiteralPath (Join-Path $root 'Middle\scope_view.c') -Raw
$signalOutput = Get-Content -LiteralPath (Join-Path $root 'APP\signal_output.c') -Raw
$hardwareTimer = Get-Content -LiteralPath (Join-Path $root 'Hardware\hw_tim.c') -Raw
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
foreach ($source in 'scope_view.c','vital_trace.c','scope_metrics.c','ecg_acq_core.c','spo2_core.c','spo2_receiver.c','hess_analyzer.c','hess_splash.c','signal_output.c') {
    if ($projectText -notmatch [regex]::Escape($source)) {
        throw "Project does not link $source"
    }
}
foreach ($mode in 'DEMO15_MODE_OSCILLOSCOPE','DEMO15_MODE_ECG_MONITOR','DEMO15_MODE_SPO2_MONITOR') {
    if ($header -notmatch $mode) { throw "Missing mode $mode" }
}
if ($main -notmatch 'HESS_Splash_Show\(\)') { throw 'Startup animation is not retained' }
if ($main -notmatch 'Demo15_SelectNextMode\(\)') { throw 'Three-mode selector is not wired' }
if (($main -notmatch 'SignalOutput_Init\(\)') -or
    ($main -notmatch 'SignalOutput_Tick1ms\(\)')) {
    throw 'PWM ECG generator is not initialized and serviced by the application 1 ms callback'
}
if (($signalOutput -notmatch '#define PWM_DAC_PERIOD_TICKS\s+59U') -or
    ($hardwareTimer -notmatch 'timer_initpara\.period\s*=\s*49U')) {
    throw 'ECG PWM carrier must not be phase-locked to the 50 us ADC trigger'
}
if ($hardwareTimer -notmatch 'GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO_PIN_6') {
    throw 'Disconnected PA6 wavelength tag must have a defined low state'
}
if (($main -notmatch 'ECG_VIEW_HOLD_MS\s+2000U') -or
    ($main -notmatch 'toggle_scope_ecg_view\(\)') -or
    ($header -notmatch 'Demo15_IsScopeEcgView')) {
    throw 'Scope ECG subview is not wired to a deliberate long-press action'
}
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
if (($task -notmatch 'SignalOutput_SelectEcg\(\)') -or
    ($task -notmatch 'SignalOutput_ToggleEcgPreset\(\)') -or
    ($task -notmatch '\(const uint8_t \*\)title') -or
    ($task -notmatch '"S-ECG"')) {
    throw '60/80 BPM ECG generator or scope ECG display bridge is missing'
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
    ($task -notmatch '"DUT"') -or
    ($task -notmatch '"2CH"') -or
    ($task -notmatch '\? "O2%" : "OUT"') -or
    ($task -notmatch 'SpO2Core_ReconstructInputSample\(') -or
    ($task -notmatch 'SpO2Receiver_ProcessSample\(') -or
    ($task -notmatch 'SpO2Receiver_GetResult\(')) {
    throw 'Simulated SpO2/PPG mode is not clearly identified in the UI'
}
if ($task -notmatch 'gpio_input_bit_get\(GPIOA, GPIO_PIN_6\)') {
    throw 'Duty-coded PA3 samples or optional PA6 RED/IR tagging is missing'
}
if ($task -match 'spo2_result\.valid[^\r\n]*signal_ok') {
    throw 'A valid SpO2 calculation must not be hidden while BPM is waiting'
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
if (($task -notmatch '5000U, 1000U, 200U') -or
    ($task -notmatch '"5Vpp", "1Vpp", "200m"') -or
    ($task -notmatch 'scope_range_index\+\+')) {
    throw 'Scope must provide the SW1-cycled 5 Vpp, 1 Vpp and 200 mVpp ranges'
}
if (($task -notmatch '#define SCOPE_NOISE_WAVE_WIDTH\s+156U') -or
    ($task -notmatch 'scope_noise_zoom_active\(\)') -or
    ($task -notmatch '"NOISE"') -or
    ($task -notmatch 'toggle_scope_small_signal[\s\S]*?TFT_StaticUI\(\)')) {
    throw 'The 200 mVpp noise range must use the full-width NOISE view and redraw immediately'
}
if (($task -notmatch 'VitalTrace_PushSample\(') -or
    ($task -notmatch 'clear_wave_column\(') -or
    ($task -notmatch 'vital_trace_reset_pending') -or
    ($task -notmatch 'vital_ui_cache\.bpm') -or
    ($task -notmatch 'vital_ui_cache\.vpp')) {
    throw 'ECG/PPG waveform must use incremental columns and change-driven status fields'
}
if (($task -notmatch 'ScopeView_CopyUniformWindow\(') -or
    ($task -notmatch 'scope_noise_zoom_active\(\) != 0U')) {
    throw 'Ordinary square-wave display must use uniform samples while NOISE retains peak detection'
}
if (($scopeView -notmatch 'history_count - source_count') -or
    ($scopeView -notmatch 'source_count - lag')) {
    throw 'Scope motion must select a continuous historical window instead of rotating one frame'
}


Write-Host 'Demo15 firmware contract passed.'
