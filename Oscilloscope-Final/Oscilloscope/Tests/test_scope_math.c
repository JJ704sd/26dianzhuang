#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "scope_math.h"

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void test_rising_edge_stabilizes_window(void)
{
    float samples[300] = {0.0f};
    scope_frame_info_t info;
    uint16_t i;

    for (i = 0U; i < 300U; ++i) {
        samples[i] = ((i % 40U) < 20U) ? 0.5f : 2.5f;
    }

    require(scope_analyze_frame(samples, 300U, 100U, &info) != 0U,
            "a complete frame should be accepted");
    require(info.trigger_index == 19U,
            "the first rising midpoint crossing should anchor the window");
    require(info.minimum == 0.5f && info.maximum == 2.5f,
            "frame extrema should be measured from the captured samples");
    require(info.peak_to_peak == 2.0f,
            "Vpp should be maximum minus minimum");
}

static void test_no_edge_does_not_reuse_previous_trigger(void)
{
    float samples[300];
    scope_frame_info_t info = {0.0f, 0.0f, 0.0f, 77U};
    uint16_t i;

    for (i = 0U; i < 300U; ++i) {
        samples[i] = 1.25f;
    }

    require(scope_analyze_frame(samples, 300U, 100U, &info) != 0U,
            "a flat but complete frame should be accepted");
    require(info.trigger_index == 0U,
            "a frame without a crossing should start at zero");
    require(scope_scale_to_plot(1.25f, info.minimum, info.maximum, 50) == 25,
            "a flat trace should be centered instead of divided by zero");
}

static void test_tail_trigger_is_clamped_and_plot_is_bounded(void)
{
    float samples[300] = {0.0f};
    scope_frame_info_t info;
    uint16_t i;

    for (i = 0U; i < 299U; ++i) {
        samples[i] = -1.0f;
    }
    samples[299] = 3.0f;

    require(scope_analyze_frame(samples, 300U, 100U, &info) != 0U,
            "a tail crossing should still produce a window");
    require(info.trigger_index == 200U,
            "the display window must remain inside the capture buffer");
    require(scope_scale_to_plot(-5.0f, -1.0f, 3.0f, 50) == 0,
            "values below the measured range should clamp to the plot bottom");
    require(scope_scale_to_plot(8.0f, -1.0f, 3.0f, 50) == 49,
            "values above the measured range should clamp to the plot top");
}

int main(void)
{
    test_rising_edge_stabilizes_window();
    test_no_edge_does_not_reuse_previous_trigger();
    test_tail_trigger_is_clamped_and_plot_is_bounded();
    puts("scope math tests passed");
    return EXIT_SUCCESS;
}
