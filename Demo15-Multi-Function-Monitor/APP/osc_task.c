#include "osc_task.h"

#include <stdio.h>

#include "hw_key.h"
#include "hess_analyzer.h"
#include "mid_adc.h"
#include "mid_lcd.h"
#include "mid_pwm.h"
#include "mid_timer.h"
#include "scope_metrics.h"
#include "scope_view.h"
#include "signal_output.h"
#include "spo2_receiver.h"
#include "vital_trace.h"

#define ADC_SAMPLE_RATE_HZ       20000U
#define ECG_SAMPLE_RATE_HZ       250U
#define ECG_DECIMATION           (ADC_SAMPLE_RATE_HZ / ECG_SAMPLE_RATE_HZ)
#define ADC_DMA_SAMPLES          (ECG_DECIMATION * 2U)
#define SCOPE_RANGE_COUNT        3U
#define SCOPE_IDLE_SPAN_COUNTS   64U
#define SCOPE_DUTY_MIN_SPAN_COUNTS 16U
#define SCOPE_STARTUP_SETTLE_SAMPLES (ADC_SAMPLE_RATE_HZ / 2U)
#define FAST_HISTORY_SAMPLES     800U
#define SCOPE_NOISE_WAVE_WIDTH   156U
#define FAST_DISPLAY_SAMPLES     SCOPE_NOISE_WAVE_WIDTH
#define SLOW_HISTORY_SAMPLES     1250U
#define SCOPE_FAST_TIMEBASE_COUNT 5U
#define ECG_HISTORY_SAMPLES      1250U
#define ECG_HEART_PULSE_SAMPLES  45U
#define ECG_SIGNAL_TIMEOUT       750U

#define WAVE_X       2U
#define WAVE_Y       18U
#define WAVE_WIDTH   120U
#define WAVE_HEIGHT  76U
#define WAVE_RIGHT   (WAVE_X + WAVE_WIDTH - 1U)
#define WAVE_BOTTOM  (WAVE_Y + WAVE_HEIGHT - 1U)

static const uint16_t timebase_samples[ECG_TIMEBASE_COUNT] = {
    250U, 500U, 1000U, 1250U
};
static const char timebase_labels[ECG_TIMEBASE_COUNT][5] = {
    "1.0s", "2.0s", "4.0s", "5.0s"
};
static const uint16_t scope_timebase_samples[SCOPE_TIMEBASE_COUNT] = {
    40U, 80U, 200U, 400U, 800U, 250U, 500U, 1000U, 1250U
};
static const char scope_timebase_labels[SCOPE_TIMEBASE_COUNT][5] = {
    " 2ms", " 4ms", "10ms", "20ms", "40ms", "1.0s", "2.0s", "4.0s", "5.0s"
};
static const uint16_t scope_display_vpp_mv[SCOPE_RANGE_COUNT] = {
    5000U, 1000U, 200U
};
static const char scope_range_labels[SCOPE_RANGE_COUNT][5] = {
    "5Vpp", "1Vpp", "200m"
};

static uint16_t adc_dma_samples[ADC_DMA_SAMPLES];
static volatile uint16_t fast_history[FAST_HISTORY_SAMPLES];
static uint16_t fast_display_samples[FAST_DISPLAY_SAMPLES];
static volatile uint16_t fast_write_index;
static volatile uint16_t fast_history_count;
static volatile uint32_t fast_sample_count;
static volatile uint8_t scope_startup_ready;
static volatile uint8_t slow_history[SLOW_HISTORY_SAMPLES];
static volatile uint16_t slow_write_index;
static volatile uint16_t slow_history_count;
static uint32_t ecg_decimation_sum;
static uint16_t ecg_decimation_count;

static volatile int8_t ecg_history[ECG_HISTORY_SAMPLES];
static volatile uint16_t ecg_write_index;
static volatile uint16_t ecg_history_count;
static volatile uint32_t ecg_sample_count;
static volatile uint16_t ecg_bpm;
static volatile uint16_t ecg_rr_ms;
static volatile uint32_t ecg_last_peak_sample;
static volatile uint8_t ecg_peak_seen;

static uint16_t ecg_vref_value = 1500U;
static uint8_t ecg_timebase_index = 2U;
static uint8_t scope_timebase_index = 2U;
static uint8_t scope_range_index;
static scope_view_mode_t scope_view_mode = SCOPE_VIEW_FREE;
static uint8_t scope_ecg_view;
static uint16_t scope_roll_phase;
static uint8_t scope_roll_toward_older = 1U;
static volatile demo15_mode_t app_mode = DEMO15_MODE_OSCILLOSCOPE;
static uint8_t osc_stop_bit = OSC_RUN;
static uint16_t ecg_vpp_mv;
static uint16_t scope_vpp_mv;
static uint32_t scope_sample_frequency;
static scope_metrics_t scope_metrics;

typedef struct
{
    uint32_t heart;
    uint32_t run_state;
    uint32_t timebase;
    uint32_t primary;
    uint32_t bpm;
    uint32_t vpp;
    uint32_t signal;
    uint32_t source;
} vital_ui_cache_t;

static vital_trace_state_t vital_trace_state;
static vital_ui_cache_t vital_ui_cache;
static uint32_t vital_trace_next_sample;
static uint16_t vital_trace_scale_abs = 32U;
static uint16_t vital_trace_peak_abs;
static uint16_t vital_trace_previous_x;
static uint16_t vital_trace_previous_y;
static uint8_t vital_trace_have_previous;
static uint8_t vital_trace_reset_pending = 1U;

static int32_t baseline_q8;
static int16_t filtered_value;
static spo2_receiver_t spo2_receiver;
static uint8_t ecg_generator_active;
static uint8_t saved_pwm_state;
static uint16_t saved_pwm_period;
static uint16_t saved_pwm_duty;

static void vital_show_ui(uint8_t spo2_sim_mode);

static uint8_t scope_noise_zoom_active(void)
{
    return (uint8_t)(scope_range_index == (SCOPE_RANGE_COUNT - 1U));
}

static uint16_t scope_active_wave_width(void)
{
    return (scope_noise_zoom_active() != 0U) ?
           SCOPE_NOISE_WAVE_WIDTH : WAVE_WIDTH;
}

static uint16_t scope_active_wave_right(void)
{
    return (uint16_t)(WAVE_X + scope_active_wave_width() - 1U);
}

static void vital_ui_cache_invalidate(void)
{
    vital_ui_cache.heart = UINT32_MAX;
    vital_ui_cache.run_state = UINT32_MAX;
    vital_ui_cache.timebase = UINT32_MAX;
    vital_ui_cache.primary = UINT32_MAX;
    vital_ui_cache.bpm = UINT32_MAX;
    vital_ui_cache.vpp = UINT32_MAX;
    vital_ui_cache.signal = UINT32_MAX;
    vital_ui_cache.source = UINT32_MAX;
}

static void ecg_generator_start(void)
{
    uint32_t primask;

    if(ecg_generator_active != 0U){
        return;
    }
    saved_pwm_state = get_pwm_state();
    saved_pwm_period = get_pwm_period();
    saved_pwm_duty = get_pwm_duty();
    primask = __get_PRIMASK();
    __disable_irq();
    SignalOutput_SelectEcg();
    SignalOutput_SetEnabled(1U);
    ecg_generator_active = 1U;
    if(primask == 0U){
        __enable_irq();
    }
}

static void ecg_generator_stop(void)
{
    uint32_t primask;

    if(ecg_generator_active == 0U){
        return;
    }
    primask = __get_PRIMASK();
    __disable_irq();
    SignalOutput_SetEnabled(0U);
    set_pwm_period(saved_pwm_period);
    set_pwm_duty(saved_pwm_duty);
    set_pwm_state(saved_pwm_state);
    ecg_generator_active = 0U;
    if(primask == 0U){
        __enable_irq();
    }
}

static uint16_t ecg_abs16(int16_t value)
{
    if(value < 0){
        return (uint16_t)(-value);
    }
    return (uint16_t)value;
}

void ECG_Init(uint16_t vref_value)
{
    uint16_t i;

    if(vref_value != 0U){
        ecg_vref_value = vref_value;
    }
    for(i = 0U; i < ECG_HISTORY_SAMPLES; i++){
        ecg_history[i] = 0;
    }
    for(i = 0U; i < FAST_HISTORY_SAMPLES; i++){
        fast_history[i] = 0U;
    }
    for(i = 0U; i < SLOW_HISTORY_SAMPLES; i++){
        slow_history[i] = 0U;
    }
    fast_write_index = 0U;
    fast_history_count = 0U;
    fast_sample_count = 0U;
    scope_startup_ready = 0U;
    slow_write_index = 0U;
    slow_history_count = 0U;
    ecg_decimation_sum = 0U;
    ecg_decimation_count = 0U;
    ecg_write_index = 0U;
    ecg_history_count = 0U;
    ecg_sample_count = 0U;
    ecg_bpm = 0U;
    ecg_rr_ms = 0U;
    ecg_last_peak_sample = 0U;
    ecg_peak_seen = 0U;
    baseline_q8 = 0;
    filtered_value = 0;
    vital_trace_reset_pending = 1U;
    vital_ui_cache_invalidate();
    SpO2Receiver_Init(&spo2_receiver);
    HESSAnalyzer_Init();
}

static void vital_analysis_reset(void)
{
    uint16_t i;

    for(i = 0U; i < ECG_HISTORY_SAMPLES; i++){
        ecg_history[i] = 0;
    }
    ecg_write_index = 0U;
    ecg_history_count = 0U;
    ecg_sample_count = 0U;
    ecg_bpm = 0U;
    ecg_rr_ms = 0U;
    ecg_last_peak_sample = 0U;
    ecg_peak_seen = 0U;
    baseline_q8 = 0;
    filtered_value = 0;
    vital_trace_reset_pending = 1U;
    HESSAnalyzer_Init();
}

static void ecg_process_sample(uint16_t raw_value)
{
    int16_t high_pass;
    int32_t packed_sample;
    if(ecg_sample_count == 0U){
        baseline_q8 = ((int32_t)raw_value << 8);
    }

    baseline_q8 += ((((int32_t)raw_value << 8) - baseline_q8) >> 7);
    /* The analog front end is inverted: Vadc = 2.5 V - Vin / 2. */
    high_pass = (int16_t)((baseline_q8 >> 8) - (int32_t)raw_value);
    filtered_value += (int16_t)((high_pass - filtered_value) >> 2);

    packed_sample = filtered_value / 8;
    if(packed_sample > 127){
        packed_sample = 127;
    }else if(packed_sample < -128){
        packed_sample = -128;
    }
    ecg_history[ecg_write_index] = (int8_t)packed_sample;
    ecg_write_index++;
    if(ecg_write_index >= ECG_HISTORY_SAMPLES){
        ecg_write_index = 0U;
    }
    if(ecg_history_count < ECG_HISTORY_SAMPLES){
        ecg_history_count++;
    }
    ecg_sample_count++;

    HESSAnalyzer_ProcessSample(raw_value);
    ecg_bpm = HESSAnalyzer_GetBpm();
    ecg_rr_ms = HESSAnalyzer_GetRrMs();
    if(HESSAnalyzer_HasNewPeak() != 0U){
        ecg_peak_seen = 1U;
        ecg_last_peak_sample = ecg_sample_count;
    }
}

static void ecg_adc_dma_callback(const uint16_t *samples, uint16_t count)
{
    uint16_t i;
    uint16_t raw_value;
    uint16_t spo2_input_sample;

    for(i = 0U; i < count; i++){
        raw_value = samples[i];
        fast_history[fast_write_index] = raw_value;
        fast_write_index++;
        if(fast_write_index >= FAST_HISTORY_SAMPLES){
            fast_write_index = 0U;
        }
        if(fast_history_count < FAST_HISTORY_SAMPLES){
            fast_history_count++;
        }
        fast_sample_count++;
        if((scope_startup_ready == 0U) &&
           (fast_sample_count >= SCOPE_STARTUP_SETTLE_SAMPLES)){
            scope_startup_ready = 1U;
        }

        ecg_decimation_sum += raw_value;
        ecg_decimation_count++;
        if(ecg_decimation_count >= ECG_DECIMATION){
            raw_value = (uint16_t)(ecg_decimation_sum / ECG_DECIMATION);
            slow_history[slow_write_index] = (uint8_t)(raw_value >> 4U);
            slow_write_index++;
            if(slow_write_index >= SLOW_HISTORY_SAMPLES){
                slow_write_index = 0U;
            }
            if(slow_history_count < SLOW_HISTORY_SAMPLES){
                slow_history_count++;
            }
            if(app_mode == DEMO15_MODE_SPO2_MONITOR){
                spo2_input_sample = SpO2Core_ReconstructInputSample(
                    raw_value, ecg_vref_value);
                /* Raw PA3 codes drive the CH1 duty-coded fallback. The
                 * reconstructed sample and PA6 tag retain the optional
                 * paired RED/IR teaching path. */
                (void)SpO2Receiver_ProcessSample(
                    &spo2_receiver, raw_value, spo2_input_sample,
                    (uint8_t)(gpio_input_bit_get(GPIOA, GPIO_PIN_6) != RESET));
                ecg_process_sample(raw_value);
            }else{
                ecg_process_sample(raw_value);
            }
            ecg_decimation_sum = 0U;
            ecg_decimation_count = 0U;
        }
    }
}

void ECG_AcquisitionStart(void)
{
    mid_adc_start_dma(adc_dma_samples, ADC_DMA_SAMPLES, ecg_adc_dma_callback);
}

void set_ecg_timebase_index(uint8_t value)
{
    if(value >= ECG_TIMEBASE_COUNT){
        value = ECG_TIMEBASE_COUNT - 1U;
    }
    if(ecg_timebase_index != value){
        ecg_timebase_index = value;
        vital_trace_reset_pending = 1U;
    }
}

uint8_t get_ecg_timebase_index(void)
{
    return ecg_timebase_index;
}

void set_scope_timebase_index(uint8_t value)
{
    if(value >= SCOPE_TIMEBASE_COUNT){
        value = SCOPE_TIMEBASE_COUNT - 1U;
    }
    if(scope_timebase_index != value){
        scope_timebase_index = value;
        scope_roll_phase = 0U;
        scope_roll_toward_older = 1U;
    }
}

uint8_t get_scope_timebase_index(void)
{
    return scope_timebase_index;
}

void toggle_scope_small_signal(void)
{
    scope_range_index++;
    if(scope_range_index >= SCOPE_RANGE_COUNT){
        scope_range_index = 0U;
    }
    scope_roll_phase = 0U;
    scope_roll_toward_older = 1U;
    TFT_StaticUI();
}

uint8_t get_scope_small_signal(void)
{
    return (uint8_t)(scope_range_index != 0U);
}

void cycle_scope_view_mode(void)
{
    scope_view_mode = (scope_view_mode == SCOPE_VIEW_FALLING) ?
                      SCOPE_VIEW_FREE :
                      (scope_view_mode_t)(scope_view_mode + 1);
    scope_roll_phase = 0U;
    scope_roll_toward_older = 1U;
}

scope_view_mode_t get_scope_view_mode(void)
{
    return scope_view_mode;
}

void toggle_scope_ecg_view(void)
{
    scope_ecg_view = (scope_ecg_view == 0U) ? 1U : 0U;
    osc_stop_bit = OSC_RUN;
    vital_analysis_reset();
    if(scope_ecg_view != 0U){
        ecg_generator_start();
    }else{
        ecg_generator_stop();
    }
    TFT_StaticUI();
}

uint8_t Demo15_IsScopeEcgView(void)
{
    return scope_ecg_view;
}

void set_osc_stop_bit(uint8_t value)
{
    if((osc_stop_bit == OSC_PAUSE) && (value == OSC_RUN)){
        vital_trace_reset_pending = 1U;
    }
    osc_stop_bit = value;
}

uint8_t get_osc_stop_bit(void)
{
    return osc_stop_bit;
}

static void draw_heart(uint8_t active)
{
    uint16_t color = (active != 0U) ? RED : MAGENTA;
    uint16_t x = (active != 0U) ? 28U : 30U;
    uint16_t y = (active != 0U) ? 2U : 4U;

    TFT_Fill(27U, 1U, 43U, 15U, BLACK);
    if(active != 0U){
        TFT_DrawLine(x + 2U, y, x + 4U, y, color);
        TFT_DrawLine(x + 8U, y, x + 10U, y, color);
        TFT_DrawLine(x + 1U, y + 1U, x + 11U, y + 1U, color);
        TFT_DrawLine(x, y + 2U, x + 12U, y + 2U, color);
        TFT_DrawLine(x, y + 3U, x + 12U, y + 3U, color);
        TFT_DrawLine(x + 1U, y + 4U, x + 11U, y + 4U, color);
        TFT_DrawLine(x + 2U, y + 5U, x + 10U, y + 5U, color);
        TFT_DrawLine(x + 3U, y + 6U, x + 9U, y + 6U, color);
        TFT_DrawLine(x + 4U, y + 7U, x + 8U, y + 7U, color);
        TFT_DrawLine(x + 5U, y + 8U, x + 7U, y + 8U, color);
        TFT_DrawPoint(x + 6U, y + 9U, color);
    }else{
        TFT_DrawLine(x + 1U, y, x + 3U, y, color);
        TFT_DrawLine(x + 6U, y, x + 8U, y, color);
        TFT_DrawLine(x, y + 1U, x + 9U, y + 1U, color);
        TFT_DrawLine(x, y + 2U, x + 9U, y + 2U, color);
        TFT_DrawLine(x + 1U, y + 3U, x + 8U, y + 3U, color);
        TFT_DrawLine(x + 2U, y + 4U, x + 7U, y + 4U, color);
        TFT_DrawLine(x + 3U, y + 5U, x + 6U, y + 5U, color);
        TFT_DrawLine(x + 4U, y + 6U, x + 5U, y + 6U, color);
    }
}

static void draw_wave_grid(uint16_t wave_right)
{
    uint16_t x;
    uint16_t y;

    TFT_Fill(WAVE_X, WAVE_Y, wave_right + 1U, WAVE_BOTTOM + 1U, BLACK);
    TFT_DrawLine(WAVE_X - 1U, WAVE_Y - 1U, wave_right + 1U, WAVE_Y - 1U, CYAN);
    TFT_DrawLine(WAVE_X - 1U, WAVE_BOTTOM + 1U, wave_right + 1U, WAVE_BOTTOM + 1U, CYAN);
    TFT_DrawLine(WAVE_X - 1U, WAVE_Y - 1U, WAVE_X - 1U, WAVE_BOTTOM + 1U, CYAN);
    TFT_DrawLine(wave_right + 1U, WAVE_Y - 1U, wave_right + 1U, WAVE_BOTTOM + 1U, CYAN);

    for(x = WAVE_X + 17U; x < wave_right; x += 18U){
        for(y = WAVE_Y + 2U; y < WAVE_BOTTOM; y += 4U){
            TFT_DrawPoint(x, y, GRAYBLUE);
        }
    }
    for(y = WAVE_Y + 18U; y < WAVE_BOTTOM; y += 19U){
        for(x = WAVE_X + 2U; x < wave_right; x += 4U){
            TFT_DrawPoint(x, y, GRAYBLUE);
        }
    }
}

static void restore_wave_grid_column(uint16_t x)
{
    uint16_t y;

    if((x >= (WAVE_X + 17U)) &&
       (((x - WAVE_X - 17U) % 18U) == 0U)){
        for(y = WAVE_Y + 2U; y < WAVE_BOTTOM; y += 4U){
            TFT_DrawPoint(x, y, GRAYBLUE);
        }
    }
    if((x >= (WAVE_X + 2U)) &&
       (((x - WAVE_X - 2U) % 4U) == 0U)){
        for(y = WAVE_Y + 18U; y < WAVE_BOTTOM; y += 19U){
            TFT_DrawPoint(x, y, GRAYBLUE);
        }
    }
}

static void clear_wave_column(uint16_t x)
{
    TFT_Fill(x, WAVE_Y, x + 1U, WAVE_BOTTOM + 1U, BLACK);
    restore_wave_grid_column(x);
}

static uint16_t vital_trace_map_y(int8_t sample)
{
    int32_t y_position = (int32_t)(WAVE_Y + (WAVE_HEIGHT / 2U)) -
                         ((int32_t)sample *
                          (int32_t)((WAVE_HEIGHT / 2U) - 3U) /
                          (int32_t)vital_trace_scale_abs);

    if(y_position < (int32_t)WAVE_Y){
        y_position = WAVE_Y;
    }else if(y_position > (int32_t)WAVE_BOTTOM){
        y_position = WAVE_BOTTOM;
    }
    return (uint16_t)y_position;
}

static void vital_trace_restart(uint8_t redraw_grid)
{
    if(redraw_grid != 0U){
        draw_wave_grid(WAVE_RIGHT);
    }
    VitalTrace_Init(&vital_trace_state, WAVE_WIDTH,
                    timebase_samples[ecg_timebase_index]);
    vital_trace_next_sample = ecg_sample_count;
    vital_trace_scale_abs = 32U;
    vital_trace_peak_abs = 0U;
    vital_trace_have_previous = 0U;
    vital_trace_previous_x = WAVE_X;
    vital_trace_previous_y = WAVE_Y + (WAVE_HEIGHT / 2U);
    ecg_vpp_mv = 0U;
    vital_trace_reset_pending = 0U;
}

static void vital_static_ui(uint8_t spo2_sim_mode)
{
    const char *title = (spo2_sim_mode == 2U) ? "S-ECG" :
                        ((spo2_sim_mode != 0U) ? "SpO2" : "ECG ");

    vital_ui_cache_invalidate();
    TFT_Fill(0U, 0U, 160U, 128U, BLACK);
    TFT_ShowString(2U, 0U, (const uint8_t *)title, WHITE, BLACK, 16U, 0U);
    TFT_ShowString(80U, 0U, (const uint8_t *)"TB", WHITE, BLACK, 16U, 0U);

    TFT_DrawLine(0U, 16U, 159U, 16U, CYAN);
    TFT_DrawLine(124U, 16U, 124U, 95U, CYAN);
    TFT_DrawLine(0U, 96U, 159U, 96U, CYAN);

    TFT_ShowString(128U, 18U,
                   (const uint8_t *)((spo2_sim_mode == 1U) ? "O2%" : "OUT"),
                   WHITE, BLACK, 16U, 0U);
    TFT_ShowString(128U, 50U, (const uint8_t *)"BPM", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(128U, 80U, (const uint8_t *)"/min", WHITE, BLACK, 16U, 0U);

    TFT_ShowString(2U, 96U, (const uint8_t *)"Vpp", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(44U, 96U, (const uint8_t *)"SIG", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(76U, 96U, (const uint8_t *)"SPAN", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(128U, 96U,
                   (const uint8_t *)((spo2_sim_mode == 1U) ? "SIM" : "PWM"),
                   WHITE, BLACK, 16U, 0U);

    draw_wave_grid(WAVE_RIGHT);
    vital_trace_restart(0U);
    vital_show_ui(spo2_sim_mode);
}

static void vital_show_ui(uint8_t spo2_sim_mode)
{
    char show_data[12];
    uint32_t primask;
    uint32_t cache_key;
    spo2_core_result_t spo2_result = {0U, 0U, 0U, 0U};
    spo2_source_t spo2_source = SPO2_SOURCE_WAITING;
    uint32_t peak_age = ecg_sample_count - ecg_last_peak_sample;
    uint8_t heart_active = (uint8_t)((ecg_peak_seen != 0U) &&
                                     (peak_age < ECG_HEART_PULSE_SAMPLES));
    uint8_t signal_ok = (uint8_t)((ecg_bpm != 0U) &&
                                  (peak_age < ECG_SIGNAL_TIMEOUT));

    cache_key = ((uint32_t)spo2_sim_mode << 8U) | heart_active;
    if((spo2_sim_mode == 0U) && (vital_ui_cache.heart != cache_key)){
        draw_heart(heart_active);
        vital_ui_cache.heart = cache_key;
    }

    cache_key = osc_stop_bit;
    if(vital_ui_cache.run_state != cache_key){
        if(osc_stop_bit == OSC_RUN){
            TFT_ShowString(44U, 0U, (const uint8_t *)"RUN ",
                           BLACK, GREEN, 16U, 0U);
        }else{
            TFT_ShowString(44U, 0U, (const uint8_t *)"HOLD",
                           BLACK, YELLOW, 16U, 0U);
        }
        vital_ui_cache.run_state = cache_key;
    }

    cache_key = ecg_timebase_index;
    if(vital_ui_cache.timebase != cache_key){
        TFT_ShowString(100U, 0U,
                       (const uint8_t *)timebase_labels[ecg_timebase_index],
                       YELLOW, BLACK, 16U, 0U);
        TFT_ShowString(76U, 112U,
                       (const uint8_t *)timebase_labels[ecg_timebase_index],
                       CYAN, BLACK, 16U, 0U);
        vital_ui_cache.timebase = cache_key;
    }

    if(spo2_sim_mode == 1U){
        primask = __get_PRIMASK();
        __disable_irq();
        spo2_result = SpO2Receiver_GetResult(&spo2_receiver);
        spo2_source = SpO2Receiver_GetSource(&spo2_receiver);
        if(primask == 0U){
            __enable_irq();
        }
        cache_key = ((uint32_t)spo2_sim_mode << 24U) |
                    ((uint32_t)spo2_result.valid << 16U) |
                    spo2_result.percent;
        if(vital_ui_cache.primary != cache_key){
            TFT_Fill(128U, 34U, 160U, 50U, BLACK);
            if(spo2_result.valid != 0U){
                sprintf(show_data, "%3u%%", (unsigned int)spo2_result.percent);
                TFT_ShowString(128U, 34U, (const uint8_t *)show_data,
                               GREEN, BLACK, 16U, 0U);
            }else{
                TFT_ShowString(128U, 34U, (const uint8_t *)"--- ",
                               YELLOW, BLACK, 16U, 0U);
            }
            vital_ui_cache.primary = cache_key;
        }
    }else{
        cache_key = ((uint32_t)spo2_sim_mode << 24U) | get_pwm_state();
        if(vital_ui_cache.primary != cache_key){
            if(get_pwm_state() == PWM_ON){
                TFT_ShowString(128U, 34U, (const uint8_t *)"ON ",
                               GREEN, BLACK, 16U, 0U);
            }else{
                TFT_ShowString(128U, 34U, (const uint8_t *)"OFF",
                               RED, BLACK, 16U, 0U);
            }
            vital_ui_cache.primary = cache_key;
        }
    }

    cache_key = ecg_bpm;
    if(vital_ui_cache.bpm != cache_key){
        if(ecg_bpm != 0U){
            sprintf(show_data, "%3u", (unsigned int)ecg_bpm);
        }else{
            sprintf(show_data, "---");
        }
        TFT_Fill(128U, 64U, 160U, 80U, BLACK);
        TFT_ShowString(128U, 64U, (const uint8_t *)show_data,
                       YELLOW, BLACK, 16U, 0U);
        vital_ui_cache.bpm = cache_key;
    }

    cache_key = ecg_vpp_mv;
    if(vital_ui_cache.vpp != cache_key){
        if(ecg_vpp_mv < 1000U){
            sprintf(show_data, "%3umV", (unsigned int)ecg_vpp_mv);
        }else{
            sprintf(show_data, "%u.%02uV",
                    (unsigned int)(ecg_vpp_mv / 1000U),
                    (unsigned int)((ecg_vpp_mv % 1000U) / 10U));
        }
        TFT_Fill(2U, 112U, 44U, 128U, BLACK);
        TFT_ShowString(2U, 112U, (const uint8_t *)show_data,
                       GREEN, BLACK, 16U, 0U);
        vital_ui_cache.vpp = cache_key;
    }

    cache_key = (uint32_t)((((spo2_sim_mode == 1U) &&
                             (spo2_result.valid != 0U)) ||
                            ((spo2_sim_mode != 1U) &&
                             (signal_ok != 0U))) ? 1U : 0U);
    if(vital_ui_cache.signal != cache_key){
        if(cache_key != 0U){
            TFT_ShowString(44U, 112U, (const uint8_t *)"OK ",
                           GREEN, BLACK, 16U, 0U);
        }else{
            TFT_ShowString(44U, 112U, (const uint8_t *)"WAIT",
                           YELLOW, BLACK, 16U, 0U);
        }
        vital_ui_cache.signal = cache_key;
    }

    if(spo2_sim_mode == 1U){
        const char *source_text = (spo2_source == SPO2_SOURCE_DUAL_CHANNEL) ?
                                  "2CH" :
                                  ((spo2_source == SPO2_SOURCE_DUTY_CODED) ?
                                   "DUT" : "WAIT");
        cache_key = ((uint32_t)spo2_sim_mode << 24U) | spo2_source;
        if(vital_ui_cache.source != cache_key){
            TFT_Fill(128U, 112U, 160U, 128U, BLACK);
            TFT_ShowString(128U, 112U, (const uint8_t *)source_text,
                           CYAN, BLACK, 16U, 0U);
            vital_ui_cache.source = cache_key;
        }
    }else{
        cache_key = ((uint32_t)spo2_sim_mode << 24U) |
                    ((uint32_t)get_pwm_state() << 16U) |
                    SignalOutput_GetValue();
        if(vital_ui_cache.source != cache_key){
            sprintf(show_data, "T%u", (unsigned int)SignalOutput_GetValue());
            TFT_Fill(128U, 112U, 160U, 128U, BLACK);
            TFT_ShowString(128U, 112U, (const uint8_t *)show_data,
                           (get_pwm_state() == PWM_ON) ? GREEN : RED,
                           BLACK, 16U, 0U);
            vital_ui_cache.source = cache_key;
        }
    }
}

static uint32_t measure_sample_frequency(const uint16_t *samples, uint16_t count,
                                         uint16_t min_value, uint16_t max_value,
                                         uint32_t sample_rate_hz)
{
    uint16_t i;
    uint16_t center;
    uint16_t hysteresis;
    uint16_t low_level;
    uint16_t high_level;
    uint16_t first_crossing = 0U;
    uint16_t last_crossing = 0U;
    uint16_t crossing_count = 0U;
    uint8_t armed = 0U;
    uint32_t sample_delta;

    if((count < 3U) || ((uint16_t)(max_value - min_value) < 24U)){
        return 0U;
    }

    center = (uint16_t)(((uint32_t)min_value + max_value) / 2U);
    hysteresis = (uint16_t)((max_value - min_value) / 8U);
    if(hysteresis < 4U){
        hysteresis = 4U;
    }
    low_level = (center > hysteresis) ? (uint16_t)(center - hysteresis) : 0U;
    high_level = (uint16_t)(center + hysteresis);

    for(i = 0U; i < count; i++){
        if(samples[i] <= low_level){
            armed = 1U;
        }else if((armed != 0U) && (samples[i] >= high_level)){
            if(crossing_count == 0U){
                first_crossing = i;
            }
            last_crossing = i;
            crossing_count++;
            armed = 0U;
        }
    }

    if((crossing_count < 2U) || (last_crossing <= first_crossing)){
        return 0U;
    }
    sample_delta = last_crossing - first_crossing;
    return (((uint32_t)(crossing_count - 1U) * sample_rate_hz) +
            (sample_delta / 2U)) / sample_delta;
}

static void draw_scope_wave(uint16_t span_samples, uint8_t slow_timebase)
{
    uint16_t history_count;
    uint16_t write_index;
    uint16_t source_available;
    uint16_t available;
    uint16_t min_value;
    uint16_t max_value;
    uint16_t half_range;
    uint16_t midpoint;
    uint16_t previous_x = 0U;
    uint16_t previous_y = 0U;
    uint16_t point;
    uint16_t sample;
    uint16_t max_lag;
    uint16_t motion_step;
    uint16_t wave_width = scope_active_wave_width();
    uint16_t wave_right = scope_active_wave_right();
    uint16_t x;
    uint16_t y;
    uint8_t have_previous = 0U;
    uint32_t primask;
    uint32_t display_vpp_mv;
    uint32_t acquisition_sample_rate_hz;
    uint32_t display_sample_rate_hz;
    int32_t y_position;
    scope_view_info_t view_info = {0U};

    if(scope_startup_ready == 0U){
        scope_vpp_mv = 0U;
        scope_sample_frequency = 0U;
        scope_metrics.valid = 0U;
        scope_metrics.input_duty_percent = 0U;
        draw_wave_grid(wave_right);
        return;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    if(slow_timebase != 0U){
        history_count = slow_history_count;
        write_index = slow_write_index;
        available = ScopeView_CopyPacked12Window((const uint8_t *)slow_history,
                                                 SLOW_HISTORY_SAMPLES,
                                                 history_count, write_index,
                                                 span_samples, scope_roll_phase,
                                                 scope_view_mode,
                                                 fast_display_samples,
                                                 wave_width, &view_info);
        acquisition_sample_rate_hz = ECG_SAMPLE_RATE_HZ;
    }else if(scope_noise_zoom_active() != 0U){
        history_count = fast_history_count;
        write_index = fast_write_index;
        available = ScopeView_CopyWindow((const uint16_t *)fast_history,
                                         FAST_HISTORY_SAMPLES, history_count,
                                         write_index, span_samples,
                                         scope_roll_phase, scope_view_mode,
                                         fast_display_samples,
                                         wave_width, &view_info);
        acquisition_sample_rate_hz = ADC_SAMPLE_RATE_HZ;
    }else{
        history_count = fast_history_count;
        write_index = fast_write_index;
        available = ScopeView_CopyUniformWindow(
            (const uint16_t *)fast_history,
            FAST_HISTORY_SAMPLES, history_count,
            write_index, span_samples,
            scope_roll_phase, scope_view_mode,
            fast_display_samples, wave_width,
            &view_info);
        acquisition_sample_rate_hz = ADC_SAMPLE_RATE_HZ;
    }
    if(primask == 0U){
        __enable_irq();
    }
    source_available = view_info.source_count;
    if(available == 0U){
        min_value = 0U;
        max_value = 0U;
    }else{
        min_value = view_info.minimum;
        max_value = view_info.maximum;
    }
    /*
     * Keep a fixed vertical scale.  The old min/max normalization made every
     * input amplitude fill the same height, hiding the difference between
     * small and large signals.  The analog front end is Uadc=(5V-Vin)/2,
     * therefore Vin=0V is represented by approximately 2.5V at the ADC.
     */
    midpoint = (uint16_t)(((uint32_t)2500U * ecg_vref_value + 605U) / 1210U);
    display_vpp_mv = scope_display_vpp_mv[scope_range_index];
    half_range = (uint16_t)((display_vpp_mv * ecg_vref_value + 2420U) /
                            (2U * 2420U));
    /* With no input the ADC can sit near zero instead of the 0V reference
     * used above. Center a nearly flat trace so the idle line remains visible.
     */
    if((uint16_t)(max_value - min_value) < SCOPE_IDLE_SPAN_COUNTS){
        midpoint = (uint16_t)(((uint32_t)max_value + min_value) / 2U);
    }
    if(half_range < 16U){
        half_range = 16U;
    }
    ecg_vpp_mv = (uint16_t)(((uint32_t)(max_value - min_value) * 1210U) /
                            ecg_vref_value);
    scope_vpp_mv = (uint16_t)(((uint32_t)(max_value - min_value) * 2420U) /
                              ecg_vref_value);
    if(scope_vpp_mv > 9999U){
        scope_vpp_mv = 9999U;
    }
    display_sample_rate_hz = acquisition_sample_rate_hz;
    if((source_available > 1U) && (available > 1U)){
        display_sample_rate_hz = (acquisition_sample_rate_hz *
                                  (available - 1U)) /
                                 (source_available - 1U);
    }
    scope_sample_frequency = measure_sample_frequency(fast_display_samples, available,
                                                      min_value, max_value,
                                                      display_sample_rate_hz);
    scope_metrics = ScopeMetrics_Analyze(fast_display_samples, available,
                                         min_value, max_value,
                                         SCOPE_DUTY_MIN_SPAN_COUNTS);

    draw_wave_grid(wave_right);
    for(point = 0U; point < available; point++){
        sample = fast_display_samples[point];
        if(available > 1U){
            x = (uint16_t)(WAVE_X +
                           (((uint32_t)point * (wave_width - 1U)) /
                            (available - 1U)));
        }else{
            x = WAVE_X;
        }
        /* Compensate the inverting analog front end in fast scope modes. */
        y_position = (int32_t)(WAVE_Y + (WAVE_HEIGHT / 2U)) +
                     (((int32_t)sample - midpoint) *
                      (int32_t)((WAVE_HEIGHT / 2U) - 3U) / half_range);
        if(y_position < (int32_t)WAVE_Y){
            y_position = WAVE_Y;
        }else if(y_position > (int32_t)WAVE_BOTTOM){
            y_position = WAVE_BOTTOM;
        }
        y = (uint16_t)y_position;

        if((have_previous != 0U) && (x >= previous_x)){
            TFT_DrawLine(previous_x, previous_y, x, y, GREEN);
        }
        previous_x = x;
        previous_y = y;
        have_previous = 1U;
    }

    if((scope_view_mode == SCOPE_VIEW_FREE) &&
       (history_count > source_available) &&
       (source_available != 0U)){
        max_lag = (uint16_t)(history_count - source_available);
        motion_step = (uint16_t)(((uint32_t)source_available * 2U +
                                  wave_width - 1U) / wave_width);
        if(motion_step == 0U){
            motion_step = 1U;
        }
        if(scope_roll_toward_older != 0U){
            if((scope_roll_phase >= max_lag) ||
               (motion_step >= (uint16_t)(max_lag - scope_roll_phase))){
                scope_roll_phase = max_lag;
                scope_roll_toward_older = 0U;
            }else{
                scope_roll_phase = (uint16_t)(scope_roll_phase + motion_step);
            }
        }else if((scope_roll_phase == 0U) ||
                 (motion_step >= scope_roll_phase)){
            scope_roll_phase = 0U;
            scope_roll_toward_older = 1U;
        }else{
            scope_roll_phase = (uint16_t)(scope_roll_phase - motion_step);
        }
    }else{
        scope_roll_phase = 0U;
        scope_roll_toward_older = 1U;
    }
}

static void ecg_wave_show(uint16_t vref_value)
{
    vital_trace_column_t column;
    uint32_t sample_count;
    uint32_t pending_count;
    uint16_t index;
    uint16_t magnitude;
    uint16_t x;
    uint16_t y_first;
    uint16_t y_minimum;
    uint16_t y_maximum;
    uint16_t y_last;
    int8_t sample;

    if(vref_value != 0U){
        ecg_vref_value = vref_value;
    }
    if(vital_trace_reset_pending != 0U){
        vital_trace_restart(1U);
        return;
    }

    sample_count = ecg_sample_count;
    if(sample_count < vital_trace_next_sample){
        vital_trace_restart(1U);
        return;
    }
    pending_count = sample_count - vital_trace_next_sample;
    if(pending_count > ECG_HISTORY_SAMPLES){
        vital_trace_restart(1U);
        return;
    }

    while(vital_trace_next_sample < sample_count){
        index = (uint16_t)(vital_trace_next_sample % ECG_HISTORY_SAMPLES);
        sample = ecg_history[index];
        magnitude = ecg_abs16(sample);
        if(magnitude > vital_trace_peak_abs){
            vital_trace_peak_abs = magnitude;
        }
        if((magnitude > vital_trace_scale_abs) &&
           (vital_trace_scale_abs < 127U)){
            vital_trace_scale_abs = (magnitude <= 64U) ? 64U : 127U;
            draw_wave_grid(WAVE_RIGHT);
            VitalTrace_Init(&vital_trace_state, WAVE_WIDTH,
                            timebase_samples[ecg_timebase_index]);
            vital_trace_have_previous = 0U;
        }

        if(VitalTrace_PushSample(&vital_trace_state, sample, &column) != 0U){
            x = (uint16_t)(WAVE_X + column.column);
            y_first = vital_trace_map_y(column.first);
            y_minimum = vital_trace_map_y(column.minimum);
            y_maximum = vital_trace_map_y(column.maximum);
            y_last = vital_trace_map_y(column.last);

            clear_wave_column(x);
            if((column.connect_previous != 0U) &&
               (vital_trace_have_previous != 0U) &&
               (x == (uint16_t)(vital_trace_previous_x + 1U))){
                TFT_DrawLine(vital_trace_previous_x,
                             vital_trace_previous_y,
                             x, y_first, GREEN);
            }
            TFT_DrawLine(x, y_minimum, x, y_maximum, GREEN);
            vital_trace_previous_x = x;
            vital_trace_previous_y = y_last;
            vital_trace_have_previous = 1U;

            x = (uint16_t)(WAVE_X + vital_trace_state.cursor_column);
            clear_wave_column(x);
            if(vital_trace_state.cursor_column == 0U){
                vital_trace_have_previous = 0U;
            }
        }
        vital_trace_next_sample++;
    }

    ecg_vpp_mv = (uint16_t)(((uint32_t)vital_trace_peak_abs *
                              2U * 8U * 1210U) / ecg_vref_value);
    if(ecg_vpp_mv > 9999U){
        ecg_vpp_mv = 9999U;
    }
}

static void scope_show_ui(void)
{
    char show_data[12];
    uint32_t input_frequency = get_freq_value();
    uint32_t output_frequency = get_pwm_out_freq();

    if(scope_startup_ready == 0U){
        input_frequency = 0U;
    }else if(input_frequency == 0U){
        input_frequency = scope_sample_frequency;
    }

    if(scope_startup_ready == 0U){
        TFT_ShowString(44U, 0U, (const uint8_t *)"WAIT", BLACK, YELLOW, 16U, 0U);
    }else if(osc_stop_bit == OSC_RUN){
        TFT_ShowString(44U, 0U, (const uint8_t *)"RUN ", BLACK, GREEN, 16U, 0U);
    }else{
        TFT_ShowString(44U, 0U, (const uint8_t *)"HOLD", BLACK, YELLOW, 16U, 0U);
    }
    TFT_ShowString(100U, 0U,
                   (const uint8_t *)scope_timebase_labels[scope_timebase_index],
                   YELLOW, BLACK, 16U, 0U);

    if(scope_noise_zoom_active() == 0U){
        if(get_pwm_state() == PWM_ON){
            TFT_ShowString(128U, 34U, (const uint8_t *)"ON ", GREEN, BLACK, 16U, 0U);
        }else{
            TFT_ShowString(128U, 34U, (const uint8_t *)"OFF", RED, BLACK, 16U, 0U);
        }

        if(output_frequency >= 1000U){
            sprintf(show_data, "%3luK", (unsigned long)(output_frequency / 1000U));
        }else{
            sprintf(show_data, "%4lu", (unsigned long)output_frequency);
        }
        TFT_Fill(128U, 66U, 160U, 82U, BLACK);
        TFT_ShowString(128U, 66U, (const uint8_t *)show_data, YELLOW, BLACK, 16U, 0U);

        TFT_ShowString(128U, 80U,
                       (const uint8_t *)scope_range_labels[scope_range_index],
                       CYAN, BLACK, 16U, 0U);
    }

    if(scope_vpp_mv < 1000U){
        sprintf(show_data, "%3umV", (unsigned int)scope_vpp_mv);
    }else{
        sprintf(show_data, "%u.%02uV",
                (unsigned int)(scope_vpp_mv / 1000U),
                (unsigned int)((scope_vpp_mv % 1000U) / 10U));
    }
    TFT_Fill(2U, 112U, 56U, 128U, BLACK);
    TFT_ShowString(2U, 112U, (const uint8_t *)show_data, GREEN, BLACK, 16U, 0U);

    if(input_frequency >= 1000U){
        sprintf(show_data, "%lu.%luK",
                (unsigned long)(input_frequency / 1000U),
                (unsigned long)((input_frequency % 1000U) / 100U));
    }else{
        sprintf(show_data, "%4lu", (unsigned long)input_frequency);
    }
    TFT_Fill(58U, 112U, 110U, 128U, BLACK);
    TFT_ShowString(58U, 112U, (const uint8_t *)show_data, CYAN, BLACK, 16U, 0U);

    if(scope_metrics.valid != 0U){
        sprintf(show_data, "%3u%%",
                (unsigned int)scope_metrics.input_duty_percent);
    }else{
        sprintf(show_data, "---%%");
    }
    TFT_Fill(128U, 112U, 160U, 128U, BLACK);
    TFT_ShowString(128U, 112U, (const uint8_t *)show_data, YELLOW, BLACK, 16U, 0U);
}

static void scope_static_ui(void)
{
    TFT_Fill(0U, 0U, 160U, 128U, BLACK);
    TFT_ShowString(2U, 0U,
                   (const uint8_t *)((scope_noise_zoom_active() != 0U) ?
                                     "NOISE" : "SCOPE"),
                   WHITE, BLACK, 16U, 0U);
    TFT_ShowString(80U, 0U, (const uint8_t *)"TB", WHITE, BLACK, 16U, 0U);

    TFT_DrawLine(0U, 16U, 159U, 16U, CYAN);
    TFT_DrawLine(0U, 96U, 159U, 96U, CYAN);

    if(scope_noise_zoom_active() == 0U){
        TFT_DrawLine(124U, 16U, 124U, 95U, CYAN);
        TFT_ShowString(128U, 18U, (const uint8_t *)"OUT", WHITE, BLACK, 16U, 0U);
        TFT_ShowString(128U, 50U, (const uint8_t *)"FOUT", WHITE, BLACK, 16U, 0U);
    }
    TFT_ShowString(2U, 96U, (const uint8_t *)"Vpp", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(58U, 96U, (const uint8_t *)"FIN", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(128U, 96U, (const uint8_t *)"DIN", WHITE, BLACK, 16U, 0U);

    draw_wave_grid(scope_active_wave_right());
    scope_show_ui();
}

static void scope_wave_show(uint16_t vref_value)
{
    uint8_t slow_timebase;

    if(vref_value != 0U){
        ecg_vref_value = vref_value;
    }
    slow_timebase = (uint8_t)(scope_timebase_index >= SCOPE_FAST_TIMEBASE_COUNT);
    draw_scope_wave(scope_timebase_samples[scope_timebase_index], slow_timebase);
}

void TFT_StaticUI(void)
{
    if(app_mode == DEMO15_MODE_ECG_MONITOR){
        vital_static_ui(0U);
    }else if(app_mode == DEMO15_MODE_SPO2_MONITOR){
        vital_static_ui(1U);
    }else if(scope_ecg_view != 0U){
        vital_static_ui(2U);
    }else{
        scope_static_ui();
    }
}

void TFT_ShowUI(void)
{
    if(app_mode == DEMO15_MODE_ECG_MONITOR){
        vital_show_ui(0U);
    }else if(app_mode == DEMO15_MODE_SPO2_MONITOR){
        vital_show_ui(1U);
    }else if(scope_ecg_view != 0U){
        vital_show_ui(2U);
    }else{
        scope_show_ui();
    }
}

void osc_waveShow(uint16_t vref_value)
{
    if((app_mode == DEMO15_MODE_ECG_MONITOR) ||
       (app_mode == DEMO15_MODE_SPO2_MONITOR) ||
       ((app_mode == DEMO15_MODE_OSCILLOSCOPE) && (scope_ecg_view != 0U))){
        ecg_wave_show(vref_value);
    }else{
        scope_wave_show(vref_value);
    }
}

void Demo15_SelectNextMode(void)
{
    uint32_t primask;
    demo15_mode_t next_mode;
    uint8_t generator_was_active;

    next_mode = (demo15_mode_t)(((uint8_t)app_mode + 1U) %
                                (uint8_t)DEMO15_MODE_COUNT);
    generator_was_active = ecg_generator_active;
    primask = __get_PRIMASK();
    __disable_irq();
    app_mode = next_mode;
    osc_stop_bit = OSC_RUN;
    if(app_mode == DEMO15_MODE_SPO2_MONITOR){
        SpO2Receiver_Init(&spo2_receiver);
        vital_analysis_reset();
    }
    if(primask == 0U){
        __enable_irq();
    }
    if((app_mode == DEMO15_MODE_ECG_MONITOR) ||
       ((app_mode == DEMO15_MODE_OSCILLOSCOPE) && (scope_ecg_view != 0U))){
        ecg_generator_start();
        if(generator_was_active == 0U){
            vital_analysis_reset();
        }
    }else{
        ecg_generator_stop();
    }
    TFT_StaticUI();
}

demo15_mode_t Demo15_GetMode(void)
{
    return app_mode;
}

void key_scanf_handle(const uint16_t key_pin, const uint8_t key_state)
{
    uint32_t primask;
    uint16_t temp_period;
    float temp_duty;

    const uint8_t ecg_output_mode =
        (uint8_t)((app_mode == DEMO15_MODE_ECG_MONITOR) ||
                  ((app_mode == DEMO15_MODE_OSCILLOSCOPE) &&
                   (scope_ecg_view != 0U)));

    if(key_pin == KEY1_Pin){
        if(key_state == KeyPress){
            if(ecg_output_mode != 0U){
                primask = __get_PRIMASK();
                __disable_irq();
                SignalOutput_SetEnabled((uint8_t)(SignalOutput_IsEnabled() == 0U));
                if(primask == 0U){
                    __enable_irq();
                }
            }else if(get_pwm_state() == PWM_OFF){
                set_pwm_period(get_pwm_period());
                set_pwm_duty(get_pwm_duty());
                set_pwm_state(PWM_ON);
            }else{
                set_pwm_state(PWM_OFF);
            }
        }
    }else if(key_pin == KEY2_Pin){
        if((key_state == KeyPress) && (ecg_output_mode == 0U)){
            temp_duty = get_pwm_duty() * 1.0f / get_pwm_period();
            temp_period = (uint16_t)(get_pwm_period() / 2U);
            if(temp_period < 125U){
                temp_period = 1000U;
            }
            set_pwm_period(temp_period);
            set_pwm_duty((uint16_t)(temp_period * temp_duty));
        }
    }else if(key_pin == KEY3_Pin){
        if(key_state == KeyPress){
            if(ecg_output_mode != 0U){
                primask = __get_PRIMASK();
                __disable_irq();
                SignalOutput_ToggleEcgPreset();
                if(primask == 0U){
                    __enable_irq();
                }
            }else if(get_pwm_duty() >= get_pwm_period()){
                temp_duty = 0.0f;
            }else{
                temp_duty = get_pwm_period() * 0.04f + get_pwm_duty();
                if(temp_duty > get_pwm_period()){
                    temp_duty = get_pwm_period();
                }
            }
            set_pwm_duty((uint16_t)temp_duty);
        }
    }else if((key_pin == KEYD_Pin) && (key_state == KeyPress)){
        if(get_osc_stop_bit() == OSC_RUN){
            set_osc_stop_bit(OSC_PAUSE);
        }else{
            set_osc_stop_bit(OSC_RUN);
        }
    }
}
