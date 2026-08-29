$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$adcSource = Get-Content -Raw -Path (Join-Path $projectRoot 'Hardware\hw_adc.c')
$oscSource = Get-Content -Raw -Path (Join-Path $projectRoot 'APP\osc_task.c')
$keilProject = Get-Content -Raw -Path (Join-Path $projectRoot 'Project\Oscilloscope.uvprojx')
$readme = Get-Content -Raw -Path (Join-Path $projectRoot 'Doc\ECG_README.md')
$mainSource = Get-Content -Raw -Path (Join-Path $projectRoot 'User\main.c')

if ($adcSource -notmatch 'dma_circulation_disable\s*\(\s*DMA_CH0\s*\)') {
    throw 'Oscilloscope frame DMA must be configured as one-shot.'
}

if ($oscSource -match 'TFT_ShowChinese\s*\([^\r\n]*"') {
    throw 'Chinese LCD text must use explicit GBK byte arrays, not source-encoded literals.'
}

if ($oscSource -match 'sprintf\s*\(\s*showData\s*,\s*"%3dHz\s*"\s*,\s*freq\s*\)') {
    throw 'uint32_t input frequency must not be formatted with %d.'
}

if ($keilProject -notmatch '<PackID>GigaDevice\.GD32E23x_DFP\.1\.0\.2</PackID>') {
    throw 'Keil project must reference the verified GD32E23x DFP 1.0.2 package.'
}

if ($readme -notmatch 'GD32E23x_DFP\s+1\.0\.2') {
    throw 'README must document the exact Keil device-pack prerequisite.'
}

if ($keilProject -notmatch '<FilePath>\.\.\\APP\\signal_gen_task\.c</FilePath>') {
    throw 'Keil project must compile the signal generator task.'
}

if (($mainSource -notmatch 'mode_signal_generator') -or
    ($mainSource -notmatch 'SignalGen_Start\s*\(') -or
    ($mainSource -notmatch 'SignalGen_Stop\s*\(')) {
    throw 'Main application must integrate the signal generator mode lifecycle.'
}

$nonAsciiCodeNames = Get-ChildItem -Path (Join-Path $projectRoot 'APP'),
                                      (Join-Path $projectRoot 'Middle'),
                                      (Join-Path $projectRoot 'Hardware'),
                                      (Join-Path $projectRoot 'User') -Recurse |
    Where-Object { $_.Name -match '[^\x00-\x7F]' }
if ($nonAsciiCodeNames) {
    throw 'Code directory and file names must use ASCII naming only.'
}

Write-Host 'Firmware configuration contract tests passed.'
