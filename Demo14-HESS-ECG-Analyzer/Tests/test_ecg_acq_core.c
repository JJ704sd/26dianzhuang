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

int main(void)
{
    test_init_and_quality_labels();
    test_quality_uses_one_second_windows();
    test_adc_input_is_clamped_before_quality_analysis();
    test_regular_qrs_pulses_produce_bpm_and_rr();
    test_variable_rr_intervals_produce_rmssd();
    test_stale_heart_rate_becomes_unavailable();
    test_display_sample_preserves_square_wave_levels();
    puts("ecg_acq_core tests passed");
    return EXIT_SUCCESS;
}
