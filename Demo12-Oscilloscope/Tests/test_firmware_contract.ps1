$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Get-Content -Raw (Join-Path $root 'Project\Demo12_Oscilloscope.uvprojx')
$main = Get-Content -Raw (Join-Path $root 'User\main.c')
$adc = Get-Content -Raw (Join-Path $root 'Hardware\hw_adc.c')

if ($project -notmatch '<OutputName>Demo12_Oscilloscope</OutputName>') { throw 'Wrong output name' }
if ($project -notmatch '\.\.\\APP\\osc_task\.c' -or $project -notmatch '\.\.\\APP\\osc_window\.c') { throw 'Oscilloscope sources missing from project' }
if ($project -match 'ecg_task|signal_gen_task') { throw 'Unrelated APP source in Demo12 project' }
if ($main -notmatch 'osc_waveShow\s*\(' -or $main -match 'ECG_|SignalGen_') { throw 'Demo12 main is not standalone' }
if ($adc -notmatch 'dma_circulation_disable\s*\(\s*DMA_CH0\s*\)') { throw 'ADC DMA must be one-shot' }
if ($project -notmatch 'GigaDevice\.GD32E23x_DFP\.1\.1\.0') { throw 'Wrong device pack' }
if (-not (Test-Path (Join-Path $root 'Spec\Demo12_OSCILLOSCOPE_SPEC.md'))) { throw 'Demo12 spec missing' }
if (Get-ChildItem (Join-Path $root 'APP'), (Join-Path $root 'User') -Recurse | Where-Object { $_.Name -match '[^\x00-\x7F]' }) { throw 'Non-ASCII code name' }
Write-Host 'Demo12 firmware contract passed.'
