#include "ecg_core.h"

#include <limits.h>
#include <string.h>

typedef struct
{
    uint16_t phase;
    int16_t value;
} ecg_anchor_t;

static const ecg_anchor_t ecg_template[] = {
    {0U, 0},
    {80U, 0},
    {120U, 120},
    {180U, 0},
    {350U, -180},
    {400U, 1000},
    {430U, -350},
    {500U, 0},
    {650U, 300},
    {800U, 0},
    {1000U, 0}
};

void ecg_detector_init(ecg_detector_t *detector,
                       uint16_t sample_rate_hz,
                       int16_t threshold_high,
                       int16_t threshold_low,
                       uint16_t refractory_ms)
{
    uint32_t refractory_samples;

    if (detector == 0)
    {
        return;
    }

    memset(detector, 0, sizeof(*detector));
    detector->sample_rate_hz = sample_rate_hz;
    detector->threshold_high = threshold_high;
    detector->threshold_low = threshold_low;
    refractory_samples = ((uint32_t)sample_rate_hz * refractory_ms + 999U) / 1000U;
    if (refractory_samples > UINT16_MAX)
    {
        refractory_samples = UINT16_MAX;
    }
    detector->refractory_samples = (uint16_t)refractory_samples;
    detector->armed = 1U;
}

ecg_event_t ecg_detector_process(ecg_detector_t *detector, int16_t sample)
{
    ecg_event_t event = {0U, 0U, 0U};
    uint32_t rr_samples;
    uint32_t bpm;

    if ((detector == 0) || (detector->sample_rate_hz == 0U))
    {
        return event;
    }

    if (sample <= detector->threshold_low)
    {
        detector->armed = 1U;
    }

    if ((detector->armed != 0U) && (sample >= detector->threshold_high) &&
        ((detector->has_peak == 0U) ||
         ((detector->sample_index - detector->last_peak_index) >= detector->refractory_samples)))
    {
        event.r_peak = 1U;
        detector->armed = 0U;

        if (detector->has_peak != 0U)
        {
            rr_samples = detector->sample_index - detector->last_peak_index;
            if (rr_samples != 0U)
            {
                bpm = ((uint32_t)60U * detector->sample_rate_hz + (rr_samples / 2U)) /
                      rr_samples;
                if ((bpm >= 30U) && (bpm <= 220U) && (rr_samples <= UINT16_MAX))
                {
                    if (detector->rr_count == ECG_RR_HISTORY_SIZE)
                    {
                        detector->rr_sum -= detector->rr_history[detector->rr_index];
                    }
                    else
                    {
                        detector->rr_count++;
                    }

                    detector->rr_history[detector->rr_index] = (uint16_t)rr_samples;
                    detector->rr_sum += rr_samples;
                    detector->rr_index =
                        (uint8_t)((detector->rr_index + 1U) % ECG_RR_HISTORY_SIZE);
                    detector->bpm =
                        (uint16_t)(((uint32_t)60U * detector->sample_rate_hz *
                                    detector->rr_count + (detector->rr_sum / 2U)) /
                                   detector->rr_sum);
                    event.rr_ms = (uint16_t)((rr_samples * 1000U +
                                              (detector->sample_rate_hz / 2U)) /
                                             detector->sample_rate_hz);
                }
            }
        }

        detector->last_peak_index = detector->sample_index;
        detector->has_peak = 1U;
    }

    event.bpm = detector->bpm;
    detector->sample_index++;
    return event;
}

int16_t ecg_waveform_sample(uint16_t sample_in_period, uint16_t period_samples)
{
    uint16_t phase;
    uint16_t i;

    if (period_samples == 0U)
    {
        return 0;
    }

    phase = (uint16_t)(((uint32_t)(sample_in_period % period_samples) * 1000U) /
                       period_samples);
    for (i = 1U; i < (uint16_t)(sizeof(ecg_template) / sizeof(ecg_template[0])); ++i)
    {
        if (phase <= ecg_template[i].phase)
        {
            const int32_t phase_offset = (int32_t)phase - ecg_template[i - 1U].phase;
            const int32_t phase_width = (int32_t)ecg_template[i].phase -
                                        ecg_template[i - 1U].phase;
            const int32_t value_delta = (int32_t)ecg_template[i].value -
                                        ecg_template[i - 1U].value;
            return (int16_t)((int32_t)ecg_template[i - 1U].value +
                             ((value_delta * phase_offset) / phase_width));
        }
    }
    return 0;
}

uint16_t ecg_pwm_duty_from_sample(int16_t sample, uint16_t period_ticks)
{
    uint32_t lower;
    uint32_t upper;
    uint32_t duty;

    if (period_ticks == 0U)
    {
        return 0U;
    }
    if (sample > 1000)
    {
        sample = 1000;
    }
    else if (sample < -1000)
    {
        sample = -1000;
    }

    lower = period_ticks / 10U;
    upper = period_ticks - lower;
    duty = lower + (((uint32_t)(sample + 1000) * (upper - lower)) / 2000U);
    return (uint16_t)duty;
}

uint16_t ecg_display_stride(uint8_t span_periods,
                            uint16_t waveform_period_samples,
                            uint16_t pixel_width)
{
    uint32_t total_samples;
    uint32_t stride;

    if (span_periods == 0U)
    {
        span_periods = 1U;
    }
    if ((waveform_period_samples == 0U) || (pixel_width == 0U))
    {
        return 1U;
    }

    total_samples = (uint32_t)span_periods * waveform_period_samples;
    stride = (total_samples + pixel_width - 1U) / pixel_width;
    if (stride == 0U)
    {
        stride = 1U;
    }
    if (stride > UINT16_MAX)
    {
        stride = UINT16_MAX;
    }
    return (uint16_t)stride;
}

uint8_t ecg_heart_visible(uint16_t age_ms)
{
    return (age_ms <= 160U) ? 1U : 0U;
}

uint16_t ecg_samples_for_periods(uint8_t span_periods,
                                 uint16_t bpm,
                                 uint16_t sample_rate_hz)
{
    uint32_t samples;

    if ((span_periods == 0U) || (bpm == 0U) || (sample_rate_hz == 0U))
    {
        return 0U;
    }
    samples = ((uint32_t)span_periods * 60U * sample_rate_hz + (bpm / 2U)) / bpm;
    if (samples > UINT16_MAX)
    {
        samples = UINT16_MAX;
    }
    return (uint16_t)samples;
}

void ecg_quality_init(ecg_quality_monitor_t *monitor, uint16_t sample_rate_hz)
{
    if (monitor == 0)
    {
        return;
    }
    memset(monitor, 0, sizeof(*monitor));
    monitor->sample_rate_hz = sample_rate_hz;
    monitor->minimum = INT16_MAX;
    monitor->maximum = INT16_MIN;
    monitor->quality = ECG_SIGNAL_UNKNOWN;
}

ecg_signal_quality_t ecg_quality_process(ecg_quality_monitor_t *monitor,
                                         int16_t centered_sample,
                                         uint16_t adc_sample)
{
    uint16_t span;

    if ((monitor == 0) || (monitor->sample_rate_hz == 0U))
    {
        return ECG_SIGNAL_UNKNOWN;
    }

    if (centered_sample < monitor->minimum)
    {
        monitor->minimum = centered_sample;
    }
    if (centered_sample > monitor->maximum)
    {
        monitor->maximum = centered_sample;
    }
    if ((adc_sample <= 8U) || (adc_sample >= 4087U))
    {
        monitor->clipped_count++;
    }
    monitor->sample_count++;

    if (monitor->sample_count < monitor->sample_rate_hz)
    {
        return monitor->quality;
    }

    span = (uint16_t)((int32_t)monitor->maximum - monitor->minimum);
    if (span < 80U)
    {
        monitor->quality = ECG_SIGNAL_LOST;
    }
    else if ((span < 300U) ||
             (((uint32_t)monitor->clipped_count * 20U) >= monitor->sample_count))
    {
        monitor->quality = ECG_SIGNAL_POOR;
    }
    else
    {
        monitor->quality = ECG_SIGNAL_GOOD;
    }

    monitor->sample_count = 0U;
    monitor->clipped_count = 0U;
    monitor->minimum = INT16_MAX;
    monitor->maximum = INT16_MIN;
    return monitor->quality;
}

uint8_t ecg_alarm_evaluate(uint16_t bpm,
                           ecg_signal_quality_t quality,
                           uint8_t bpm_timed_out)
{
    if ((quality == ECG_SIGNAL_LOST) || (bpm_timed_out != 0U))
    {
        return ECG_ALARM_SIGNAL_LOST;
    }
    if (quality != ECG_SIGNAL_GOOD)
    {
        return ECG_ALARM_NONE;
    }
    if ((bpm != 0U) && (bpm < 50U))
    {
        return ECG_ALARM_BRADY;
    }
    if (bpm > 120U)
    {
        return ECG_ALARM_TACHY;
    }
    return ECG_ALARM_NONE;
}
