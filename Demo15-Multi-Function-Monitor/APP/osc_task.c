#include "osc_task.h"

#include <stdio.h>

#include "hw_key.h"
#include "hess_analyzer.h"
#include "mid_adc.h"
#include "mid_lcd.h"
#include "mid_pwm.h"
#include "mid_timer.h"
#include "scope_view.h"
#include "signal_output.h"

#define ADC_SAMPLE_RATE_HZ       20000U
#define ECG_SAMPLE_RATE_HZ       250U
#define ECG_DECIMATION           (ADC_SAMPLE_RATE_HZ / ECG_SAMPLE_RATE_HZ)
#define ADC_DMA_SAMPLES          (ECG_DECIMATION * 2U)
#define SCOPE_DISPLAY_VPP_MV     5000U
#define SCOPE_SMALL_VPP_MV       1000U
#define SCOPE_IDLE_SPAN_COUNTS   64U
#define FAST_HISTORY_SAMPLES     800U
#define FAST_DISPLAY_SAMPLES     WAVE_WIDTH
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

static uint16_t adc_dma_samples[ADC_DMA_SAMPLES];
static volatile uint16_t fast_history[FAST_HISTORY_SAMPLES];
static uint16_t fast_display_samples[FAST_DISPLAY_SAMPLES];
static volatile uint16_t fast_write_index;
static volatile uint16_t fast_history_count;
static volatile uint32_t fast_sample_count;
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
static uint8_t scope_small_signal;
static scope_view_mode_t scope_view_mode = SCOPE_VIEW_FREE;
static uint16_t scope_roll_phase;
static demo15_mode_t app_mode = DEMO15_MODE_OSCILLOSCOPE;
static uint8_t osc_stop_bit = OSC_RUN;
static uint16_t ecg_vpp_mv;
static uint16_t scope_vpp_mv;
static uint32_t scope_sample_frequency;

static int32_t baseline_q8;
static int16_t filtered_value;

static void ecg_show_ui(void);

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
            ecg_process_sample(raw_value);
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
    ecg_timebase_index = value;
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
    scope_timebase_index = value;
}

uint8_t get_scope_timebase_index(void)
{
    return scope_timebase_index;
}

void toggle_scope_small_signal(void)
{
    scope_small_signal = (scope_small_signal == 0U) ? 1U : 0U;
}

uint8_t get_scope_small_signal(void)
{
    return scope_small_signal;
}

void cycle_scope_view_mode(void)
{
    scope_view_mode = (scope_view_mode == SCOPE_VIEW_FALLING) ?
                      SCOPE_VIEW_FREE :
                      (scope_view_mode_t)(scope_view_mode + 1);
    scope_roll_phase = 0U;
}

scope_view_mode_t get_scope_view_mode(void)
{
    return scope_view_mode;
}

void set_osc_stop_bit(uint8_t value)
{
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

static void draw_wave_grid(void)
{
    uint16_t x;
    uint16_t y;

    TFT_Fill(WAVE_X, WAVE_Y, WAVE_RIGHT + 1U, WAVE_BOTTOM + 1U, BLACK);
    TFT_DrawLine(WAVE_X - 1U, WAVE_Y - 1U, WAVE_RIGHT + 1U, WAVE_Y - 1U, CYAN);
    TFT_DrawLine(WAVE_X - 1U, WAVE_BOTTOM + 1U, WAVE_RIGHT + 1U, WAVE_BOTTOM + 1U, CYAN);
    TFT_DrawLine(WAVE_X - 1U, WAVE_Y - 1U, WAVE_X - 1U, WAVE_BOTTOM + 1U, CYAN);
    TFT_DrawLine(WAVE_RIGHT + 1U, WAVE_Y - 1U, WAVE_RIGHT + 1U, WAVE_BOTTOM + 1U, CYAN);

    for(x = WAVE_X + 17U; x < WAVE_RIGHT; x += 18U){
        for(y = WAVE_Y + 2U; y < WAVE_BOTTOM; y += 4U){
            TFT_DrawPoint(x, y, GRAYBLUE);
        }
    }
    for(y = WAVE_Y + 18U; y < WAVE_BOTTOM; y += 19U){
        for(x = WAVE_X + 2U; x < WAVE_RIGHT; x += 4U){
            TFT_DrawPoint(x, y, GRAYBLUE);
        }
    }
}

static void ecg_static_ui(void)
{
    TFT_Fill(0U, 0U, 160U, 128U, BLACK);
    TFT_ShowString(2U, 0U, (const uint8_t *)"ECG", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(80U, 0U, (const uint8_t *)"TB", WHITE, BLACK, 16U, 0U);

    TFT_DrawLine(0U, 16U, 159U, 16U, CYAN);
    TFT_DrawLine(124U, 16U, 124U, 95U, CYAN);
    TFT_DrawLine(0U, 96U, 159U, 96U, CYAN);

    TFT_ShowString(128U, 18U, (const uint8_t *)"OUT", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(128U, 50U, (const uint8_t *)"BPM", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(128U, 80U, (const uint8_t *)"/min", WHITE, BLACK, 16U, 0U);

    TFT_ShowString(2U, 96U, (const uint8_t *)"Vpp", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(44U, 96U, (const uint8_t *)"SIG", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(76U, 96U, (const uint8_t *)"SPAN", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(128U, 96U, (const uint8_t *)"PWM", WHITE, BLACK, 16U, 0U);

    draw_wave_grid();
    ecg_show_ui();
}

static void ecg_show_ui(void)
{
    char show_data[12];
    uint32_t peak_age = ecg_sample_count - ecg_last_peak_sample;
    uint8_t heart_active = (uint8_t)((ecg_peak_seen != 0U) &&
                                     (peak_age < ECG_HEART_PULSE_SAMPLES));
    uint8_t signal_ok = (uint8_t)((ecg_bpm != 0U) &&
                                  (peak_age < ECG_SIGNAL_TIMEOUT));

    draw_heart(heart_active);

    if(osc_stop_bit == OSC_RUN){
        TFT_ShowString(44U, 0U, (const uint8_t *)"RUN ", BLACK, GREEN, 16U, 0U);
    }else{
        TFT_ShowString(44U, 0U, (const uint8_t *)"HOLD", BLACK, YELLOW, 16U, 0U);
    }

    TFT_ShowString(100U, 0U,
                   (const uint8_t *)timebase_labels[ecg_timebase_index],
                   YELLOW, BLACK, 16U, 0U);

    if(get_pwm_state() == PWM_ON){
        TFT_ShowString(128U, 34U, (const uint8_t *)"ON ", GREEN, BLACK, 16U, 0U);
    }else{
        TFT_ShowString(128U, 34U, (const uint8_t *)"OFF", RED, BLACK, 16U, 0U);
    }

    if(ecg_bpm != 0U){
        sprintf(show_data, "%3u", (unsigned int)ecg_bpm);
    }else{
        sprintf(show_data, "---");
    }
    TFT_ShowString(128U, 64U, (const uint8_t *)show_data, YELLOW, BLACK, 16U, 0U);

    if(ecg_vpp_mv < 1000U){
        sprintf(show_data, "%3umV", (unsigned int)ecg_vpp_mv);
    }else{
        sprintf(show_data, "%u.%02uV",
                (unsigned int)(ecg_vpp_mv / 1000U),
                (unsigned int)((ecg_vpp_mv % 1000U) / 10U));
    }
    TFT_ShowString(2U, 112U, (const uint8_t *)show_data, GREEN, BLACK, 16U, 0U);

    if(signal_ok != 0U){
        TFT_ShowString(44U, 112U, (const uint8_t *)"OK ", GREEN, BLACK, 16U, 0U);
    }else{
        TFT_ShowString(44U, 112U, (const uint8_t *)"WAIT", YELLOW, BLACK, 16U, 0U);
    }

    TFT_ShowString(76U, 112U,
                   (const uint8_t *)timebase_labels[ecg_timebase_index],
                   CYAN, BLACK, 16U, 0U);

    if(get_pwm_state() == PWM_ON){
        TFT_ShowString(128U, 112U, (const uint8_t *)"RUN", GREEN, BLACK, 16U, 0U);
    }else{
        TFT_ShowString(128U, 112U, (const uint8_t *)"OFF", RED, BLACK, 16U, 0U);
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
    uint16_t x;
    uint16_t y;
    uint8_t have_previous = 0U;
    uint32_t primask;
    uint32_t display_vpp_mv;
    uint32_t acquisition_sample_rate_hz;
    uint32_t display_sample_rate_hz;
    int32_t y_position;
    scope_view_info_t view_info = {0U};

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
                                                 FAST_DISPLAY_SAMPLES, &view_info);
        acquisition_sample_rate_hz = ECG_SAMPLE_RATE_HZ;
    }else{
        history_count = fast_history_count;
        write_index = fast_write_index;
        available = ScopeView_CopyWindow((const uint16_t *)fast_history,
                                         FAST_HISTORY_SAMPLES, history_count,
                                         write_index, span_samples,
                                         scope_roll_phase, scope_view_mode,
                                         fast_display_samples,
                                         FAST_DISPLAY_SAMPLES, &view_info);
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
        if(scope_view_mode == SCOPE_VIEW_FREE){
            scope_roll_phase = (uint16_t)((scope_roll_phase + 2U) %
                                          source_available);
        }
    }
    /*
     * Keep a fixed vertical scale.  The old min/max normalization made every
     * input amplitude fill the same height, hiding the difference between
     * small and large signals.  The analog front end is Uadc=(5V-Vin)/2,
     * therefore Vin=0V is represented by approximately 2.5V at the ADC.
     */
    midpoint = (uint16_t)(((uint32_t)2500U * ecg_vref_value + 605U) / 1210U);
    display_vpp_mv = (scope_small_signal != 0U) ? SCOPE_SMALL_VPP_MV : SCOPE_DISPLAY_VPP_MV;
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

    draw_wave_grid();
    for(point = 0U; point < available; point++){
        sample = fast_display_samples[point];
        if(available > 1U){
            x = (uint16_t)(WAVE_X +
                           (((uint32_t)point * (WAVE_WIDTH - 1U)) /
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
}

static void ecg_wave_show(uint16_t vref_value)
{
    uint16_t span_samples = timebase_samples[ecg_timebase_index];
    uint16_t history_count = ecg_history_count;
    uint16_t write_index = ecg_write_index;
    uint32_t sample_count = ecg_sample_count;
    uint16_t available;
    uint16_t x;
    uint16_t point;
    uint16_t age;
    uint16_t index;
    uint16_t max_abs = 32U;
    uint16_t magnitude;
    uint16_t previous_x = 0U;
    uint16_t previous_y = 0U;
    uint16_t y;
    int32_t y_position;
    int16_t sample;
    uint8_t have_previous = 0U;
    uint32_t peak_age;
    uint32_t sample_number;

    if(vref_value != 0U){
        ecg_vref_value = vref_value;
    }
    available = (history_count < span_samples) ? history_count : span_samples;

    for(age = 0U; age < available; age++){
        index = (uint16_t)((write_index + ECG_HISTORY_SAMPLES - 1U - age) % ECG_HISTORY_SAMPLES);
        magnitude = ecg_abs16(ecg_history[index]);
        if(magnitude > max_abs){
            max_abs = magnitude;
        }
    }

    ecg_vpp_mv = (uint16_t)(((uint32_t)max_abs * 2U * 8U * 1210U) /
                            ecg_vref_value);
    if(ecg_vpp_mv > 9999U){
        ecg_vpp_mv = 9999U;
    }

    draw_wave_grid();
    for(point = available; point > 0U; point--){
        age = point - 1U;
        index = (uint16_t)((write_index + ECG_HISTORY_SAMPLES - 1U - age) % ECG_HISTORY_SAMPLES);
        sample = ecg_history[index];
        sample_number = sample_count - 1U - age;
        x = (uint16_t)(WAVE_X +
                       (((sample_number % span_samples) * (WAVE_WIDTH - 1U)) /
                        (span_samples - 1U)));
        y_position = (int32_t)(WAVE_Y + (WAVE_HEIGHT / 2U)) -
                     ((int32_t)sample * (int32_t)((WAVE_HEIGHT / 2U) - 3U) / max_abs);
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

    if((ecg_peak_seen != 0U) && (sample_count >= ecg_last_peak_sample)){
        peak_age = sample_count - ecg_last_peak_sample;
        if(peak_age < span_samples){
            x = (uint16_t)(WAVE_X +
                           ((((ecg_last_peak_sample - 1U) % span_samples) *
                             (WAVE_WIDTH - 1U)) / (span_samples - 1U)));
            TFT_DrawLine(x, WAVE_Y, x, WAVE_Y + 4U, RED);
            if(x > WAVE_X){
                TFT_DrawPoint(x - 1U, WAVE_Y + 1U, RED);
            }
            if(x < WAVE_RIGHT){
                TFT_DrawPoint(x + 1U, WAVE_Y + 1U, RED);
            }
        }
    }
}

static void scope_show_ui(void)
{
    char show_data[12];
    uint32_t input_frequency = get_freq_value();
    uint32_t output_frequency = get_pwm_out_freq();
    uint16_t duty = (uint16_t)(((uint32_t)get_pwm_duty() * 100U) /
                               get_pwm_period());

    if(input_frequency == 0U){
        input_frequency = scope_sample_frequency;
    }

    if(osc_stop_bit == OSC_RUN){
        TFT_ShowString(44U, 0U, (const uint8_t *)"RUN ", BLACK, GREEN, 16U, 0U);
    }else{
        TFT_ShowString(44U, 0U, (const uint8_t *)"HOLD", BLACK, YELLOW, 16U, 0U);
    }
    TFT_ShowString(100U, 0U,
                   (const uint8_t *)scope_timebase_labels[scope_timebase_index],
                   YELLOW, BLACK, 16U, 0U);

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
                   (const uint8_t *)((scope_small_signal != 0U) ? "1Vpp" : "5Vpp"),
                   CYAN, BLACK, 16U, 0U);

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

    sprintf(show_data, "%3u%%", (unsigned int)duty);
    TFT_ShowString(128U, 112U, (const uint8_t *)show_data, YELLOW, BLACK, 16U, 0U);
}

static void scope_static_ui(void)
{
    TFT_Fill(0U, 0U, 160U, 128U, BLACK);
    TFT_ShowString(2U, 0U, (const uint8_t *)"SCOPE", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(80U, 0U, (const uint8_t *)"TB", WHITE, BLACK, 16U, 0U);

    TFT_DrawLine(0U, 16U, 159U, 16U, CYAN);
    TFT_DrawLine(124U, 16U, 124U, 95U, CYAN);
    TFT_DrawLine(0U, 96U, 159U, 96U, CYAN);

    TFT_ShowString(128U, 18U, (const uint8_t *)"OUT", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(128U, 50U, (const uint8_t *)"FOUT", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(2U, 96U, (const uint8_t *)"Vpp", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(58U, 96U, (const uint8_t *)"FIN", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(128U, 96U, (const uint8_t *)"DUTY", WHITE, BLACK, 16U, 0U);

    draw_wave_grid();
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
        ecg_static_ui();
    }else if(app_mode == DEMO15_MODE_HESS_ANALYZER){
        HESSAnalyzer_DrawStatic();
        draw_wave_grid();
        HESSAnalyzer_ShowUI((uint8_t)(osc_stop_bit == OSC_RUN),
                            timebase_labels[ecg_timebase_index]);
    }else{
        scope_static_ui();
    }
}

void TFT_ShowUI(void)
{
    if(app_mode == DEMO15_MODE_ECG_MONITOR){
        ecg_show_ui();
    }else if(app_mode == DEMO15_MODE_HESS_ANALYZER){
        HESSAnalyzer_ShowUI((uint8_t)(osc_stop_bit == OSC_RUN),
                            timebase_labels[ecg_timebase_index]);
    }else{
        scope_show_ui();
    }
}

void osc_waveShow(uint16_t vref_value)
{
    if((app_mode == DEMO15_MODE_ECG_MONITOR) ||
       (app_mode == DEMO15_MODE_HESS_ANALYZER)){
        ecg_wave_show(vref_value);
    }else{
        scope_wave_show(vref_value);
    }
}

void Demo15_SelectNextMode(void)
{
    app_mode = (demo15_mode_t)(((uint8_t)app_mode + 1U) %
                               (uint8_t)DEMO15_MODE_COUNT);
    osc_stop_bit = OSC_RUN;
    TFT_StaticUI();
}

demo15_mode_t Demo15_GetMode(void)
{
    return app_mode;
}

void key_scanf_handle(const uint16_t key_pin, const uint8_t key_state)
{
    uint16_t temp_period;
    float temp_duty;

    if(key_pin == KEY1_Pin){
        if(key_state == KeyPress){
            if(get_pwm_state() == PWM_OFF){
                set_pwm_period(get_pwm_period());
                set_pwm_duty(get_pwm_duty());
                set_pwm_state(PWM_ON);
            }else{
                set_pwm_state(PWM_OFF);
            }
        }
    }else if(key_pin == KEY2_Pin){
        if(key_state == KeyPress){
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
            if(get_pwm_duty() >= get_pwm_period()){
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
