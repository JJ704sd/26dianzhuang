$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Get-Content -Raw (Join-Path $root 'Project\Demo13_ECG_Monitor.uvprojx')
$main = Get-Content -Raw (Join-Path $root 'User\main.c')
$source = Get-Content -Raw (Join-Path $root 'APP\ecg_task.c')

if ($project -notmatch '<OutputName>Demo13_ECG_Monitor</OutputName>') { throw 'Wrong output name' }
if ($project -notmatch '\.\.\\APP\\ecg_task\.c' -or $project -notmatch '\.\.\\Middle\\ecg_core\.c') { throw 'ECG sources missing from project' }
if ($project -match 'osc_task|osc_window|signal_gen_task') { throw 'Unrelated APP source in Demo13 project' }
if ($main -notmatch 'ECG_Start\s*\(' -or $main -match 'osc_waveShow|SignalGen_') { throw 'Demo13 main is not standalone' }
if ($source -notmatch 'ECG_GetSignalQuality' -or $source -notmatch 'ECG_GetAlarmFlags') { throw 'Quality or alarm feature missing' }
if ($project -notmatch 'GigaDevice\.GD32E23x_DFP\.1\.0\.2') { throw 'Wrong device pack' }
if (-not (Test-Path (Join-Path $root 'Spec\Demo13_ECG_MONITOR_SPEC.md'))) { throw 'Demo13 spec missing' }
if (Get-ChildItem (Join-Path $root 'APP'), (Join-Path $root 'User') -Recurse | Where-Object { $_.Name -match '[^\x00-\x7F]' }) { throw 'Non-ASCII code name' }
Write-Host 'Demo13 firmware contract passed.'
