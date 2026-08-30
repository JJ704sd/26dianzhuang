#include "ecg_acq_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                        \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                       \
        {                                                                       \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__,          \
                    #condition);                                                \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    } while (0)

static ecg_acq_result_t process_window(ecg_acq_core_t *core,
                                       uint16_t low,
                                       uint16_t high)
{
    ecg_acq_result_t result = {0};
    uint16_t i;

    for (i = 0U; i < ECG_ACQ_SAMPLE_RATE_HZ; ++i)
    {
        result = ECGAcqCore_Process(core, (i & 1U) ? high : low);
    }
    return result;
}

static void test_init_and_quality_labels(void)
{
    ecg_acq_core_t core;

    ECGAcqCore_Init(&core);
    CHECK(core.sample_count == 0U);
    CHECK(core.bpm == 0U);
    CHECK(core.latest_rr_ms == 0U);
    CHECK(core.rmssd_ms == 0U);
    CHECK(core.quality == ECG_SIGNAL_WAIT);
    CHECK(core.refractory_samples == 62U);
    CHECK(strcmp(ECGAcqCore_QualityText(ECG_SIGNAL_WAIT), "WAIT") == 0);
    CHECK(strcmp(ECGAcqCore_QualityText(ECG_SIGNAL_GOOD), "GOOD") == 0);
    CHECK(strcmp(ECGAcqCore_QualityText(ECG_SIGNAL_POOR), "POOR") == 0);
    CHECK(strcmp(ECGAcqCore_QualityText(ECG_SIGNAL_LEAD_OFF), "LEAD OFF") == 0);
    CHECK(strcmp(ECGAcqCore_QualityText(ECG_SIGNAL_CLIPPED), "CLIPPED") == 0);

    ECGAcqCore_Init(NULL);
    CHECK(ECGAcqCore_Process(NULL, 2048U).quality == ECG_SIGNAL_WAIT);
}

static void test_quality_uses_one_second_windows(void)
{
    ecg_acq_core_t core;
    ecg_acq_result_t result;

    ECGAcqCore_Init(&core);
    result = process_window(&core, 2048U, 2048U);
    CHECK(result.quality == ECG_SIGNAL_LEAD_OFF);

    ECGAcqCore_Init(&core);
    result = process_window(&core, 2000U, 2200U);
    CHECK(result.quality == ECG_SIGNAL_GOOD);

    ECGAcqCore_Init(&core);
    result = process_window(&core, 2048U, 2080U);
    CHECK(result.quality == ECG_SIGNAL_POOR);

    ECGAcqCore_Init(&core);
    result = process_window(&core, 0U, 4095U);
    CHECK(result.quality == ECG_SIGNAL_CLIPPED);
}

static void test_adc_input_is_clamped_before_quality_analysis(void)
{
    ecg_acq_core_t core;
    ecg_acq_result_t result = {0};
    uint16_t i;

    ECGAcqCore_Init(&core);
    for (i = 0U; i < ECG_ACQ_SAMPLE_RATE_HZ; ++i)
    {
        result = ECGAcqCore_Process(&core, 5000U);
    }
    CHECK(result.quality == ECG_SIGNAL_CLIPPED);
}

static void test_regular_qrs_pulses_produce_bpm_and_rr(void)
{
    ecg_acq_core_t core;
    ecg_acq_result_t result = {0};
    uint16_t sample;
    uint8_t peak_count = 0U;

    ECGAcqCore_Init(&core);
    for (sample = 0U; sample < 1100U; ++sample)
    {
        uint16_t adc = 2048U;
        if ((sample == 300U) || (sample == 550U) ||
            (sample == 800U) || (sample == 1050U))
        {
            adc = 3200U;
        }
        result = ECGAcqCore_Process(&core, adc);
        if (result.r_peak != 0U)
        {
            peak_count++;
        }
    }

    CHECK(peak_count == 4U);
    CHECK(result.bpm == 60U);
    CHECK(result.rr_ms == 1000U);
    CHECK(result.rmssd_ms == 0U);
}

static void test_variable_rr_intervals_produce_rmssd(void)
{
    static const uint16_t peak_samples[] = {300U, 550U, 750U, 1000U};
    ecg_acq_core_t core;
    ecg_acq_result_t result = {0};
    uint16_t sample;
    uint8_t next_peak = 0U;

    ECGAcqCore_Init(&core);
    for (sample = 0U; sample < 1050U; ++sample)
    {
        uint16_t adc = 2048U;
        if ((next_peak < (uint8_t)(sizeof(peak_samples) /
                                   sizeof(peak_samples[0]))) &&
            (sample == peak_samples[next_peak]))
        {
            adc = 3200U;
            next_peak++;
        }
        result = ECGAcqCore_Process(&core, adc);
    }

    CHECK(result.bpm == 64U);
    CHECK(result.rr_ms == 1000U);
    CHECK((result.rmssd_ms >= 199U) && (result.rmssd_ms <= 201U));
}

static void test_stale_heart_rate_becomes_unavailable(void)
{
    ecg_acq_core_t core;
    ecg_acq_result_t result = {0};
    uint16_t sample;

    ECGAcqCore_Init(&core);
    for (sample = 0U; sample < 850U; ++sample)
    {
        uint16_t adc = 2048U;
        if ((sample == 300U) || (sample == 550U) || (sample == 800U))
        {
            adc = 3200U;
        }
        result = ECGAcqCore_Process(&core, adc);
    }
    CHECK(result.bpm == 60U);

    for (sample = 0U; sample <= (3U * ECG_ACQ_SAMPLE_RATE_HZ); ++sample)
    {
        result = ECGAcqCore_Process(&core, 2048U);
    }
    CHECK(result.bpm == 0U);
    CHECK(result.rr_ms == 0U);
    CHECK(result.rmssd_ms == 0U);
}

static void test_display_sample_preserves_square_wave_levels(void)
{
    uint16_t i;

    for (i = 0U; i < ECG_ACQ_SAMPLE_RATE_HZ; ++i)
    {
        CHECK(ECGAcqCore_DisplaySample(1228U) == -51);
        CHECK(ECGAcqCore_DisplaySample(2868U) == 51);
    }
    CHECK(ECGAcqCore_DisplaySample(0U) == -128);
    CHECK(ECGAcqCore_DisplaySample(4095U) == 127);
    CHECK(ECGAcqCore_DisplaySample(5000U) == 127);
}

static void test_high_rate_waveform_mapping_preserves_shape_and_margin(void)
{
    int8_t square_samples[200];
    int8_t small_sine_samples[16] =
        {0, 7, 10, 7, 0, -7, -10, -7, 0, 7, 10, 7, 0, -7, -10, -7};
    int16_t plot[156];
    int16_t small_x1[16];
    int16_t small_x4[16];
    int8_t full_range[16];
    int16_t full_plot[16];
    int16_t min_y = 127;
    int16_t max_y = 0;
    uint16_t transitions = 0U;
    uint16_t i;
    uint8_t fit_limited;

    CHECK(ECG_DISPLAY_SAMPLE_RATE_HZ >= 40000U);
    CHECK(ECG_DISPLAY_WAVE_WINDOW_SAMPLES == 200U);
    for (i = 0U; i < 200U; ++i)
    {
        square_samples[i] = (((i + 10U) % 20U) < 10U) ? 77 : -77;
    }
    fit_limited = ECGAcqCore_MapDisplaySamples(
        square_samples, 200U, 4U, 1U, 17, 109, 63, 44, plot, 156U);
    CHECK(fit_limited == 1U);
    CHECK(plot[0] < 63);
    for (i = 0U; i < 156U; ++i)
    {
        CHECK(plot[i] >= 19);
        CHECK(plot[i] <= 107);
        if (plot[i] < min_y) { min_y = plot[i]; }
        if (plot[i] > max_y) { max_y = plot[i]; }
        if ((i > 0U) && (plot[i] != plot[i - 1U])) { transitions++; }
    }
    CHECK((max_y - min_y) >= 80);
    CHECK(transitions >= 18U);

    ECGAcqCore_MapDisplaySamples(small_sine_samples, 16U, 1U, 0U,
                                 17, 109, 63, 44, small_x1, 16U);
    ECGAcqCore_MapDisplaySamples(small_sine_samples, 16U, 4U, 0U,
                                 17, 109, 63, 44, small_x4, 16U);
    CHECK((small_x4[6] - small_x4[2]) >
          (small_x1[6] - small_x1[2]));
    for (i = 0U; i < 16U; ++i)
    {
        CHECK(small_x4[i] >= 19);
        CHECK(small_x4[i] <= 107);
        full_range[i] = ((i & 1U) == 0U) ? -128 : 127;
    }
    fit_limited = ECGAcqCore_MapDisplaySamples(
        full_range, 16U, 4U, 0U, 17, 109, 63, 44, full_plot, 16U);
    CHECK(fit_limited == 1U);
    for (i = 0U; i < 16U; ++i)
    {
        CHECK(full_plot[i] >= 19);
        CHECK(full_plot[i] <= 107);
    }
    CHECK((full_plot[0] - full_plot[1]) >= 80);
}

int main(void)
{
    test_init_and_quality_labels();
    test_quality_uses_one_second_windows();
    test_adc_input_is_clamped_before_quality_analysis();
    test_regular_qrs_pulses_produce_bpm_and_rr();
    test_variable_rr_intervals_produce_rmssd();
    test_stale_heart_rate_becomes_unavailable();
    test_display_sample_preserves_square_wave_levels();
    test_high_rate_waveform_mapping_preserves_shape_and_margin();
    puts("ecg_acq_core tests passed");
    return EXIT_SUCCESS;
}
