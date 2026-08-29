#include "ecg_acq_task.h"

#include "hw_key.h"
#include "mid_adc.h"
#include "mid_pwm.h"
#include "mid_timer.h"

#define ECG_ACQ_SAMPLE_INTERVAL_MS  4U
#define ECG_ACQ_BUFFER_SIZE      2500U
#define ECG_ACQ_MAX_PLOT_POINTS \
    (ECG_WAVE_PLOT_X1 - ECG_WAVE_PLOT_X0 + 1U)
#define ECG_PWM_PRESET_COUNT 5U

static const uint16_t pwm_frequency_presets[ECG_PWM_PRESET_COUNT] =
{
    100U, 250U, 500U, 1000U, 2000U
};

static ecg_acq_core_t acquisition_core;
static volatile int8_t sample_buffer[ECG_ACQ_BUFFER_SIZE];
static volatile uint16_t buffer_head;
static volatile uint16_t buffer_count;
static volatile uint8_t buffer_sequence;
static volatile uint32_t acquired_samples;
static volatile uint32_t event_sample;
static volatile uint8_t event_valid;
static volatile uint16_t measured_bpm;
static volatile uint16_t latest_rr_ms;
static volatile uint16_t rmssd_ms;
static volatile uint8_t signal_quality;
static uint8_t sample_tick_ms;
static uint8_t gain = 1U;
static uint8_t window_seconds = 5U;
static ecg_monitor_page_t current_page = ECG_MONITOR_PAGE;
static uint8_t pwm_preset_index = 3U;
static volatile uint8_t running;
static volatile uint8_t active;

static void ecg_acq_apply_pwm_preset(void)
{
    uint16_t period = (uint16_t)(PWM_TIMER_FREQ_HZ /
                      pwm_frequency_presets[pwm_preset_index]);

    set_pwm_period(period);
    set_pwm_duty((uint16_t)(period / 2U));
    if (get_pwm_state() == PWM_ON)
    {
        set_pwm_state(PWM_ON);
    }
}

static void ecg_acq_show_frequency_ui(void)
{
    ecg_monitor_view_t view;

    view.page = ECG_FREQ_PAGE;
    view.bpm = 0U;
    view.rr_ms = 0U;
    view.rmssd_ms = 0U;
    view.quality = ECG_SIGNAL_WAIT;
    view.running = running;
    view.gain = gain;
    view.window_seconds = window_seconds;
    view.event_marker_valid = 0U;
    view.event_marker_x = 0U;
    view.pwm_enabled = (get_pwm_state() == PWM_ON) ? 1U : 0U;
    view.pwm_target_hz = get_pwm_out_freq();
    view.pwm_measured_hz = get_freq_value();
    ECGMonitorUI_Render(&view, (const int16_t *)0, 0U);
}

static void ecg_acq_reset_measurement(void)
{
    uint16_t i;

    ECGAcqCore_Init(&acquisition_core);
    buffer_sequence++;
    for (i = 0U; i < ECG_ACQ_BUFFER_SIZE; ++i)
    {
        sample_buffer[i] = 0;
    }
    buffer_head = 0U;
    buffer_count = 0U;
    acquired_samples = 0U;
    event_sample = 0U;
    event_valid = 0U;
    measured_bpm = 0U;
    latest_rr_ms = 0U;
    rmssd_ms = 0U;
    signal_quality = ECG_SIGNAL_WAIT;
    sample_tick_ms = 0U;
    buffer_sequence++;
}

static int16_t ecg_acq_plot_y(int8_t packed_sample,
                              uint8_t gain_snapshot,
                              int16_t plot_y0,
                              int16_t plot_y1,
                              int16_t plot_center_y,
                              int16_t plot_half_height)
{
    int32_t scaled = (int32_t)packed_sample * gain_snapshot;
    int16_t y;

    if (scaled > 127)
    {
        scaled = 127;
    }
    else if (scaled < -128)
    {
        scaled = -128;
    }
    y = (int16_t)(plot_center_y -
                  ((scaled * plot_half_height) / 128));
    if (y < plot_y0)
    {
        y = plot_y0;
    }
    else if (y > plot_y1)
    {
        y = plot_y1;
    }
    return y;
}

static void ecg_acq_sample(void)
{
    ecg_acq_result_t result;
    uint16_t raw_sample;
    int16_t packed;

    raw_sample = Get_ADC_Latest();
    result = ECGAcqCore_Process(&acquisition_core, raw_sample);
    packed = ECGAcqCore_DisplaySample(raw_sample);

    buffer_sequence++;
    sample_buffer[buffer_head] = (int8_t)packed;
    buffer_head = (uint16_t)((buffer_head + 1U) % ECG_ACQ_BUFFER_SIZE);
    if (buffer_count < ECG_ACQ_BUFFER_SIZE)
    {
        buffer_count++;
    }
    acquired_samples++;
    measured_bpm = result.bpm;
    latest_rr_ms = result.rr_ms;
    rmssd_ms = result.rmssd_ms;
    signal_quality = (uint8_t)result.quality;
    buffer_sequence++;
}

void ECGAcq_Init(void)
{
    active = 0U;
    running = 0U;
    gain = 1U;
    window_seconds = 5U;
    current_page = ECG_MONITOR_PAGE;
    pwm_preset_index = 3U;
    set_pwm_state(PWM_OFF);
    ecg_acq_apply_pwm_preset();
    buffer_sequence = 0U;
    ecg_acq_reset_measurement();
}

void ECGAcq_Start(void)
{
    if (active != 0U)
    {
        return;
    }
    ecg_acq_reset_measurement();
    active = 1U;
    running = 1U;
}

void ECGAcq_Stop(void)
{
    active = 0U;
    running = 0U;
}

void ECGAcq_TimerTick1ms(void)
{
    if ((active == 0U) || (running == 0U))
    {
        return;
    }
    sample_tick_ms++;
    if (sample_tick_ms >= ECG_ACQ_SAMPLE_INTERVAL_MS)
    {
        sample_tick_ms = 0U;
        ecg_acq_sample();
    }
}

void ECGAcq_StaticUI(void)
{
    ECGMonitorUI_DrawStatic(current_page);
    ECGAcq_ShowUI();
}

void ECGAcq_ShowUI(void)
{
    ecg_monitor_view_t view;
    int16_t plot[ECG_ACQ_MAX_PLOT_POINTS];
    uint16_t i;
    uint16_t index;
    uint16_t offset;
    uint16_t head_snapshot;
    uint16_t count_snapshot;
    uint8_t sequence_before;
    uint8_t sequence_after;
    uint32_t samples_snapshot;
    uint32_t marker_snapshot;
    uint8_t marker_valid_snapshot;
    uint16_t plot_x0;
    uint16_t plot_x1;
    int16_t plot_y0;
    int16_t plot_y1;
    int16_t plot_center_y;
    int16_t plot_half_height;
    uint16_t width;
    uint16_t needed;

    if (active == 0U)
    {
        return;
    }
    if (current_page == ECG_FREQ_PAGE)
    {
        ecg_acq_show_frequency_ui();
        return;
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
        samples_snapshot = acquired_samples;
        marker_snapshot = event_sample;
        marker_valid_snapshot = event_valid;
        view.page = current_page;
        view.bpm = measured_bpm;
        view.rr_ms = latest_rr_ms;
        view.rmssd_ms = rmssd_ms;
        view.quality = (ecg_signal_quality_t)signal_quality;
        view.running = running;
        view.gain = gain;
        view.window_seconds = window_seconds;

        if (view.page == ECG_WAVE_PAGE)
        {
            plot_x0 = ECG_WAVE_PLOT_X0;
            plot_x1 = ECG_WAVE_PLOT_X1;
            plot_y0 = ECG_WAVE_PLOT_Y0;
            plot_y1 = ECG_WAVE_PLOT_Y1;
            plot_center_y = ECG_WAVE_PLOT_CENTER_Y;
            plot_half_height = ECG_WAVE_PLOT_HALF_HEIGHT;
        }
        else
        {
            plot_x0 = ECG_MONITOR_PLOT_X0;
            plot_x1 = ECG_MONITOR_PLOT_X1;
            plot_y0 = ECG_MONITOR_PLOT_Y0;
            plot_y1 = ECG_MONITOR_PLOT_Y1;
            plot_center_y = ECG_MONITOR_PLOT_CENTER_Y;
            plot_half_height = ECG_MONITOR_PLOT_HALF_HEIGHT;
        }
        width = (uint16_t)(plot_x1 - plot_x0 + 1U);
        needed = (uint16_t)((uint16_t)view.window_seconds *
                            ECG_ACQ_SAMPLE_RATE_HZ);
        for (i = 0U; i < width; ++i)
        {
            offset = (uint16_t)(((uint32_t)(width - 1U - i) *
                                (needed - 1U)) / (width - 1U));
            if (offset < count_snapshot)
            {
                index = (uint16_t)((head_snapshot + ECG_ACQ_BUFFER_SIZE - 1U -
                                    offset) % ECG_ACQ_BUFFER_SIZE);
                plot[i] = ecg_acq_plot_y(sample_buffer[index], view.gain,
                                         plot_y0, plot_y1, plot_center_y,
                                         plot_half_height);
            }
            else
            {
                plot[i] = plot_center_y;
            }
        }
        sequence_after = buffer_sequence;
    } while ((sequence_before != sequence_after) ||
             ((sequence_after & 1U) != 0U));

    view.event_marker_valid = 0U;
        view.event_marker_x = 0U;
        view.pwm_enabled = 0U;
        view.pwm_target_hz = 0U;
        view.pwm_measured_hz = 0U;
    if ((marker_valid_snapshot != 0U) &&
        (samples_snapshot >= marker_snapshot) &&
        ((samples_snapshot - marker_snapshot) < needed))
    {
        uint32_t age = samples_snapshot - marker_snapshot;
        view.event_marker_valid = 1U;
        view.event_marker_x = (uint16_t)(plot_x1 -
            ((age * (width - 1U)) / (needed - 1U)));
    }
    ECGMonitorUI_Render(&view, plot, width);
}

void ECGAcq_KeyHandle(uint16_t key_pin, uint8_t key_state)
{
    if ((active == 0U) || (key_state == KEY_NoPress))
    {
        return;
    }
    if ((current_page != ECG_FREQ_PAGE) &&
        (key_pin == KEY1_Pin) && (key_state == KeyPress))
    {
        running = (running == 0U) ? 1U : 0U;
        sample_tick_ms = 0U;
    }
    else if ((key_pin == KEY2_Pin) && (key_state == KeyDoublePress))
    {
        if (current_page == ECG_MONITOR_PAGE)
        {
            current_page = ECG_WAVE_PAGE;
        }
        else if (current_page == ECG_WAVE_PAGE)
        {
            current_page = ECG_FREQ_PAGE;
        }
        else
        {
            current_page = ECG_MONITOR_PAGE;
        }
        ECGMonitorUI_DrawStatic(current_page);
        ECGAcq_ShowUI();
    }
    else if ((current_page != ECG_FREQ_PAGE) &&
             (key_pin == KEY2_Pin) && (key_state == KeyPress))
    {
        gain = (gain == 1U) ? 2U : ((gain == 2U) ? 4U : 1U);
    }
    else if ((current_page != ECG_FREQ_PAGE) &&
             (key_pin == KEY3_Pin) &&
             ((key_state == KeyPress) ||
              (key_state == KeyDoublePress)))
    {
        uint8_t resume_sampling = running;

        running = 0U;
        if (key_state == KeyDoublePress)
        {
            ecg_acq_reset_measurement();
        }
        else if (key_state == KeyPress)
        {
            buffer_sequence++;
            event_sample = acquired_samples;
            event_valid = 1U;
            buffer_sequence++;
        }
        running = resume_sampling;
    }
    else if ((current_page == ECG_FREQ_PAGE) &&
             (key_pin == KEYD_Pin) && (key_state == KeyPress))
    {
        set_pwm_state((get_pwm_state() == PWM_ON) ? PWM_OFF : PWM_ON);
        if (current_page == ECG_FREQ_PAGE)
        {
            ECGAcq_ShowUI();
        }
    }
}

void ECGAcq_Rotate(int8_t direction)
{
    if ((active == 0U) || (direction == 0))
    {
        return;
    }
    if (current_page == ECG_FREQ_PAGE)
    {
        if (direction > 0)
        {
            pwm_preset_index = (uint8_t)((pwm_preset_index + 1U) %
                                         ECG_PWM_PRESET_COUNT);
        }
        else
        {
            pwm_preset_index = (pwm_preset_index == 0U) ?
                               (ECG_PWM_PRESET_COUNT - 1U) :
                               (uint8_t)(pwm_preset_index - 1U);
        }
        ecg_acq_apply_pwm_preset();
        ECGAcq_ShowUI();
        return;
    }
    if (direction > 0)
    {
        window_seconds = (window_seconds == 2U) ? 5U : 10U;
    }
    else
    {
        window_seconds = (window_seconds == 10U) ? 5U : 2U;
    }
}

uint8_t ECGAcq_IsActive(void)
{
    return active;
}

uint8_t ECGAcq_IsRunning(void)
{
    return running;
}

uint8_t ECGAcq_GetGain(void)
{
    return gain;
}

uint8_t ECGAcq_GetWindowSeconds(void)
{
    return window_seconds;
}

ecg_monitor_page_t ECGAcq_GetPage(void)
{
    return current_page;
}

uint16_t ECGAcq_GetBpm(void)
{
    return measured_bpm;
}

uint16_t ECGAcq_GetRrMs(void)
{
    return latest_rr_ms;
}

uint16_t ECGAcq_GetRmssdMs(void)
{
    return rmssd_ms;
}

ecg_signal_quality_t ECGAcq_GetQuality(void)
{
    return (ecg_signal_quality_t)signal_quality;
}
