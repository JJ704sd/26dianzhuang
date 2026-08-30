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
foreach ($source in 'scope_view.c','ecg_acq_core.c','hess_analyzer.c','hess_splash.c') {
    if ($projectText -notmatch [regex]::Escape($source)) {
        throw "Project does not link $source"
    }
}
foreach ($mode in 'DEMO15_MODE_OSCILLOSCOPE','DEMO15_MODE_ECG_MONITOR','DEMO15_MODE_HESS_ANALYZER') {
    if ($header -notmatch $mode) { throw "Missing mode $mode" }
}
if ($main -notmatch 'HESS_Splash_Show\(\)') { throw 'HESS splash is not started' }
if ($main -notmatch 'Demo15_SelectNextMode\(\)') { throw 'Three-mode selector is not wired' }
if (($main -notmatch 'key2_hold_period = get_pwm_period\(\)') -or
    ($main -notmatch 'set_pwm_period\(key2_hold_period\)')) {
    throw 'SW2 long press does not suppress the PWM short-press side effect'
}
if ($task -notmatch '#define FAST_HISTORY_SAMPLES\s+800U') {
    throw 'Fast history RAM budget changed unexpectedly'
}
if ($task -notmatch '#define ECG_HISTORY_SAMPLES\s+1250U') {
    throw 'ECG history must retain the full five-second window'
}
if ($task -match '\bAPP_MODE_(SCOPE|ECG)\b') {
    throw 'Stale two-mode identifiers remain in Demo15'
}

Write-Host 'Demo15 firmware contract passed.'
