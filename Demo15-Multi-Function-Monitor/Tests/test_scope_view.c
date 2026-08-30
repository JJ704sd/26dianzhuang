#include "scope_view.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
    exit(EXIT_FAILURE); } } while (0)

static void make_square(uint16_t *history, uint16_t count, uint16_t shift)
{
    uint16_t i;
    for (i = 0U; i < count; ++i)
    {
        history[i] = (((i + shift) % 20U) < 10U) ? 1000U : 3000U;
    }
}

int main(void)
{
    uint16_t history[200];
    uint16_t shifted[200];
    uint16_t free_a[100];
    uint16_t free_b[100];
    uint16_t trig_a[100];
    uint16_t trig_b[100];
    scope_view_info_t info;
    uint16_t i;
    uint8_t slow_history[250];
    uint16_t slow_output[120];

    make_square(history, 200U, 0U);
    make_square(shifted, 200U, 5U);
    CHECK(ScopeView_CopyWindow(history, 200U, 200U, 0U, 200U, 0U,
                               SCOPE_VIEW_FREE, free_a, 100U, &info) == 100U);
    CHECK(ScopeView_CopyWindow(history, 200U, 200U, 0U, 200U, 2U,
                               SCOPE_VIEW_FREE, free_b, 100U, &info) == 100U);
    /* A display phase must never rotate one captured window and splice its
     * newest end back to its oldest beginning. That artificial seam was
     * rendered as a moving square-wave spike. */
    for (i = 0U; i < 100U; ++i) { CHECK(free_a[i] == free_b[i]); }

    CHECK(ScopeView_CopyWindow(history, 200U, 200U, 0U, 200U, 0U,
                               SCOPE_VIEW_RISING, trig_a, 100U, &info) == 100U);
    CHECK(info.trigger_found != 0U);
    CHECK(ScopeView_CopyWindow(shifted, 200U, 200U, 0U, 200U, 0U,
                               SCOPE_VIEW_RISING, trig_b, 100U, &info) == 100U);
    for (i = 0U; i < 100U; ++i) { CHECK(trig_a[i] == trig_b[i]); }

    /* A requested display lag selects an older but still strictly
     * chronological window. It must never rotate samples inside one span. */
    for (i = 0U; i < 200U; ++i) { history[i] = i; }
    CHECK(ScopeView_CopyUniformWindow(history, 200U, 200U, 0U,
                                      100U, 37U, SCOPE_VIEW_FREE,
                                      free_a, 100U, &info) == 100U);
    CHECK(free_a[0] == 63U);
    CHECK(free_a[99] == 162U);
    for (i = 1U; i < 100U; ++i)
    {
        CHECK(free_a[i] == (uint16_t)(free_a[i - 1U] + 1U));
    }

    /* One second of packed 250 Sa/s history must retain a full 1 Hz wave.
     * Values 64 and 192 represent 12-bit ADC counts 1024 and 3072.
     */
    for (i = 0U; i < 250U; ++i)
    {
        slow_history[i] = (i < 125U) ? 64U : 192U;
    }
    CHECK(ScopeView_CopyPacked12Window(slow_history, 250U, 250U, 0U,
                                       250U, 0U, SCOPE_VIEW_FREE,
                                       slow_output, 120U, &info) == 120U);
    CHECK(info.source_count == 250U);
    CHECK(info.minimum == 1024U);
    CHECK(info.maximum == 3072U);
    CHECK(slow_output[0] == 1024U);
    CHECK(slow_output[119] == 3072U);

    for (i = 0U; i < 250U; ++i) { slow_history[i] = (uint8_t)i; }
    CHECK(ScopeView_CopyPacked12Window(slow_history, 250U, 250U, 0U,
                                       250U, 0U, SCOPE_VIEW_FREE,
                                       slow_output, 120U, &info) == 120U);
    CHECK(slow_output[0] == 0U);
    CHECK(slow_output[119] == 3984U);

    CHECK(ScopeView_CopyPacked12Window(slow_history, 250U, 250U, 0U,
                                       250U, 37U, SCOPE_VIEW_FREE,
                                       slow_output, 120U, &info) == 120U);
    CHECK(slow_output[0] == 0U);
    CHECK(slow_output[119] == 3984U);

    {
        uint8_t wrapped_history[1250];
        for (i = 0U; i < 1250U; ++i)
        {
            wrapped_history[(uint16_t)((100U + i) % 1250U)] =
                (i < 625U) ? 64U : 192U;
        }
        CHECK(ScopeView_CopyPacked12Window(wrapped_history, 1250U, 1250U,
                                           100U, 1250U, 0U,
                                           SCOPE_VIEW_RISING, slow_output,
                                           120U, &info) == 120U);
        CHECK(info.trigger_found != 0U);
        CHECK(info.minimum == 1024U);
        CHECK(info.maximum == 3072U);
    }

    /* The 1-5 s path represents low-frequency waveforms. Short one- or
     * two-sample ADC/front-end glitches must not inflate Vpp or appear as
     * needles on an otherwise clean square wave. At 250 Sa/s these fixtures
     * are at most 8 ms wide, far shorter than the 2 Hz half-period. */
    for (i = 0U; i < 250U; ++i)
    {
        slow_history[i] = (i < 125U) ? 64U : 192U;
    }
    slow_history[20U] = 255U;
    slow_history[21U] = 255U;
    slow_history[180U] = 0U;
    slow_history[181U] = 0U;
    CHECK(ScopeView_CopyPacked12Window(slow_history, 250U, 250U, 0U,
                                       250U, 0U, SCOPE_VIEW_FREE,
                                       slow_output, 120U, &info) == 120U);
    CHECK(info.minimum == 1024U);
    CHECK(info.maximum == 3072U);
    for (i = 0U; i < 120U; ++i)
    {
        CHECK((slow_output[i] == 1024U) || (slow_output[i] == 3072U));
    }

    /* Fast-scope downsampling must retain narrow raw noise excursions instead
     * of selecting one evenly-spaced sample and silently dropping the other
     * extrema. The two spikes are deliberately between the legacy sample
     * positions for a 240-to-120 point conversion.
     */
    {
        uint16_t noisy_history[240];
        uint16_t noisy_output[120];
        uint8_t found_low = 0U;
        uint8_t found_high = 0U;

        for (i = 0U; i < 240U; ++i) { noisy_history[i] = 2000U; }
        noisy_history[1] = 1000U;
        noisy_history[3] = 3000U;
        CHECK(ScopeView_CopyWindow(noisy_history, 240U, 240U, 0U,
                                   240U, 0U, SCOPE_VIEW_FREE,
                                   noisy_output, 120U, &info) == 120U);
        for (i = 0U; i < 120U; ++i)
        {
            if (noisy_output[i] == 1000U) { found_low = 1U; }
            if (noisy_output[i] == 3000U) { found_high = 1U; }
        }
        CHECK(found_low != 0U);
        CHECK(found_high != 0U);
    }

    /* Ordinary scope rendering must preserve time weighting. Peak-pair
     * compression is intentionally retained only for the NOISE view because
     * it changes a 25% square wave into roughly 33% high samples. */
    {
        uint16_t square_history[400];
        uint16_t square_output[120];
        uint16_t input_high_count = 0U;

        for (i = 0U; i < 400U; ++i)
        {
            square_history[i] = ((i % 20U) < 5U) ? 1000U : 3000U;
        }
        CHECK(ScopeView_CopyUniformWindow(square_history, 400U, 400U, 0U,
                                          400U, 29U, SCOPE_VIEW_FREE,
                                          square_output, 120U,
                                          &info) == 120U);
        for (i = 0U; i < 120U; ++i)
        {
            if (square_output[i] < 2000U) { ++input_high_count; }
        }
        CHECK(input_high_count >= 27U);
        CHECK(input_high_count <= 35U);
    }

    puts("scope view tests passed");
    return EXIT_SUCCESS;
}
