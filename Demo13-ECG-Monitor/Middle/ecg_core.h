#ifndef __ECG_CORE_H
#define __ECG_CORE_H

#include <stdint.h>

#define ECG_RR_HISTORY_SIZE 4U

#define ECG_ALARM_NONE        0U
#define ECG_ALARM_BRADY       (1U << 0)
#define ECG_ALARM_TACHY       (1U << 1)
#define ECG_ALARM_SIGNAL_LOST (1U << 2)

typedef enum
{
    ECG_SIGNAL_UNKNOWN = 0,
    ECG_SIGNAL_GOOD,
    ECG_SIGNAL_POOR,
    ECG_SIGNAL_LOST
} ecg_signal_quality_t;

typedef struct
{
    uint16_t sample_rate_hz;
    uint16_t sample_count;
    uint16_t clipped_count;
    int16_t minimum;
    int16_t maximum;
    ecg_signal_quality_t quality;
} ecg_quality_monitor_t;

typedef struct
{
    uint16_t sample_rate_hz;
    int16_t threshold_high;
    int16_t threshold_low;
    uint16_t refractory_samples;
    uint32_t sample_index;
    uint32_t last_peak_index;
    uint32_t rr_sum;
    uint16_t rr_history[ECG_RR_HISTORY_SIZE];
    uint16_t bpm;
    uint8_t rr_count;
    uint8_t rr_index;
    uint8_t has_peak;
    uint8_t armed;
} ecg_detector_t;

typedef struct
{
    uint8_t r_peak;
    uint16_t bpm;
    uint16_t rr_ms;
} ecg_event_t;

void ecg_detector_init(ecg_detector_t *detector,
                       uint16_t sample_rate_hz,
                       int16_t threshold_high,
                       int16_t threshold_low,
                       uint16_t refractory_ms);
ecg_event_t ecg_detector_process(ecg_detector_t *detector, int16_t sample);
int16_t ecg_waveform_sample(uint16_t sample_in_period, uint16_t period_samples);
uint16_t ecg_pwm_duty_from_sample(int16_t sample, uint16_t period_ticks);
uint16_t ecg_display_stride(uint8_t span_periods,
                            uint16_t waveform_period_samples,
                            uint16_t pixel_width);
uint8_t ecg_heart_visible(uint16_t age_ms);
uint16_t ecg_samples_for_periods(uint8_t span_periods,
                                 uint16_t bpm,
                                 uint16_t sample_rate_hz);
void ecg_quality_init(ecg_quality_monitor_t *monitor, uint16_t sample_rate_hz);
ecg_signal_quality_t ecg_quality_process(ecg_quality_monitor_t *monitor,
                                         int16_t centered_sample,
                                         uint16_t adc_sample);
uint8_t ecg_alarm_evaluate(uint16_t bpm,
                           ecg_signal_quality_t quality,
                           uint8_t bpm_timed_out);

#endif
