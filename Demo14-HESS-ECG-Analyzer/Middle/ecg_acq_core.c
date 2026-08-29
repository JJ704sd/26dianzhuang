#include "ecg_acq_core.h"

#include <stddef.h>

#define ECG_ADC_MAX_VALUE             4095U
#define ECG_RAIL_LOW_VALUE              16U
#define ECG_RAIL_HIGH_VALUE           4079U
#define ECG_REFRACTORY_MS              250U
#define ECG_MIN_QRS_THRESHOLD           70U
#define ECG_INITIAL_NOISE_LEVEL          20U
#define ECG_MIN_VALID_RR_MS            300U
#define ECG_MAX_VALID_RR_MS           2000U
#define ECG_BPM_TIMEOUT_MS            3000U

static void clear_hrv_measurements(ecg_acq_core_t *core)
{
    uint8_t i;

    core->last_peak_sample = 0U;
    core->rr_count = 0U;
    core->rr_write = 0U;
    core->bpm = 0U;
    core->latest_rr_ms = 0U;
    core->rmssd_ms = 0U;
    for (i = 0U; i < ECG_ACQ_RR_HISTORY_COUNT; ++i)
    {
        core->rr_ms[i] = 0U;
    }
}

static uint32_t integer_sqrt(uint32_t value)
{
    uint32_t root = 0U;
    uint32_t bit = 1UL << 30;

    while (bit > value)
    {
        bit >>= 2;
    }
    while (bit != 0U)
    {
        if (value >= root + bit)
        {
            value -= root + bit;
            root = (root >> 1) + bit;
        }
        else
        {
            root >>= 1;
        }
        bit >>= 2;
    }
    return root;
}

static void update_hrv(ecg_acq_core_t *core, uint16_t rr_ms)
{
    uint32_t rr_sum = 0U;
    uint32_t diff_square_sum = 0U;
    uint8_t i;
    uint8_t previous_index;
    uint8_t current_index;

    core->rr_ms[core->rr_write] = rr_ms;
    core->rr_write = (uint8_t)((core->rr_write + 1U) % ECG_ACQ_RR_HISTORY_COUNT);
    if (core->rr_count < ECG_ACQ_RR_HISTORY_COUNT)
    {
        core->rr_count++;
    }

    for (i = 0U; i < core->rr_count; ++i)
    {
        rr_sum += core->rr_ms[i];
    }
    if (rr_sum != 0U)
    {
        core->bpm = (uint16_t)((60000U * core->rr_count + (rr_sum / 2U)) / rr_sum);
    }

    if (core->rr_count < 2U)
    {
        core->rmssd_ms = 0U;
        return;
    }

    current_index = (uint8_t)((core->rr_write + ECG_ACQ_RR_HISTORY_COUNT - core->rr_count) %
                              ECG_ACQ_RR_HISTORY_COUNT);
    previous_index = current_index;
    current_index = (uint8_t)((current_index + 1U) % ECG_ACQ_RR_HISTORY_COUNT);
    for (i = 1U; i < core->rr_count; ++i)
    {
        int32_t difference = (int32_t)core->rr_ms[current_index] -
                             (int32_t)core->rr_ms[previous_index];
        diff_square_sum += (uint32_t)(difference * difference);
        previous_index = current_index;
        current_index = (uint8_t)((current_index + 1U) % ECG_ACQ_RR_HISTORY_COUNT);
    }
    core->rmssd_ms = (uint16_t)integer_sqrt(diff_square_sum / (core->rr_count - 1U));
}

static void update_quality(ecg_acq_core_t *core, uint16_t sample)
{
    uint16_t span;

    if (sample < core->window_min)
    {
        core->window_min = sample;
    }
    if (sample > core->window_max)
    {
        core->window_max = sample;
    }
    if ((sample <= ECG_RAIL_LOW_VALUE) || (sample >= ECG_RAIL_HIGH_VALUE))
    {
        core->rail_count++;
    }
    core->window_count++;

    if (core->window_count < ECG_ACQ_SAMPLE_RATE_HZ)
    {
        return;
    }

    span = (uint16_t)(core->window_max - core->window_min);
    if (core->rail_count >= 5U)
    {
        core->quality = ECG_SIGNAL_CLIPPED;
    }
    else if (span < 20U)
    {
        core->quality = ECG_SIGNAL_LEAD_OFF;
    }
    else if ((span < 70U) || (span > 3000U))
    {
        core->quality = ECG_SIGNAL_POOR;
    }
    else
    {
        core->quality = ECG_SIGNAL_GOOD;
    }

    core->window_min = ECG_ADC_MAX_VALUE;
    core->window_max = 0U;
    core->rail_count = 0U;
    core->window_count = 0U;
}

void ECGAcqCore_Init(ecg_acq_core_t *core)
{
    if (core == NULL)
    {
        return;
    }
    core->baseline_q8 = 0;
    core->smooth_q8 = 0;
    core->sample_count = 0U;
    core->refractory_samples = (ECG_REFRACTORY_MS * ECG_ACQ_SAMPLE_RATE_HZ) / 1000U;
    core->noise_q8 = ECG_INITIAL_NOISE_LEVEL << 8;
    clear_hrv_measurements(core);
    core->window_min = ECG_ADC_MAX_VALUE;
    core->window_max = 0U;
    core->rail_count = 0U;
    core->window_count = 0U;
    core->quality = ECG_SIGNAL_WAIT;
}

ecg_acq_result_t ECGAcqCore_Process(ecg_acq_core_t *core, uint16_t adc_sample)
{
    ecg_acq_result_t result = {0, 0U, 0U, 0U, 0U, ECG_SIGNAL_WAIT};
    int32_t centered_q8;
    uint32_t magnitude;
    uint32_t threshold;

    if (core == NULL)
    {
        return result;
    }
    if (adc_sample > ECG_ADC_MAX_VALUE)
    {
        adc_sample = ECG_ADC_MAX_VALUE;
    }

    if (core->sample_count == 0U)
    {
        core->baseline_q8 = (int32_t)adc_sample << 8;
    }
    core->baseline_q8 += (((int32_t)adc_sample << 8) - core->baseline_q8) >> 8;
    centered_q8 = ((int32_t)adc_sample << 8) - core->baseline_q8;
    core->smooth_q8 += (centered_q8 - core->smooth_q8) >> 2;
    result.filtered = (int16_t)(core->smooth_q8 >> 8);
    magnitude = (result.filtered < 0) ? (uint32_t)(-result.filtered) :
                                       (uint32_t)result.filtered;
    threshold = core->noise_q8 >> 8;
    threshold = threshold * 3U;
    if (threshold < ECG_MIN_QRS_THRESHOLD)
    {
        threshold = ECG_MIN_QRS_THRESHOLD;
    }

    if (magnitude < threshold)
    {
        core->noise_q8 += (((int32_t)magnitude << 8) - (int32_t)core->noise_q8) >> 6;
    }

    if ((magnitude >= threshold) &&
        ((core->sample_count - core->last_peak_sample) >= core->refractory_samples))
    {
        uint32_t rr_samples = core->sample_count - core->last_peak_sample;
        uint16_t rr_ms = (uint16_t)((rr_samples * 1000U) / ECG_ACQ_SAMPLE_RATE_HZ);

        result.r_peak = 1U;
        if ((core->last_peak_sample != 0U) &&
            (rr_ms >= ECG_MIN_VALID_RR_MS) && (rr_ms <= ECG_MAX_VALID_RR_MS))
        {
            core->latest_rr_ms = rr_ms;
            update_hrv(core, rr_ms);
        }
        core->last_peak_sample = core->sample_count;
    }

    if ((core->last_peak_sample != 0U) &&
        ((core->sample_count - core->last_peak_sample) >
         ((ECG_BPM_TIMEOUT_MS * ECG_ACQ_SAMPLE_RATE_HZ) / 1000U)))
    {
        clear_hrv_measurements(core);
    }

    update_quality(core, adc_sample);
    core->sample_count++;
    result.bpm = core->bpm;
    result.rr_ms = core->latest_rr_ms;
    result.rmssd_ms = core->rmssd_ms;
    result.quality = core->quality;
    return result;
}

int8_t ECGAcqCore_DisplaySample(uint16_t adc_sample)
{
    int32_t centered;

    if (adc_sample > ECG_ADC_MAX_VALUE)
    {
        adc_sample = ECG_ADC_MAX_VALUE;
    }
    centered = (int32_t)adc_sample - 2048;
    centered /= 16;
    if (centered > 127)
    {
        centered = 127;
    }
    else if (centered < -128)
    {
        centered = -128;
    }
    return (int8_t)centered;
}

const char *ECGAcqCore_QualityText(ecg_signal_quality_t quality)
{
    switch (quality)
    {
        case ECG_SIGNAL_GOOD: return "GOOD";
        case ECG_SIGNAL_POOR: return "POOR";
        case ECG_SIGNAL_LEAD_OFF: return "LEAD OFF";
        case ECG_SIGNAL_CLIPPED: return "CLIPPED";
        default: return "WAIT";
    }
}
