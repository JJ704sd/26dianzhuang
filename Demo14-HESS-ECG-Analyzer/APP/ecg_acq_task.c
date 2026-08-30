#include "ecg_acq_task.h"

#include "mid_adc.h"
#include "mid_timer.h"

#define ECG_ACQ_SAMPLE_INTERVAL_MS  4U
#define ECG_ACQ_BUFFER_SIZE      1250U
#define ECG_ACQ_MAX_PLOT_POINTS \
    (ECG_WAVE_PLOT_X1 - ECG_WAVE_PLOT_X0 + 1U)

typedef struct
{
    uint16_t x0;
    uint16_t x1;
    int16_t y0;
    int16_t y1;
    int16_t center_y;
    int16_t half_height;
} ecg_plot_geometry_t;

static ecg_acq_core_t acquisition_core;
static volatile int8_t sample_buffer[ECG_ACQ_BUFFER_SIZE];
static volatile uint16_t buffer_head;
static volatile uint16_t buffer_count;
static volatile uint32_t buffer_sequence;
static volatile uint32_t acquired_samples;
static volatile uint32_t event_sample;
static volatile uint8_t event_valid;
static volatile uint16_t measured_bpm;
static volatile uint16_t latest_rr_ms;
static volatile uint16_t rmssd_ms;
static volatile uint8_t signal_quality;
static volatile uint8_t sample_tick_ms;
static uint8_t gain = 1U;
static uint8_t window_seconds = 2U;
static ecg_monitor_page_t current_page = ECG_MONITOR_PAGE;
static volatile uint8_t running;
static volatile uint8_t active;

static ecg_plot_geometry_t ecg_acq_plot_geometry(ecg_monitor_page_t page)
{
    ecg_plot_geometry_t geometry;

    if (page == ECG_WAVE_PAGE)
    {
        geometry.x0 = ECG_WAVE_PLOT_X0;
        geometry.x1 = ECG_WAVE_PLOT_X1;
        geometry.y0 = ECG_WAVE_PLOT_Y0;
        geometry.y1 = ECG_WAVE_PLOT_Y1;
        geometry.center_y = ECG_WAVE_PLOT_CENTER_Y;
        geometry.half_height = ECG_WAVE_PLOT_HALF_HEIGHT;
    }
    else
    {
        geometry.x0 = ECG_MONITOR_PLOT_X0;
        geometry.x1 = ECG_MONITOR_PLOT_X1;
        geometry.y0 = ECG_MONITOR_PLOT_Y0;
        geometry.y1 = ECG_MONITOR_PLOT_Y1;
        geometry.center_y = ECG_MONITOR_PLOT_CENTER_Y;
        geometry.half_height = ECG_MONITOR_PLOT_HALF_HEIGHT;
    }
    return geometry;
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
    window_seconds = 2U;
    current_page = ECG_MONITOR_PAGE;
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
    ecg_plot_geometry_t geometry;
    int16_t plot[ECG_ACQ_MAX_PLOT_POINTS];
    int8_t display_samples[ECG_ACQ_MAX_PLOT_POINTS];
    uint16_t i;
    uint16_t index;
    uint16_t offset;
    uint16_t head_snapshot;
    uint16_t count_snapshot;
    uint32_t sequence_before;
    uint32_t sequence_after;
    uint32_t samples_snapshot;
    uint32_t marker_snapshot;
    uint8_t marker_valid_snapshot;
    uint16_t width;
    uint16_t needed;

    if (active == 0U)
    {
        return;
    }
    for (;;)
    {
        sequence_before = buffer_sequence;
        if ((sequence_before & 1U) != 0U)
        {
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
        view.fit_limited = 0U;
        view.waveform_revision = samples_snapshot;
        geometry = ecg_acq_plot_geometry(view.page);
        width = (uint16_t)(geometry.x1 - geometry.x0 + 1U);
        needed = (uint16_t)((uint16_t)view.window_seconds *
                            ECG_ACQ_SAMPLE_RATE_HZ);
        for (i = 0U; i < width; ++i)
        {
            offset = (uint16_t)(((uint32_t)(width - 1U - i) *
                                (needed - 1U)) / (width - 1U));
            if (offset < count_snapshot)
            {
                index = (uint16_t)((head_snapshot + ECG_ACQ_BUFFER_SIZE -
                                    1U - offset) % ECG_ACQ_BUFFER_SIZE);
                display_samples[i] = sample_buffer[index];
            }
            else
            {
                display_samples[i] = 0;
            }
        }
        sequence_after = buffer_sequence;
        if ((sequence_before == sequence_after) &&
            ((sequence_after & 1U) == 0U))
        {
            break;
        }
    }

    view.fit_limited = ECGAcqCore_MapDisplaySamples(
        display_samples, width, view.gain, 0U,
        geometry.y0, geometry.y1, geometry.center_y, geometry.half_height,
        plot, width);

    view.event_marker_valid = 0U;
    view.event_marker_x = 0U;
    if ((marker_valid_snapshot != 0U) &&
        (samples_snapshot >= marker_snapshot) &&
        ((samples_snapshot - marker_snapshot) < needed))
    {
        uint32_t age = samples_snapshot - marker_snapshot;
        view.event_marker_valid = 1U;
        view.event_marker_x = (uint16_t)(geometry.x1 -
            ((age * (width - 1U)) / (needed - 1U)));
    }
    ECGMonitorUI_Render(&view, plot, width);
}

void ECGAcq_HandleAction(ecg_acq_action_t action)
{
    if (active == 0U)
    {
        return;
    }

    switch (action)
    {
        case ECG_ACQ_ACTION_TOGGLE_RUN:
            running = (running == 0U) ? 1U : 0U;
            sample_tick_ms = 0U;
            break;
        case ECG_ACQ_ACTION_CYCLE_GAIN:
            gain = (gain == 1U) ? 2U : ((gain == 2U) ? 4U : 1U);
            break;
        case ECG_ACQ_ACTION_TOGGLE_PAGE:
            current_page = (current_page == ECG_MONITOR_PAGE) ?
                           ECG_WAVE_PAGE : ECG_MONITOR_PAGE;
            ECGMonitorUI_DrawStatic(current_page);
            ECGAcq_ShowUI();
            break;
        case ECG_ACQ_ACTION_MARK_EVENT:
        {
            uint8_t resume_sampling = running;
            running = 0U;
            buffer_sequence++;
            event_sample = acquired_samples;
            event_valid = 1U;
            buffer_sequence++;
            running = resume_sampling;
            break;
        }
        case ECG_ACQ_ACTION_RESET_MEASUREMENTS:
        {
            uint8_t resume_sampling = running;
            running = 0U;
            ecg_acq_reset_measurement();
            running = resume_sampling;
            break;
        }
        case ECG_ACQ_ACTION_WINDOW_2_SECONDS:
            window_seconds = ECG_ACQ_WINDOW_MIN_SECONDS;
            break;
        case ECG_ACQ_ACTION_WINDOW_5_SECONDS:
            window_seconds = ECG_ACQ_WINDOW_MAX_SECONDS;
            break;
        default:
            break;
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
