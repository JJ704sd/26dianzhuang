$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot

function Require-Match {
    param(
        [string]$RelativePath,
        [string]$Pattern,
        [string]$Message
    )

    $path = Join-Path $projectRoot $RelativePath
    $content = Get-Content -Path $path -Raw
    if ($content -notmatch $Pattern) {
        throw "FAIL: $Message ($RelativePath)"
    }
}

function Require-NoMatch {
    param(
        [string]$RelativePath,
        [string]$Pattern,
        [string]$Message
    )

    $path = Join-Path $projectRoot $RelativePath
    $content = Get-Content -Path $path -Raw
    if ($content -match $Pattern) {
        throw "FAIL: $Message ($RelativePath)"
    }
}

Require-Match 'Hardware\ADC\adc.c' 'dma_circulation_disable\(DMA_CH0\)' 'DMA capture must be single-frame'
Require-Match 'Hardware\ADC\adc.c' 'value\s*>=\s*ADC_VALUE_NUM' 'ADC index check must reject count itself'
Require-NoMatch 'User\main.c' 'dma_transfer_number_config|dma_channel_enable' 'main must restart capture through the ADC API'
Require-Match 'Hardware\FREQ\freq.c' 'timer_interrupt_enable\(TIMER2,\s*TIMER_INT_UP\)' 'frequency capture must track counter wraps'
Require-Match 'Hardware\FREQ\freq.c' 'FREQ_TIMEOUT_WRAPS' 'frequency capture must clear stale readings'
Require-Match 'Hardware\TIMER\timer.c' 'period\s*-\s*1U' 'PWM ARR must use period minus one'
Require-Match 'Hardware\TFT\tft.c' 'text_scope_title\[\].*0xBC,0xF2' 'Chinese UI text must use explicit GBK bytes'
Require-Match 'Hardware\TFT\tft.c' '%3luHz' 'uint32 frequency must use a matching format'
Require-Match 'Project\Oscilloscope.uvprojx' '\.\.\\Middle' 'Keil project must include the tested math modules'
Require-Match 'Project\Oscilloscope.uvprojx' '<FileName>scope_math\.c</FileName>' 'Keil project must compile scope math'
Require-Match 'Project\Oscilloscope.uvprojx' '<FileName>timer_math\.c</FileName>' 'Keil project must compile timer math'

'firmware contract tests passed'
