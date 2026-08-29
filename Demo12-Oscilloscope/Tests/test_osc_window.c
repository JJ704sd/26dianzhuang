#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "osc_window.h"

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void test_exact_width_uses_only_complete_window(void)
{
    int16_t samples[100] = {0};
    uint16_t start = 99U;

    samples[20] = 10U;
    samples[22] = 30U;

    require(osc_window_find(samples, 100U, 20U, 100U, &start) != 0U,
            "a buffer exactly one window wide should be usable");
    require(start == 0U,
            "a buffer exactly one window wide must start at zero");
}

static void test_normal_trigger_becomes_window_start(void)
{
    int16_t samples[1000] = {0};
    uint16_t start = 0U;

    samples[200] = 10U;
    samples[202] = 30U;

    require(osc_window_find(samples, 1000U, 20U, 100U, &start) != 0U,
            "a complete 100-sample window should be returned");
    require(start == 200U,
            "a safe rising-edge trigger should remain the window start");
}

static void test_tail_trigger_is_clamped_to_complete_window(void)
{
    int16_t samples[1000] = {0};
    uint16_t start = 0U;

    samples[996] = 10U;
    samples[998] = 30U;

    require(osc_window_find(samples, 1000U, 20U, 100U, &start) != 0U,
            "a tail trigger should still yield a complete window");
    require(start == 900U,
            "a tail trigger must clamp to count minus width");
    require((uint32_t)start + 100U <= 1000U,
            "the selected tail window must remain inside the buffer");
}

static void test_short_buffer_is_rejected(void)
{
    int16_t samples[99] = {0};
    uint16_t start = 77U;

    require(osc_window_find(samples, 99U, 20U, 100U, &start) == 0U,
            "a buffer shorter than the requested window must be rejected");
}

static void test_no_trigger_falls_back_to_zero(void)
{
    int16_t samples[1000] = {0};
    uint16_t start = 77U;

    require(osc_window_find(samples, 1000U, 20U, 100U, &start) != 0U,
            "a complete window should exist even without a trigger");
    require(start == 0U,
            "no trigger should fall back to the beginning of the buffer");
}

static void test_negative_display_coordinates_do_not_wrap_unsigned(void)
{
    int16_t samples[100] = {0};
    uint16_t start = 99U;

    samples[10] = -25;
    samples[12] = 50;

    require(osc_window_find(samples, 100U, 40, 100U, &start) != 0U,
            "signed display coordinates should remain valid trigger input");
    require(start == 0U,
            "a signed crossing in one complete window should be usable");
}

static void test_auto_trigger_tracks_signal_offset_with_pretrigger(void)
{
    int16_t samples[160] = {0};
    uint16_t start = 0U;
    uint16_t i;

    for (i = 0U; i < 160U; ++i) {
        samples[i] = 55;
    }
    for (i = 40U; i < 80U; ++i) {
        samples[i] = 25;
    }

    require(osc_window_find_auto(samples, 160U, 100U, 25U, 8U, &start) != 0U,
            "an offset waveform should produce a complete auto-triggered window");
    require(start == 55U,
            "the rising display crossing at sample 80 should retain 25 pre-trigger samples");
}

static void test_auto_trigger_hysteresis_ignores_midpoint_noise(void)
{
    int16_t samples[180] = {0};
    uint16_t start = 0U;
    uint16_t i;

    for (i = 0U; i < 180U; ++i) {
        samples[i] = 60;
    }
    for (i = 30U; i < 70U; ++i) {
        samples[i] = 20;
    }
    samples[50] = 39;
    samples[51] = 41;
    samples[52] = 39;
    samples[53] = 41;

    require(osc_window_find_auto(samples, 180U, 100U, 20U, 8U, &start) != 0U,
            "a noisy waveform should still provide a complete window");
    require(start == 50U,
            "midpoint noise must not trigger before the signal crosses the upper hysteresis level");
}

static void test_auto_trigger_flat_signal_falls_back_safely(void)
{
    int16_t samples[100] = {0};
    uint16_t start = 77U;
    uint16_t i;

    for (i = 0U; i < 100U; ++i) {
        samples[i] = 42;
    }

    require(osc_window_find_auto(samples, 100U, 100U, 25U, 8U, &start) != 0U,
            "a flat but complete frame remains displayable");
    require(start == 0U,
            "a flat frame should fall back to the first complete window");
}

static void test_auto_trigger_rejects_invalid_pretrigger(void)
{
    int16_t samples[100] = {0};
    uint16_t start = 0U;

    require(osc_window_find_auto(samples, 100U, 100U, 100U, 8U, &start) == 0U,
            "pre-trigger samples must leave room for the trigger and post-trigger data");
}

int main(void)
{
    test_exact_width_uses_only_complete_window();
    test_normal_trigger_becomes_window_start();
    test_tail_trigger_is_clamped_to_complete_window();
    test_short_buffer_is_rejected();
    test_no_trigger_falls_back_to_zero();
    test_negative_display_coordinates_do_not_wrap_unsigned();
    test_auto_trigger_tracks_signal_offset_with_pretrigger();
    test_auto_trigger_hysteresis_ignores_midpoint_noise();
    test_auto_trigger_flat_signal_falls_back_safely();
    test_auto_trigger_rejects_invalid_pretrigger();
    puts("osc_window tests passed");
    return EXIT_SUCCESS;
}
