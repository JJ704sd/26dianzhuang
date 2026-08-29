#include "ecg_task.h"

#include <stdio.h>
#include <string.h>

#include "ecg_core.h"
#include "hw_key.h"
#include "mid_adc.h"
#include "mid_lcd.h"
#include "mid_pwm.h"

#define ECG_SAMPLE_RATE_HZ       250U
#define ECG_SAMPLE_INTERVAL_MS   4U
#define ECG_HISTORY_RATE_HZ      125U
#define ECG_HISTORY_DECIMATION   (ECG_SAMPLE_RATE_HZ / ECG_HISTORY_RATE_HZ)
#define ECG_PWM_PERIOD_TICKS     1000U
#define ECG_OUTPUT_BPM_MIN       60U
#define ECG_OUTPUT_BPM_MAX       120U
#define ECG_OUTPUT_BPM_STEP      6U
#define ECG_OUTPUT_BPM_DEFAULT   72U
#define ECG_BPM_TIMEOUT_MS       3000U
#define ECG_BUFFER_SIZE          1000U
#define ECG_PLOT_X0              2U
#define ECG_PLOT_X1              103U
#define ECG_PLOT_Y0              18U
#define ECG_PLOT_Y1              86U
#define ECG_PLOT_CENTER_Y        55
#define ECG_PLOT_AMPLITUDE       30

typedef struct
{
    uint16_t period;
    uint16_t duty;
    uint8_t state;
} ecg_saved_pwm_t;

static ecg_detector_t detector;
static ecg_quality_monitor_t quality_monitor;
static ecg_saved_pwm_t saved_pwm;
static volatile int8_t sample_buffer[ECG_BUFFER_SIZE];
static int16_t old_plot[ECG_PLOT_X1 - ECG_PLOT_X0 + 1U];
static volatile uint16_t buffer_head;
static volatile uint16_t buffer_count;
static volatile uint8_t buffer_sequence;
static uint16_t phase_sample;
static volatile uint8_t phase_reset_requested;
static uint8_t sample_tick_ms;
static uint8_t history_decimator;
static int32_t adc_baseline_q8;
static uint8_t adc_baseline_ready;
static volatile uint16_t heart_age_ms;
static volatile uint16_t bpm_age_ms;
static volatile uint16_t measured_bpm;
static volatile uint16_t output_bpm = ECG_OUTPUT_BPM_DEFAULT;
static volatile uint8_t signal_quality = ECG_SIGNAL_UNKNOWN;
static volatile uint8_t alarm_flags;
static volatile uint8_t bpm_timed_out;
static uint8_t display_span = ECG_DISPLAY_SPAN_MIN;
static volatile uint8_t active;

static void ecg_clear_old_plot(void)
{
    uint16_t i;

    for (i = 0U; i < (uint16_t)(sizeof(old_plot) / sizeof(old_plot[0])); ++i)
    {
        old_plot[i] = ECG_PLOT_CENTER_Y;
    }
}

static uint16_t ecg_period_samples(void)
{
    return (uint16_t)(((uint32_t)60U * ECG_SAMPLE_RATE_HZ +
                       (output_bpm / 2U)) / output_bpm);
}

static uint16_t ecg_display_period_samples(void)
{
    const uint16_t bpm = (measured_bpm != 0U) ? measured_bpm : output_bpm;

    return ecg_samples_for_periods(1U, bpm, ECG_HISTORY_RATE_HZ);
}

static int16_t ecg_plot_y(int8_t sample)
{
    int16_t y = ECG_PLOT_CENTER_Y -
                (int16_t)(((int16_t)sample * ECG_PLOT_AMPLITUDE) / 125);

    if (y < (int16_t)ECG_PLOT_Y0)
    {
        y = (int16_t)ECG_PLOT_Y0;
    }
    else if (y > (int16_t)ECG_PLOT_Y1)
    {
        y = (int16_t)ECG_PLOT_Y1;
    }
    return y;
}

static void ecg_reset_signal(void)
{
    uint16_t i;

    ecg_detector_init(&detector, ECG_SAMPLE_RATE_HZ, 600, 200, 250U);
    ecg_quality_init(&quality_monitor, ECG_SAMPLE_RATE_HZ);
    for (i = 0U; i < ECG_BUFFER_SIZE; ++i)
    {
        sample_buffer[i] = 0;
    }
    buffer_head = 0U;
    buffer_count = 0U;
    buffer_sequence = 0U;
    phase_sample = 0U;
    phase_reset_requested = 0U;
    sample_tick_ms = 0U;
    history_decimator = 0U;
    adc_baseline_q8 = 0;
    adc_baseline_ready = 0U;
    heart_age_ms = 0xFFFFU;
    bpm_age_ms = 0xFFFFU;
    measured_bpm = 0U;
    signal_quality = ECG_SIGNAL_UNKNOWN;
    alarm_flags = ECG_ALARM_NONE;
    bpm_timed_out = 0U;
}

static void ecg_sample_tick(void)
{
    uint16_t period_samples;
    int16_t output_sample;
    const uint16_t adc_sample = Get_ADC_Latest();
    int16_t centered_sample;
    int16_t detector_sample;
    int16_t packed_sample;
    ecg_event_t event;

    if (phase_reset_requested != 0U)
    {
        phase_sample = 0U;
        phase_reset_requested = 0U;
    }
    period_samples = ecg_period_samples();
    output_sample = ecg_waveform_sample(phase_sample, period_samples);

    if (adc_baseline_ready == 0U)
    {
        adc_baseline_q8 = (int32_t)adc_sample << 8;
        adc_baseline_ready = 1U;
    }
    else
    {
        adc_baseline_q8 += ((((int32_t)adc_sample << 8) - adc_baseline_q8) / 64);
    }
    centered_sample = (int16_t)((int32_t)adc_sample - (adc_baseline_q8 >> 8));
    detector_sample = (centered_sample < 0) ? (int16_t)-centered_sample : centered_sample;
    event = ecg_detector_process(&detector, detector_sample);
    signal_quality = (uint8_t)ecg_quality_process(&quality_monitor,
                                                   centered_sample,
                                                   adc_sample);
    packed_sample = centered_sample / 8;

    if (packed_sample > 127)
    {
        packed_sample = 127;
    }
    else if (packed_sample < -128)
    {
        packed_sample = -128;
    }
    history_decimator++;
    if (history_decimator >= ECG_HISTORY_DECIMATION)
    {
        history_decimator = 0U;
        buffer_sequence++;
        sample_buffer[buffer_head] = (int8_t)packed_sample;
        buffer_head = (uint16_t)((buffer_head + 1U) % ECG_BUFFER_SIZE);
        if (buffer_count < ECG_BUFFER_SIZE)
        {
            buffer_count++;
        }
        buffer_sequence++;
    }

    if (get_pwm_state() == PWM_ON)
    {
        set_pwm_duty(ecg_pwm_duty_from_sample(output_sample, ECG_PWM_PERIOD_TICKS));
    }

    if (event.r_peak != 0U)
    {
        heart_age_ms = 0U;
    }
    if ((event.rr_ms != 0U) && (event.bpm != 0U))
    {
        measured_bpm = event.bpm;
        bpm_age_ms = 0U;
        bpm_timed_out = 0U;
    }
    alarm_flags = ecg_alarm_evaluate(measured_bpm,
                                     (ecg_signal_quality_t)signal_quality,
                                     bpm_timed_out);

    phase_sample++;
    if (phase_sample >= period_samples)
    {
        phase_sample = 0U;
    }
}

static void ecg_draw_grid(void)
{
    uint16_t x;
    uint16_t y;

    for (x = ECG_PLOT_X0; x <= ECG_PLOT_X1; x += 10U)
    {
        for (y = ECG_PLOT_Y0; y <= ECG_PLOT_Y1; y += 8U)
        {
            TFT_DrawPoint(x, y, DARKBLUE);
        }
    }
    for (x = ECG_PLOT_X0; x <= ECG_PLOT_X1; x += 2U)
    {
        TFT_DrawPoint(x, (uint16_t)ECG_PLOT_CENTER_Y, DARKBLUE);
    }
}

static void ecg_show_heart(uint8_t expanded)
{
    TFT_Fill(27U, 3U, 41U, 15U, BLACK);
    if (expanded != 0U)
    {
        TFT_DrawLine(29U, 6U, 39U, 6U, RED);
        TFT_DrawLine(28U, 7U, 40U, 7U, RED);
        TFT_DrawLine(29U, 8U, 39U, 8U, RED);
        TFT_DrawLine(30U, 9U, 38U, 9U, RED);
        TFT_DrawLine(31U, 10U, 37U, 10U, RED);
        TFT_DrawLine(32U, 11U, 36U, 11U, RED);
        TFT_DrawLine(33U, 12U, 35U, 12U, RED);
        TFT_DrawPoint(34U, 13U, RED);
        return;
    }

    TFT_DrawPoint(32U, 6U, RED);
    TFT_DrawPoint(36U, 6U, RED);
    TFT_DrawLine(31U, 7U, 37U, 7U, RED);
    TFT_DrawLine(32U, 8U, 36U, 8U, RED);
    TFT_DrawLine(33U, 9U, 35U, 9U, RED);
    TFT_DrawPoint(34U, 10U, RED);
}

void ECG_Init(void)
{
    active = 0U;
    output_bpm = ECG_OUTPUT_BPM_DEFAULT;
    display_span = ECG_DISPLAY_SPAN_MIN;
    ecg_reset_signal();
    ecg_clear_old_plot();
}

void ECG_Start(void)
{
    if (active != 0U)
    {
        return;
    }

    saved_pwm.period = get_pwm_period();
    saved_pwm.duty = get_pwm_duty();
    saved_pwm.state = get_pwm_state();
    ecg_reset_signal();
    set_pwm_period(ECG_PWM_PERIOD_TICKS);
    set_pwm_duty(ecg_pwm_duty_from_sample(0, ECG_PWM_PERIOD_TICKS));
    set_pwm_state(PWM_ON);
    active = 1U;
}

void ECG_Stop(void)
{
    if (active == 0U)
    {
        return;
    }

    active = 0U;
    set_pwm_state(PWM_OFF);
    set_pwm_period(saved_pwm.period);
    set_pwm_duty(saved_pwm.duty);
    set_pwm_state(saved_pwm.state);
}

void ECG_TimerTick1ms(void)
{
    if (active == 0U)
    {
        return;
    }

    if (heart_age_ms != 0xFFFFU)
    {
        heart_age_ms++;
    }
    if (bpm_age_ms != 0xFFFFU)
    {
        bpm_age_ms++;
        if (bpm_age_ms == (ECG_BPM_TIMEOUT_MS + 1U))
        {
            measured_bpm = 0U;
            bpm_timed_out = 1U;
            ecg_detector_init(&detector, ECG_SAMPLE_RATE_HZ, 600, 200, 250U);
            alarm_flags = ecg_alarm_evaluate(measured_bpm,
                                             (ecg_signal_quality_t)signal_quality,
                                             bpm_timed_out);
        }
    }
    sample_tick_ms++;
    if (sample_tick_ms >= ECG_SAMPLE_INTERVAL_MS)
    {
        sample_tick_ms = 0U;
        ecg_sample_tick();
    }
}

void ECG_StaticUI(void)
{
    TFT_Fill(0U, 0U, 160U, 128U, BLACK);
    ecg_clear_old_plot();
    TFT_ShowString(2U, 1U, (const uint8_t *)"ECG", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(42U, 1U, (const uint8_t *)"RUN", GREEN, BLACK, 16U, 0U);
    TFT_ShowString(72U, 1U, (const uint8_t *)"TB", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(108U, 1U, (const uint8_t *)"Q:?", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(108U, 18U, (const uint8_t *)"OUT", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(108U, 50U, (const uint8_t *)"OBPM", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(108U, 82U, (const uint8_t *)"BPM", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(2U, 108U, (const uint8_t *)"K1:OUT K2:BPM", GRAY, BLACK, 16U, 0U);

    ecg_draw_grid();
}

void ECG_ShowUI(void)
{
    char text[12];
    int16_t new_plot[ECG_PLOT_X1 - ECG_PLOT_X0 + 1U];
    uint16_t i;
    uint16_t needed;
    uint16_t offset;
    uint16_t index;
    const uint16_t width = (uint16_t)(sizeof(new_plot) / sizeof(new_plot[0]));
    uint16_t head_snapshot;
    uint16_t count_snapshot;
    uint8_t sequence_before;
    uint8_t sequence_after;
    const char *quality_text;
    const char *alarm_text;

    if (active == 0U)
    {
        return;
    }

    needed = (uint16_t)((uint32_t)display_span * ecg_display_period_samples());
    if (needed > ECG_BUFFER_SIZE)
    {
        needed = ECG_BUFFER_SIZE;
    }

    do
    {
        sequence_before = buffer_sequence;
        if ((sequence_before & 1U) != 0U)
        {
            sequence_after = sequence_before;
            continue;
        }
        head_snapshot = buffer_head;
        count_snapshot = buffer_count;
        for (i = 0U; i < width; ++i)
        {
            offset = (uint16_t)(((uint32_t)(width - 1U - i) * (needed - 1U)) /
                                (width - 1U));
            if (offset < count_snapshot)
            {
                index = (uint16_t)((head_snapshot + ECG_BUFFER_SIZE - 1U - offset) %
                                   ECG_BUFFER_SIZE);
                new_plot[i] = ecg_plot_y(sample_buffer[index]);
            }
            else
            {
                new_plot[i] = ECG_PLOT_CENTER_Y;
            }
        }
        sequence_after = buffer_sequence;
    } while ((sequence_before != sequence_after) || ((sequence_after & 1U) != 0U));

    for (i = 1U; i < width; ++i)
    {
        TFT_DrawLine((uint16_t)(ECG_PLOT_X0 + i - 1U), (uint16_t)old_plot[i - 1U],
                     (uint16_t)(ECG_PLOT_X0 + i), (uint16_t)old_plot[i], BLACK);
    }
    ecg_draw_grid();
    for (i = 1U; i < width; ++i)
    {
        TFT_DrawLine((uint16_t)(ECG_PLOT_X0 + i - 1U), (uint16_t)new_plot[i - 1U],
                     (uint16_t)(ECG_PLOT_X0 + i), (uint16_t)new_plot[i], GREEN);
        old_plot[i - 1U] = new_plot[i - 1U];
    }
    old_plot[width - 1U] = new_plot[width - 1U];

    sprintf(text, "x%u ", (unsigned int)display_span);
    TFT_ShowString(88U, 1U, (const uint8_t *)text, WHITE, BLACK, 16U, 0U);
    TFT_ShowString(108U, 34U,
                   (const uint8_t *)((get_pwm_state() == PWM_ON) ? "ON " : "OFF"),
                   GREEN, BLACK, 16U, 0U);
    sprintf(text, "%3u/m", (unsigned int)output_bpm);
    TFT_ShowString(108U, 66U, (const uint8_t *)text, CYAN, BLACK, 16U, 0U);
    if (measured_bpm == 0U)
    {
        strcpy(text, "---/m");
    }
    else
    {
        sprintf(text, "%3u/m", (unsigned int)measured_bpm);
    }
    TFT_ShowString(108U, 98U, (const uint8_t *)text, YELLOW, BLACK, 16U, 0U);
    if (signal_quality == ECG_SIGNAL_GOOD)
    {
        quality_text = "Q:G";
    }
    else if (signal_quality == ECG_SIGNAL_POOR)
    {
        quality_text = "Q:P";
    }
    else if (signal_quality == ECG_SIGNAL_LOST)
    {
        quality_text = "Q:L";
    }
    else
    {
        quality_text = "Q:?";
    }
    TFT_ShowString(108U, 1U, (const uint8_t *)quality_text,
                   (signal_quality == ECG_SIGNAL_GOOD) ? GREEN : YELLOW,
                   BLACK, 16U, 0U);
    if ((alarm_flags & ECG_ALARM_SIGNAL_LOST) != 0U)
    {
        alarm_text = "ALM:SIG ";
    }
    else if ((alarm_flags & ECG_ALARM_BRADY) != 0U)
    {
        alarm_text = "ALM:LOW ";
    }
    else if ((alarm_flags & ECG_ALARM_TACHY) != 0U)
    {
        alarm_text = "ALM:HIGH";
    }
    else
    {
        alarm_text = "K1:OUT K2:BPM";
    }
    TFT_ShowString(2U, 108U, (const uint8_t *)alarm_text,
                   (alarm_flags == ECG_ALARM_NONE) ? GRAY : RED,
                   BLACK, 16U, 0U);
    ecg_show_heart(ecg_heart_visible(heart_age_ms));
}

void ECG_KeyHandle(uint16_t key_pin, uint8_t key_state)
{
    if ((active == 0U) || (key_state == KEY_NoPress))
    {
        return;
    }

    if ((key_pin == KEY1_Pin) && (key_state == KeyPress))
    {
        set_pwm_state((get_pwm_state() == PWM_ON) ? PWM_OFF : PWM_ON);
    }
    else if (key_pin == KEY2_Pin)
    {
        if (key_state == KeyPress)
        {
            output_bpm = (uint16_t)(output_bpm + ECG_OUTPUT_BPM_STEP);
            if (output_bpm > ECG_OUTPUT_BPM_MAX)
            {
                output_bpm = ECG_OUTPUT_BPM_MIN;
            }
            phase_reset_requested = 1U;
        }
        else if (key_state == KeyDoublePress)
        {
            if (output_bpm <= ECG_OUTPUT_BPM_MIN)
            {
                output_bpm = ECG_OUTPUT_BPM_MAX;
            }
            else
            {
                output_bpm = (uint16_t)(output_bpm - ECG_OUTPUT_BPM_STEP);
            }
            phase_reset_requested = 1U;
        }
    }
}

void ECG_SetDisplaySpan(uint8_t periods)
{
    if (periods < ECG_DISPLAY_SPAN_MIN)
    {
        periods = ECG_DISPLAY_SPAN_MIN;
    }
    else if (periods > ECG_DISPLAY_SPAN_MAX)
    {
        periods = ECG_DISPLAY_SPAN_MAX;
    }
    display_span = periods;
}

uint8_t ECG_GetDisplaySpan(void)
{
    return display_span;
}

uint16_t ECG_GetBpm(void)
{
    return measured_bpm;
}

uint16_t ECG_GetOutputBpm(void)
{
    return output_bpm;
}

ecg_signal_quality_t ECG_GetSignalQuality(void)
{
    return (ecg_signal_quality_t)signal_quality;
}

uint8_t ECG_GetAlarmFlags(void)
{
    return alarm_flags;
}

uint8_t ECG_IsActive(void)
{
    return active;
}
