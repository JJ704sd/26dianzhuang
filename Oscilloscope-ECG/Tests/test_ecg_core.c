#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ecg_core.h"

static unsigned int failures = 0U;

#define CHECK(condition)                                                        \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                       \
        {                                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            failures++;                                                         \
        }                                                                       \
    } while (0)

static void test_generated_wave_has_ecg_morphology(void)
{
    const uint16_t period = 250U;
    const int16_t baseline = ecg_waveform_sample(0U, period);
    const int16_t p = ecg_waveform_sample(30U, period);
    const int16_t q = ecg_waveform_sample(88U, period);
    const int16_t r = ecg_waveform_sample(100U, period);
    const int16_t s = ecg_waveform_sample(108U, period);
    const int16_t t = ecg_waveform_sample(163U, period);

    CHECK(p > baseline);
    CHECK(q < baseline);
    CHECK(r > p);
    CHECK(s < baseline);
    CHECK(t > p);
    CHECK(ecg_waveform_sample(period, period) == baseline);
}

static void test_pwm_mapping_is_centered_and_clamped(void)
{
    CHECK(ecg_pwm_duty_from_sample(0, 50U) == 25U);
    CHECK(ecg_pwm_duty_from_sample(1000, 50U) == 45U);
    CHECK(ecg_pwm_duty_from_sample(-1000, 50U) == 5U);
    CHECK(ecg_pwm_duty_from_sample(2000, 50U) == 45U);
    CHECK(ecg_pwm_duty_from_sample(-2000, 50U) == 5U);
}

static void test_detector_reports_bpm_after_two_valid_peaks(void)
{
    ecg_detector_t detector;
    ecg_event_t event = {0};
    uint16_t i;

    ecg_detector_init(&detector, 250U, 600, 200, 250U);
    event = ecg_detector_process(&detector, 1000);
    CHECK(event.r_peak == 1U);
    CHECK(event.bpm == 0U);

    for (i = 1U; i < 250U; ++i)
    {
        event = ecg_detector_process(&detector, 0);
        CHECK(event.r_peak == 0U);
    }

    event = ecg_detector_process(&detector, 1000);
    CHECK(event.r_peak == 1U);
    CHECK(event.rr_ms == 1000U);
    CHECK(event.bpm == 60U);
}

static void test_detector_rejects_refractory_double_peak(void)
{
    ecg_detector_t detector;
    uint16_t i;

    ecg_detector_init(&detector, 250U, 600, 200, 250U);
    CHECK(ecg_detector_process(&detector, 1000).r_peak == 1U);

    for (i = 1U; i < 62U; ++i)
    {
        (void)ecg_detector_process(&detector, 0);
    }
    CHECK(ecg_detector_process(&detector, 1000).r_peak == 0U);
    CHECK(ecg_detector_process(&detector, 1000).r_peak == 1U);
}

static void test_detector_averages_rr_without_truncating_samples(void)
{
    ecg_detector_t detector;
    ecg_event_t event;
    uint16_t i;

    ecg_detector_init(&detector, 250U, 600, 200, 0U);
    CHECK(ecg_detector_process(&detector, 1000).r_peak == 1U);

    for (i = 1U; i < 69U; ++i)
    {
        (void)ecg_detector_process(&detector, 0);
    }
    event = ecg_detector_process(&detector, 1000);
    CHECK(event.bpm == 217U);

    for (i = 1U; i < 70U; ++i)
    {
        (void)ecg_detector_process(&detector, 0);
    }
    event = ecg_detector_process(&detector, 1000);
    CHECK(event.bpm == 216U);
}

static void test_detector_survives_exact_sample_counter_wrap(void)
{
    ecg_detector_t detector;
    ecg_event_t event;

    ecg_detector_init(&detector, 250U, 600, 200, 0U);
    detector.has_peak = 1U;
    detector.armed = 1U;
    detector.last_peak_index = 7U;
    detector.sample_index = 7U;
    detector.bpm = 80U;

    event = ecg_detector_process(&detector, 1000);
    CHECK(event.r_peak == 1U);
    CHECK(event.bpm == 80U);
    CHECK(detector.rr_count == 0U);
}

static void test_display_span_and_heart_window(void)
{
    CHECK(ecg_display_stride(1U, 250U, 100U) == 3U);
    CHECK(ecg_display_stride(4U, 250U, 100U) == 10U);
    CHECK(ecg_display_stride(0U, 250U, 100U) == 3U);
    CHECK(ecg_heart_visible(0U) == 1U);
    CHECK(ecg_heart_visible(160U) == 1U);
    CHECK(ecg_heart_visible(161U) == 0U);
    CHECK(ecg_samples_for_periods(4U, 30U, 125U) == 1000U);
    CHECK(ecg_samples_for_periods(4U, 58U, 125U) == 517U);
}

static void test_degenerate_inputs_are_safe(void)
{
    ecg_detector_t detector;
    ecg_event_t event;

    ecg_detector_init(0, 250U, 600, 200, 250U);
    event = ecg_detector_process(0, 1000);
    CHECK(event.r_peak == 0U);
    CHECK(event.bpm == 0U);
    CHECK(event.rr_ms == 0U);

    ecg_detector_init(&detector, 0U, 600, 200, 250U);
    event = ecg_detector_process(&detector, 1000);
    CHECK(event.r_peak == 0U);
    CHECK(detector.sample_index == 0U);

    CHECK(ecg_waveform_sample(123U, 0U) == 0);
    CHECK(ecg_pwm_duty_from_sample(1000, 0U) == 0U);
    CHECK(ecg_display_stride(1U, 0U, 100U) == 1U);
    CHECK(ecg_display_stride(1U, 250U, 0U) == 1U);
    CHECK(ecg_samples_for_periods(4U, 0U, 125U) == 0U);
}

static void test_signal_quality_uses_fixed_one_second_windows(void)
{
    ecg_quality_monitor_t monitor;
    ecg_signal_quality_t quality = ECG_SIGNAL_UNKNOWN;
    uint16_t i;

    ecg_quality_init(&monitor, 250U);
    for (i = 0U; i < 249U; ++i)
    {
        quality = ecg_quality_process(&monitor, (i & 1U) ? 500 : -500,
                                      (i & 1U) ? 2548U : 1548U);
        CHECK(quality == ECG_SIGNAL_UNKNOWN);
    }
    quality = ecg_quality_process(&monitor, 500, 2548U);
    CHECK(quality == ECG_SIGNAL_GOOD);

    for (i = 0U; i < 250U; ++i)
    {
        quality = ecg_quality_process(&monitor, 4, 2052U);
    }
    CHECK(quality == ECG_SIGNAL_LOST);

    for (i = 0U; i < 250U; ++i)
    {
        quality = ecg_quality_process(&monitor, (i & 1U) ? 100 : -100,
                                      (i < 20U) ? 4095U : 2048U);
    }
    CHECK(quality == ECG_SIGNAL_POOR);
}

static void test_alarm_flags_require_usable_signal(void)
{
    CHECK(ecg_alarm_evaluate(45U, ECG_SIGNAL_GOOD, 0U) == ECG_ALARM_BRADY);
    CHECK(ecg_alarm_evaluate(130U, ECG_SIGNAL_GOOD, 0U) == ECG_ALARM_TACHY);
    CHECK(ecg_alarm_evaluate(80U, ECG_SIGNAL_POOR, 0U) == ECG_ALARM_NONE);
    CHECK(ecg_alarm_evaluate(80U, ECG_SIGNAL_LOST, 0U) == ECG_ALARM_SIGNAL_LOST);
    CHECK(ecg_alarm_evaluate(0U, ECG_SIGNAL_UNKNOWN, 1U) == ECG_ALARM_SIGNAL_LOST);
}

int main(void)
{
    test_generated_wave_has_ecg_morphology();
    test_pwm_mapping_is_centered_and_clamped();
    test_detector_reports_bpm_after_two_valid_peaks();
    test_detector_rejects_refractory_double_peak();
    test_detector_averages_rr_without_truncating_samples();
    test_detector_survives_exact_sample_counter_wrap();
    test_display_span_and_heart_window();
    test_degenerate_inputs_are_safe();
    test_signal_quality_uses_fixed_one_second_windows();
    test_alarm_flags_require_usable_signal();

    if (failures != 0U)
    {
        fprintf(stderr, "%u ECG core test(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("All ECG core tests passed.");
    return EXIT_SUCCESS;
}
