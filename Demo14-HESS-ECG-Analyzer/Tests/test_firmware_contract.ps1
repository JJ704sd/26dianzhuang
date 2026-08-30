$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

function Assert-Contract([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}

function Get-CodeWithoutComments([string]$path) {
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

function Convert-LiteralCoordinate([string]$argument) {
    $trimmed = $argument.Trim()
    if ($trimmed -match '^\(?\s*(\d+)\s*[uUlL]*\s*\)?$') { return [int]$Matches[1] }
    return $null
}

function Assert-LiteralCoordinatesInBounds([string]$code) {
    $calls = [regex]::Matches($code,
        '\bTFT_(?<name>DrawPoint|DrawLine|ShowString|Fill)\s*\((?<args>[^;]*?)\)\s*;',
        [System.Text.RegularExpressions.RegexOptions]::Singleline)
    foreach ($call in $calls) {
        $args = $call.Groups['args'].Value -split ','
        $name = $call.Groups['name'].Value
        $indices = if ($name -eq 'DrawLine' -or $name -eq 'Fill') { @(0, 1, 2, 3) } else { @(0, 1) }
        foreach ($index in $indices) {
            if ($index -ge $args.Count) { continue }
            $value = Convert-LiteralCoordinate $args[$index]
            if ($null -eq $value) { continue }
            $isX = (($index % 2) -eq 0)
            # TFT_Fill uses an exclusive right/bottom edge; other primitives use pixels.
            $limit = if ($name -eq 'Fill') {
                if ($isX) { 160 } else { 128 }
            } else {
                if ($isX) { 159 } else { 127 }
            }
            Assert-Contract ($value -le $limit) "TFT_$name contains out-of-range literal coordinate $value"
        }
        if ($name -eq 'ShowString' -and $args.Count -ge 6) {
            $fontSize = Convert-LiteralCoordinate $args[5]
            if ($null -ne $fontSize) {
                Assert-Contract ($fontSize -eq 16 -or $fontSize -eq 24) `
                    "TFT_ShowString uses unsupported ASCII font height $fontSize"
            }
        }
    }
}

$projectPath = Join-Path $root 'Project\Demo14_HESS_ECG_Analyzer.uvprojx'
$taskPath = Join-Path $root 'APP\ecg_acq_task.c'
$taskHeaderPath = Join-Path $root 'APP\ecg_acq_task.h'
$coreHeaderPath = Join-Path $root 'Middle\ecg_acq_core.h'
$uiPath = Join-Path $root 'APP\ecg_monitor_ui.c'
$uiHeaderPath = Join-Path $root 'APP\ecg_monitor_ui.h'
$pwmPath = Join-Path $root 'Middle\mid_pwm.c'
$pwmHeaderPath = Join-Path $root 'Middle\mid_pwm.h'
$timerPath = Join-Path $root 'Middle\mid_timer.c'
$timerHeaderPath = Join-Path $root 'Middle\mid_timer.h'
$hardwareTimerPath = Join-Path $root 'Hardware\hw_tim.c'
$hardwareAdcPath = Join-Path $root 'Hardware\hw_adc.c'
$middleAdcPath = Join-Path $root 'Middle\mid_adc.c'
$lcdHardwarePath = Join-Path $root 'Hardware\hw_lcdinit.c'
$lcdHardwareHeaderPath = Join-Path $root 'Hardware\hw_lcdinit.h'
$lcdMiddlePath = Join-Path $root 'Middle\mid_lcd.c'
$splashPath = Join-Path $root 'APP\hess_splash.c'
$splashHeaderPath = Join-Path $root 'APP\hess_splash.h'
$hospitalSpecPath = Join-Path $root 'Spec\Demo14_HOSPITAL_MONITOR_UI_SPEC.md'

foreach ($required in @($projectPath, $taskPath, $taskHeaderPath, $uiPath,
                         $coreHeaderPath, $hardwareAdcPath, $middleAdcPath,
                         $uiHeaderPath, $splashPath, $splashHeaderPath,
                         $pwmPath, $pwmHeaderPath, $timerPath, $timerHeaderPath,
                         $hardwareTimerPath, $lcdHardwarePath,
                         $lcdHardwareHeaderPath, $lcdMiddlePath,
                         $hospitalSpecPath)) {
    Assert-Contract (Test-Path $required) "Required project file missing: $required"
}

[xml]$project = Get-Content -Raw $projectPath
$projectFiles = @($project.SelectNodes('//FilePath') | ForEach-Object {
    $_.InnerText.Replace('/', '\').ToLowerInvariant()
})
$outputName = [string]$project.Project.Targets.Target.TargetOption.TargetCommonOption.OutputName
$main = Get-CodeWithoutComments (Join-Path $root 'User\main.c')
$task = Get-CodeWithoutComments $taskPath
$taskHeader = Get-CodeWithoutComments $taskHeaderPath
$coreHeader = Get-CodeWithoutComments $coreHeaderPath
$ui = Get-CodeWithoutComments $uiPath
$uiHeader = Get-CodeWithoutComments $uiHeaderPath
$pwm = Get-CodeWithoutComments $pwmPath
$pwmHeader = Get-CodeWithoutComments $pwmHeaderPath
$timer = Get-CodeWithoutComments $timerPath
$timerHeader = Get-CodeWithoutComments $timerHeaderPath
$hardwareTimer = Get-CodeWithoutComments $hardwareTimerPath
$hardwareAdc = Get-CodeWithoutComments $hardwareAdcPath
$middleAdc = Get-CodeWithoutComments $middleAdcPath
$lcdHardware = Get-CodeWithoutComments $lcdHardwarePath
$lcdHardwareHeader = Get-CodeWithoutComments $lcdHardwareHeaderPath
$lcdMiddle = Get-CodeWithoutComments $lcdMiddlePath
$splash = Get-CodeWithoutComments $splashPath
$splashHeader = Get-CodeWithoutComments $splashHeaderPath

Assert-Contract ($outputName -eq 'Demo14_HESS_ECG_Analyzer') 'Wrong output name'
foreach ($source in @('..\middle\ecg_acq_core.c', '..\middle\mid_pwm.c',
                       '..\app\ecg_acq_task.c',
                       '..\app\ecg_monitor_ui.c', '..\app\hess_splash.c')) {
    Assert-Contract ($projectFiles -contains $source) "Project source missing: $source"
}
Assert-Contract (-not ($projectFiles -match 'signal_gen_task')) 'Signal generator source remains in Demo14 project'

Assert-Contract ($taskHeader -match '\bECGAcq_[A-Za-z0-9_]+\s*\(' -and
                 $task -match '\bECGAcq_[A-Za-z0-9_]+\s*\(') 'ECGAcq public interface missing'
Assert-Contract ($task -match '\bECGAcqCore_DisplaySample\s*\(\s*raw_sample\s*\)' -and
                 $task -notmatch 'result\.filtered\s*/') 'Waveform history must use the undistorted display sample, not the ECG baseline filter'
Assert-Contract ($coreHeader -match 'ECG_DISPLAY_SAMPLE_RATE_HZ\s+40000U' -and
                 $coreHeader -match 'ECG_DISPLAY_WAVE_WINDOW_MS\s+5U') 'Fast waveform display must use the 40 kSa/s, 5 ms acceptance timebase'
Assert-Contract ($task -match '\bADC_StreamCopyLatestDisplay\s*\(' -and
                 $task -match '\bECGAcqCore_MapDisplaySamples\s*\(') 'WAVE page must use the high-rate DMA frame and bounded display mapper'
Assert-Contract ($task -match '\bfast_wave_samples\b' -and
                 $task -match '(?s)view\.running\s*!=\s*0U.*?ADC_StreamCopyLatestDisplay') 'WAVE DMA frame must remain frozen while RUN is off'
Assert-Contract ($splashHeader -match '\bHESS_Splash_Show\s*\(' -and
                 $splash -match '\bHESS_Splash_Show\s*\(') 'HESS splash interface missing'
Assert-Contract ($main -match '\bHESS_Splash_Show\s*\(') 'Main does not show the HESS splash'
Assert-Contract ($main -match '\bECGAcq_[A-Za-z0-9_]+\s*\(') 'Main does not start ECG acquisition'
Assert-Contract ($main -notmatch '\bSignalGen_') 'Signal generator calls remain in main'
Assert-Contract ($main -match '\bmx_tim2_init\s*\(' -and
                 $main -match '\bmx_tim14_init\s*\(' -and
                  $main -match '\btimer_enable\s*\(\s*TIMER2\s*\)') 'PWM output and frequency capture timers are not initialized'
Assert-Contract ($main -match '\bmx_tim0_adc_init\s*\(' -and
                 $main -match '\bADC_StreamInit\s*\(' -and
                 $main -match '\btimer_enable\s*\(\s*TIMER0\s*\)') '40 kSa/s ADC timer/DMA stream is not started'
$uiRefresh = [regex]::Match($main,
    'if\s*\(\s*get_tft_timer_value\(\)\s*>=\s*ECG_UI_REFRESH_MS\s*\)\s*\{(?<body>.*?)\}',
    [System.Text.RegularExpressions.RegexOptions]::Singleline)
Assert-Contract ($uiRefresh.Success) 'UI refresh scheduler missing'
Assert-Contract ($main -match '(?m)^\s*#define\s+ECG_UI_REFRESH_MS\s+40U\s*$') 'Live waveform refresh must run at 25 FPS'
Assert-Contract ($uiRefresh.Groups['body'].Value -notmatch '\bPAUSE_MS_TIMER\b') 'UI rendering must not pause key or display clocks'
Assert-Contract ($uiRefresh.Groups['body'].Value -match '(?s)set_tft_timer_value\s*\(\s*0U\s*\).*?ECGAcq_ShowUI\s*\(') 'UI period must be measured from frame start'

$pageEnum = [regex]::Match($uiHeader,
    'typedef\s+enum\s*\{(?<body>.*?)\}\s*ecg_monitor_page_t\s*;',
    [System.Text.RegularExpressions.RegexOptions]::Singleline)
Assert-Contract $pageEnum.Success 'ecg_monitor_page_t enum missing from UI header'
Assert-Contract ($pageEnum.Groups['body'].Value -match '\bECG_MONITOR_PAGE\b' -and
                 $pageEnum.Groups['body'].Value -match '\bECG_WAVE_PAGE\b' -and
                 $pageEnum.Groups['body'].Value -match '\bECG_FREQ_PAGE\b') 'MONITOR, WAVE, and FREQ page values are required'
Assert-Contract ($uiHeader -match '\bECGMonitorUI_DrawStatic\s*\(\s*ecg_monitor_page_t\b') 'Static page renderer interface missing'
Assert-Contract ($uiHeader -match '\bECGMonitorUI_Render\s*\(' -and
                 $uiHeader -match '\bconst\s+ecg_monitor_view_t\s*\*' -and
                 $uiHeader -match '\bconst\s+int16_t\s*\*') 'Immutable ECG monitor render interface missing'
Assert-Contract ($taskHeader -match '\becg_monitor_page_t\s+ECGAcq_GetPage\s*\(\s*void\s*\)') 'ECGAcq_GetPage declaration missing'
Assert-Contract ($task -match '\becg_monitor_page_t\s+ECGAcq_GetPage\s*\(\s*void\s*\)') 'ECGAcq_GetPage implementation missing'

$initBody = Get-FunctionBody $task 'ECGAcq_Init'
Assert-Contract ($null -ne $initBody -and
                 $initBody -match '\bcurrent_page\s*=\s*ECG_MONITOR_PAGE\b') 'Startup page must be MONITOR'
$resetBody = Get-FunctionBody $task 'ecg_acq_reset_measurement'
Assert-Contract ($null -ne $resetBody) 'Measurement reset implementation missing'
Assert-Contract ($resetBody -notmatch '\b(?:current_page|gain|window_seconds)\s*=') 'Measurement reset must preserve page, gain, and time window'

$keyBody = Get-FunctionBody $task 'ECGAcq_KeyHandle'
Assert-Contract ($null -ne $keyBody) 'ECGAcq_KeyHandle implementation missing'
Assert-Contract ($keyBody -match '\bKEY2_Pin\b' -and
                 $keyBody -match '\bKeyPress\b' -and
                 $keyBody -match '\bKeyDoublePress\b') 'SW2 must handle both short and double press events'
Assert-Contract ($keyBody -match '\bgain\b' -and
                 $keyBody -match '\b(?:page|ECG_MONITOR_PAGE|ECG_WAVE_PAGE)\b') 'SW2 key handling must retain gain cycling and add page switching'
Assert-Contract ($keyBody -match '\bECGMonitorUI_DrawStatic\s*\(') 'Page changes must trigger a complete static redraw'
Assert-Contract ($keyBody -match '\bECGMonitorUI_ShowUI\s*\(' -or
                 $keyBody -match '\bECGAcq_ShowUI\s*\(') 'Page changes must immediately render current dynamic state'
Assert-Contract ($keyBody -match '(?s)key_pin\s*==\s*KEY3_Pin.*?key_state\s*==\s*KeyPress.*?key_state\s*==\s*KeyDoublePress') 'SW3 handling must explicitly accept only short and double press events'

$staticUiBody = Get-FunctionBody $task 'ECGAcq_StaticUI'
Assert-Contract ($null -ne $staticUiBody -and
                 $staticUiBody -match '\bECGMonitorUI_DrawStatic\s*\(' -and
                 $staticUiBody -match '\bECGAcq_ShowUI\s*\(') 'Initial static page must immediately render current dynamic state'
Assert-Contract ($ui -notmatch '(?:RR|RMSSD):----') 'Unavailable RR and RMSSD must use the specified three-dash placeholder'
Assert-Contract ($keyBody -match '\bKEYD_Pin\b' -and
                 $keyBody -match '\bset_pwm_state\s*\(') 'Encoder push must toggle PWM state'
Assert-Contract ($task -match '\bECG_PWM_PRESET_COUNT\b' -and
                 $task -match '1U\s*,\s*2U\s*,\s*5U\s*,\s*10U\s*,\s*20U') 'Low-frequency PWM self-test presets missing'
Assert-Contract ($ui -match '\btext_output\b' -and
                 $ui -match '\btext_input\b' -and
                 $ui -match 'NO SIG' -and
                 $ui -match 'PA2>PA6') 'FREQ page must distinguish output, measured input, and loopback wiring'
Assert-Contract ($ui -match 'MATCH' -and $ui -match 'MISMATCH') 'Loopback result must use explicit match terminology'
Assert-Contract ($ui -notmatch '"OK\s*"' -and $ui -notmatch '"CHK"') 'Loopback result must not imply independent calibration approval'
Assert-Contract ($ui -match '%5luHz' -and
                 $ui -match '\(unsigned long\)measured') 'Measured uint32 frequency must use a matching format'
Assert-Contract ($pwm -match 'period\s*-\s*1U' -and
                 $pwm -match 'duty\s*>\s*pwm_period') 'PWM ARR N-1 and duty clamp are required'
Assert-Contract ($pwmHeader -match 'PWM_TIMER_FREQ_HZ\s+10000U' -and
                 $hardwareTimer -match '(?s)mx_tim14_init.*?prescaler\s*=\s*7199') 'PWM timer must use the 10 kHz base required for 1 Hz output'
Assert-Contract ($pwm -notmatch '\b(?:float|double)\b') 'PWM module must use integer math only'
Assert-Contract ($timerHeader -match 'FREQ_SIGNAL_TIMEOUT_MS\s+2500U' -and
                 $timer -match '\btimer_measurement_is_fresh\s*\(') 'Frequency capture stale-data timeout missing'
Assert-Contract ($hardwareTimer -match '(?s)mx_tim14_init.*?GPIO_PIN_2.*?TIMER14' -and
                 $hardwareTimer -match '(?s)mx_tim2_init.*?GPIO_PIN_6.*?TIMER2') 'PA2 PWM or PA6 capture pin contract missing'
Assert-Contract ($hardwareTimer -match '(?s)mx_tim2_init.*?gpio_mode_set\s*\(\s*GPIOA\s*,\s*GPIO_MODE_AF\s*,\s*GPIO_PUPD_PULLDOWN\s*,\s*GPIO_PIN_6\s*\)') 'PA6 capture input must use a pull-down for a defined disconnected state'
Assert-Contract ($hardwareTimer -match '(?m)^\s*#define\s+FREQ_CAPTURE_FILTER\s+0x0[1-9A-Fa-f]U?\s*$' -and
                 $hardwareTimer -match '\bicfilter\s*=\s*FREQ_CAPTURE_FILTER\b') 'PA6 capture must use a nonzero digital input filter'
Assert-Contract ($hardwareTimer -match 'nvic_irq_enable\s*\(\s*TIMER2_IRQn\s*,\s*1U\s*\)' -and
                  $hardwareTimer -match 'nvic_irq_enable\s*\(\s*TIMER15_IRQn\s*,\s*0U\s*\)') 'ECG sample IRQ must outrank frequency capture IRQ'
Assert-Contract ($hardwareTimer -match 'ADC_SCOPE_TIMER_PERIOD\s+24U' -and
                 $hardwareTimer -match 'ADC_SCOPE_TIMER_COMPARE\s+12U' -and
                 $hardwareTimer -match '(?s)mx_tim0_adc_init.*?TIMER0.*?TIMER_CH_0' -and
                 $hardwareTimer -match 'timer_channel_output_pulse_value_config\s*\(\s*TIMER0\s*,\s*TIMER_CH_0\s*,\s*ADC_SCOPE_TIMER_COMPARE\s*\)') 'TIMER0 must provide a 40 kHz ADC trigger at an interior compare point'
Assert-Contract ($hardwareAdc -match 'ADC_CONTINUOUS_MODE\s*,\s*DISABLE' -and
                 $hardwareAdc -match 'ADC_EXTTRIG_REGULAR_T0_CH0' -and
                 $hardwareAdc -match '(?s)mx_adc_scope_dma_init.*?dma_circulation_enable' -and
                 $hardwareAdc -match 'DMA_CHXCTL_HTFIE\s*\|\s*DMA_CHXCTL_FTFIE') 'ADC fast path must use timer-triggered circular DMA with stable half frames'
Assert-Contract ($middleAdc -match '\bADC_StreamCopyLatestDisplay\s*\(' -and
                 $middleAdc -match '\bADC_SCOPE_HALF_SAMPLES\b' -and
                 $middleAdc -match 'static\s+volatile\s+uint16_t\s+adc_scope_buffer') 'ADC stream snapshot must treat the DMA buffer as asynchronous data'
Assert-Contract ($uiHeader -match '\bwave_frame_ready\b' -and
                 $uiHeader -match '\bwave_span\b' -and
                 $ui -match 'DMA WAIT' -and $ui -match 'FLAT') 'WAVE UI must distinguish a missing DMA frame from a flat frame'
Assert-Contract ($lcdHardware -match '\bSPI_FLAG_TRANS\b') 'LCD SPI transactions must wait for the final shifted bit'
Assert-Contract ($lcdHardwareHeader -match '\bTFT_WriteColorBurst\s*\(' -and
                 $lcdHardware -match '\bTFT_WriteColorBurst\s*\(') 'LCD color-burst interface missing'
Assert-Contract ($lcdHardwareHeader -match '\bTFT_WritePixels\s*\(' -and
                 $lcdHardware -match '\bTFT_WritePixels\s*\(' -and
                 $lcdMiddle -match '\bTFT_DrawPixelRow\s*\(' -and
                 $ui -match '\bTFT_DrawPixelRow\s*\(') 'LCD scanline burst path missing'
$fillBody = Get-FunctionBody $lcdMiddle 'TFT_Fill'
Assert-Contract ($null -ne $fillBody -and
                 $fillBody -match '\bTFT_WriteColorBurst\s*\(') 'TFT_Fill must use one continuous color burst'

$renderBody = Get-FunctionBody $ui 'ECGMonitorUI_Render'
Assert-Contract ($null -ne $renderBody) 'ECGMonitorUI_Render implementation missing'
$renderChecks = [regex]::Replace($renderBody, '\(\s*const\s+[A-Za-z0-9_]+\s*\*\s*\)', '')
Assert-Contract ($renderChecks -match '(?:!\s*view|view\s*==\s*(?:NULL|0))' -and
                 $renderChecks -match '(?:!\s*plot_y|plot_y\s*==\s*(?:NULL|0))') 'Renderer must reject null view and plot pointers'
Assert-Contract ($renderBody -match '(?:plot_count\s*<\s*2U?|plot_count\s*<=\s*1U?)') 'Renderer must reject plot counts below two'
Assert-Contract ($ui -notmatch '\b(?:malloc|calloc|realloc|free)\s*\(') 'UI module must not use dynamic allocation'
Assert-Contract ($ui -notmatch '\b(?:float|double)\b') 'UI module must not require floating point'
Assert-Contract ($ui -match '\bTFT_ShowChinese\s*\(') 'Demo UI must present concise Chinese descriptions'
Assert-Contract ($ui -match '%ums' -and
                 $ui -match 'fit_limited') 'WAVE page must show millisecond timebase and automatic fit state'
Assert-Contract ($ui -match '0xCA\s*,\s*0xE4\s*,\s*0xC8\s*,\s*0xEB' -and
                 $ui -match '0xC6\s*,\s*0xB5\s*,\s*0xC2\s*,\s*0xCA') 'Chinese UI labels must use explicit GBK byte arrays'
Assert-Contract ($ui -notmatch '"[^"\r\n]*[\u4e00-\u9fff][^"\r\n]*"') 'Do not pass UTF-8 Chinese literals to the GBK-indexed TFT font'
foreach ($gridMacro in @('ECG_GRID_MINOR_X', 'ECG_GRID_MINOR_Y',
                          'ECG_GRID_MAJOR_X', 'ECG_GRID_MAJOR_Y')) {
    Assert-Contract ($ui -match "(?m)^\s*#define\s+$gridMacro\s+\d+U?\s*$") `
        "Practical ECG grid macro missing: $gridMacro"
}
Assert-Contract ($ui -match '(?s)ECG_GRID_MAJOR_X.*?TFT_(?:DrawLine|Fill)' -and
                 $ui -match '(?s)ECG_GRID_MAJOR_Y.*?TFT_(?:DrawLine|Fill)') 'Major ECG grid divisions must use continuous lines'
Assert-LiteralCoordinatesInBounds $ui

$numericDefines = @{}
foreach ($match in [regex]::Matches(($uiHeader + "`n" + $ui), '(?m)^\s*#define\s+(?<name>[A-Z0-9_]+)\s+(?<value>\d+)U?\s*$')) {
    $numericDefines[$match.Groups['name'].Value] = [int]$match.Groups['value'].Value
}
foreach ($name in $numericDefines.Keys) {
    if ($name -match '(?:_X0|_X1)$') {
        Assert-Contract ($numericDefines[$name] -le 159) "$name exceeds the 160-pixel display width"
    }
    elseif ($name -match '(?:_Y0|_Y1|_CENTER_Y)$') {
        Assert-Contract ($numericDefines[$name] -le 127) "$name exceeds the 128-pixel display height"
    }
}
$waveTops = @($numericDefines.Keys | Where-Object { $_ -match 'WAVE.*(?:Y0|TOP)$' })
$waveBottoms = @($numericDefines.Keys | Where-Object { $_ -match 'WAVE.*(?:Y1|BOTTOM)$' })
$hasLargeWavePlot = $false
foreach ($topName in $waveTops) {
    foreach ($bottomName in $waveBottoms) {
        if (($numericDefines[$bottomName] - $numericDefines[$topName] + 1) -ge 90) { $hasLargeWavePlot = $true }
    }
}
Assert-Contract $hasLargeWavePlot 'Named WAVE page plot bounds must prove at least 90 vertical pixels'

$projectText = Get-Content -Raw $projectPath
Assert-Contract ($projectText -match 'GigaDevice\.GD32E23x_DFP\.1\.1\.0') 'Wrong device pack'
Assert-Contract (Test-Path (Join-Path $root 'Spec\Demo14_HESS_ECG_ANALYZER_SPEC.md')) 'Demo14 HESS ECG base spec missing'
Assert-Contract (-not (Get-ChildItem (Join-Path $root 'APP'),
                                    (Join-Path $root 'Middle'),
                                    (Join-Path $root 'User') -Recurse |
    Where-Object { $_.Name -match '[^\x00-\x7F]' })) 'Non-ASCII code name'

Write-Host 'Demo14 hospital monitor firmware contract passed.'
