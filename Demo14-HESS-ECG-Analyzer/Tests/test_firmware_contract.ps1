$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

function Assert-Contract([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}

function Get-Code([string]$relativePath) {
    $path = Join-Path $root $relativePath
    Assert-Contract (Test-Path $path) "Required project file missing: $path"
    $code = Get-Content -Raw $path
    $code = [regex]::Replace($code, '/\*.*?\*/', '',
        [System.Text.RegularExpressions.RegexOptions]::Singleline)
    return [regex]::Replace($code, '//[^\r\n]*', '')
}

function Get-FunctionBody([string]$code, [string]$functionName) {
    $match = [regex]::Match($code, "\b$([regex]::Escape($functionName))\s*\([^;]*?\)\s*\{")
    if (-not $match.Success) { return $null }
    $open = $match.Index + $match.Length - 1
    $depth = 0
    for ($i = $open; $i -lt $code.Length; ++$i) {
        if ($code[$i] -eq '{') { ++$depth }
        elseif ($code[$i] -eq '}') {
            --$depth
            if ($depth -eq 0) { return $code.Substring($open + 1, $i - $open - 1) }
        }
    }
    return $null
}

$projectPath = Join-Path $root 'Project\Demo14_HESS_ECG_Analyzer.uvprojx'
Assert-Contract (Test-Path $projectPath) "Project file missing: $projectPath"
[xml]$project = Get-Content -Raw $projectPath
$projectFiles = @($project.SelectNodes('//FilePath') | ForEach-Object {
    $_.InnerText.Replace('/', '\').ToLowerInvariant()
})
$outputName = [string]$project.Project.Targets.Target.TargetOption.TargetCommonOption.OutputName

$main = Get-Code 'User\main.c'
$task = Get-Code 'APP\ecg_acq_task.c'
$taskHeader = Get-Code 'APP\ecg_acq_task.h'
$ui = Get-Code 'APP\ecg_monitor_ui.c'
$uiHeader = Get-Code 'APP\ecg_monitor_ui.h'
$coreHeader = Get-Code 'Middle\ecg_acq_core.h'
$timer = Get-Code 'Middle\mid_timer.c'
$timerHeader = Get-Code 'Middle\mid_timer.h'
$middleAdc = Get-Code 'Middle\mid_adc.c'
$hardwareAdc = Get-Code 'Hardware\hw_adc.c'
$hardwareTimer = Get-Code 'Hardware\hw_tim.c'
$lcdHardware = Get-Code 'Hardware\hw_lcdinit.c'
$lcdHardwareHeader = Get-Code 'Hardware\hw_lcdinit.h'
$lcdMiddle = Get-Code 'Middle\mid_lcd.c'
$splash = Get-Code 'APP\hess_splash.c'
$splashHeader = Get-Code 'APP\hess_splash.h'

Assert-Contract ($outputName -eq 'Demo14_HESS_ECG_Analyzer') 'Wrong output name'
foreach ($source in @('..\middle\ecg_acq_core.c', '..\app\ecg_acq_task.c',
                       '..\app\ecg_monitor_ui.c', '..\app\hess_splash.c')) {
    Assert-Contract ($projectFiles -contains $source) "Project source missing: $source"
}
Assert-Contract (-not ($projectFiles -match '(?:mid_pwm|timer_math|signal_gen_task)\.c')) 'Non-ECG generator/capture source remains in project'

$pageEnum = [regex]::Match($uiHeader,
    'typedef\s+enum\s*\{(?<body>.*?)\}\s*ecg_monitor_page_t\s*;',
    [System.Text.RegularExpressions.RegexOptions]::Singleline)
Assert-Contract ($pageEnum.Success -and
                 $pageEnum.Groups['body'].Value -match '\bECG_MONITOR_PAGE\b' -and
                 $pageEnum.Groups['body'].Value -match '\bECG_WAVE_PAGE\b' -and
                 $pageEnum.Groups['body'].Value -notmatch '\bECG_FREQ_PAGE\b') 'Demo14 must expose only monitor and ECG wave pages'
Assert-Contract ($uiHeader -notmatch '\bpwm_(?:enabled|target_hz|measured_hz)\b') 'UI still exposes PWM state'
Assert-Contract ($task -notmatch '\b(?:ADC_Stream|fast_wave|ECG_PWM|set_pwm_|get_pwm_)') 'Task still depends on fast-wave or PWM code'
Assert-Contract ($main -notmatch '\b(?:ADC_StreamInit|mx_tim2_init|mx_tim14_init|TIMER2|TIMER14)\b') 'Main still starts PWM, capture, or fast-DMA services'
Assert-Contract ($timer -notmatch '\bTIMER2_IRQHandler\b' -and
                 $timerHeader -notmatch '\bget_freq_value\b') 'Runtime frequency capture remains enabled'

Assert-Contract ($task -match 'ECG_ACQ_BUFFER_SIZE\s+1250U' -and
                 $taskHeader -match 'ECG_ACQ_WINDOW_MAX_SECONDS\s+5U' -and
                 $task -match 'window_seconds\s*=\s*2U') 'ECG history must use the 2/5 second bounded memory budget'
Assert-Contract ($coreHeader -match 'ECG_ACQ_SAMPLE_RATE_HZ\s+250U' -and
                 $coreHeader -notmatch 'ECG_DISPLAY_(?:SAMPLE_RATE_HZ|WAVE_WINDOW_MS)') 'ECG-only sample-rate contract is incorrect'
Assert-Contract ($hardwareTimer -match 'ADC_ECG_TIMER_PERIOD\s+3999U' -and
                 $hardwareTimer -match 'ADC_ECG_TIMER_COMPARE\s+2000U' -and
                 $main -match '\bmx_tim0_adc_init\s*\(' -and
                 $main -match '\btimer_enable\s*\(\s*TIMER0\s*\)') 'TIMER0 must trigger ADC at 250 Hz'
Assert-Contract ($hardwareAdc -match 'ADC_EXTTRIG_REGULAR_T0_CH0' -and
                 $hardwareAdc -notmatch '\badc_dma_mode_enable\s*\(' -and
                 $middleAdc -notmatch '\b(?:ADC_Stream|adc_scope_buffer|DMA_Channel0_IRQHandler)\b') 'High-rate ADC DMA path was not removed'

$actionBody = Get-FunctionBody $task 'ECGAcq_HandleAction'
Assert-Contract ($null -ne $actionBody -and
                 $taskHeader -match '\bECG_ACQ_ACTION_TOGGLE_RUN\b' -and
                 $taskHeader -match '\bECG_ACQ_ACTION_CYCLE_GAIN\b' -and
                 $taskHeader -match '\bECG_ACQ_ACTION_TOGGLE_PAGE\b' -and
                 $taskHeader -match '\bECG_ACQ_ACTION_MARK_EVENT\b' -and
                 $taskHeader -match '\bECG_ACQ_ACTION_RESET_MEASUREMENTS\b' -and
                 $actionBody -match '\bECGMonitorUI_DrawStatic\s*\(') 'ECG task action contract is incomplete'
Assert-Contract ($main -match '\bKEY1_Pin\b' -and
                 $main -match '\bKEY2_Pin\b' -and
                 $main -match '\bKEY3_Pin\b' -and
                 $main -match '\bKeyPress\b' -and
                 $main -match '\bKeyDoublePress\b' -and
                 $main -match '\bECGAcq_HandleAction\s*\(' -and
                 $task -notmatch '\bKEY[123]_Pin\b') 'Physical key mapping must stay in the main adapter'
Assert-Contract ($task -match '\bECGAcqCore_DisplaySample\s*\(\s*raw_sample\s*\)' -and
                 $task -match '\bECGAcqCore_MapDisplaySamples\s*\(') 'Both pages must render shared ECG history'
Assert-Contract ($uiHeader -match '\bwaveform_revision\b' -and
                 $ui -notmatch 'static\s+int16_t\s+previous_plot\[') 'Wave redraw tracking must not duplicate the plot buffer'
Assert-Contract ($ui -match '"%-8s x%u%s %us"' -and
                 $ui -notmatch 'DMA WAIT|FLAT|%ums') 'Enlarged ECG page must show seconds, not milliseconds'

$uiRefresh = [regex]::Match($main,
    'if\s*\(\s*get_tft_timer_value\(\)\s*>=\s*ECG_UI_REFRESH_MS\s*\)\s*\{(?<body>.*?)\}',
    [System.Text.RegularExpressions.RegexOptions]::Singleline)
Assert-Contract ($uiRefresh.Success -and
                 $main -match '(?m)^\s*#define\s+ECG_UI_REFRESH_MS\s+40U\s*$' -and
                 $uiRefresh.Groups['body'].Value -match '(?s)set_tft_timer_value\s*\(\s*0U\s*\).*?ECGAcq_ShowUI\s*\(') 'UI scheduler must remain a nonblocking 25 FPS loop'

Assert-Contract ($splashHeader -match '\bHESS_Splash_Show\s*\(' -and
                 $splash -match '\bHESS_Splash_Show\s*\(' -and
                 $main -match '\bHESS_Splash_Show\s*\(') 'HESS splash missing'
Assert-Contract ($lcdHardware -match '\bSPI_FLAG_TRANS\b') 'LCD SPI must wait for final shifted bit'
Assert-Contract ($lcdHardwareHeader -match '\bTFT_WriteColorBurst\s*\(' -and
                 $lcdHardwareHeader -match '\bTFT_WritePixels\s*\(' -and
                 $lcdMiddle -match '\bTFT_DrawPixelRow\s*\(' -and
                 $ui -match '\bTFT_DrawPixelRow\s*\(') 'LCD burst/scanline path missing'
Assert-Contract ($ui -notmatch '\b(?:malloc|calloc|realloc|free|float|double)\b') 'UI must use fixed memory and integer math'
Assert-Contract ($ui -match '\bTFT_ShowChinese\s*\(' -and
                 $ui -notmatch '"[^"\r\n]*[\u4e00-\u9fff][^"\r\n]*"') 'Chinese UI must use GBK byte arrays'
Assert-Contract ($uiHeader -match 'ECG_WAVE_PLOT_Y0\s+17U' -and
                 $uiHeader -match 'ECG_WAVE_PLOT_Y1\s+109U') 'WAVE plot must retain at least 90 vertical pixels'

$projectText = Get-Content -Raw $projectPath
Assert-Contract ($projectText -match 'GigaDevice\.GD32E23x_DFP\.1\.1\.0') 'Wrong device pack'
Assert-Contract (Test-Path (Join-Path $root 'Spec\Demo14_HESS_ECG_ANALYZER_SPEC.md')) 'Demo14 HESS ECG spec missing'
Write-Host 'Demo14 ECG-only firmware contract passed.'
