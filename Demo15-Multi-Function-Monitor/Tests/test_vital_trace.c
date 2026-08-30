#include "vital_trace.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
    exit(EXIT_FAILURE); } } while (0)

static uint16_t count_columns(uint16_t span_samples, uint16_t sample_count)
{
    vital_trace_state_t state;
    vital_trace_column_t column;
    uint16_t emitted = 0U;
    uint16_t i;

    VitalTrace_Init(&state, 120U, span_samples);
    for (i = 0U; i < sample_count; ++i)
    {
        emitted = (uint16_t)(emitted +
                  VitalTrace_PushSample(&state, (int8_t)(i & 31U), &column));
    }
    return emitted;
}

int main(void)
{
    static const uint16_t spans[4] = {250U, 500U, 1000U, 1250U};
    static const uint16_t first_refresh_columns[4] = {9U, 4U, 2U, 1U};
    vital_trace_state_t state;
    vital_trace_column_t column;
    uint16_t i;
    uint16_t emitted;

    for (i = 0U; i < 4U; ++i)
    {
        CHECK(count_columns(spans[i], spans[i]) == 120U);
        CHECK(count_columns(spans[i], 20U) == first_refresh_columns[i]);
    }

    VitalTrace_Init(&state, 120U, 250U);
    CHECK(VitalTrace_PushSample(&state, 0, &column) == 0U);
    CHECK(VitalTrace_PushSample(&state, 100, &column) == 0U);
    CHECK(VitalTrace_PushSample(&state, -100, &column) == 1U);
    CHECK(column.column == 0U);
    CHECK(column.first == 0);
    CHECK(column.minimum == -100);
    CHECK(column.maximum == 100);
    CHECK(column.last == -100);
    CHECK(column.connect_previous == 0U);

    VitalTrace_Init(&state, 3U, 6U);
    emitted = 0U;
    for (i = 0U; i < 8U; ++i)
    {
        if (VitalTrace_PushSample(&state, (int8_t)i, &column) != 0U)
        {
            if (emitted == 0U)
            {
                CHECK(column.column == 0U);
                CHECK(column.connect_previous == 0U);
            }
            else if (emitted < 3U)
            {
                CHECK(column.column == emitted);
                CHECK(column.connect_previous != 0U);
            }
            else
            {
                CHECK(column.column == 0U);
                CHECK(column.connect_previous == 0U);
            }
            ++emitted;
        }
    }
    CHECK(emitted == 4U);

    puts("vital trace tests passed");
    return EXIT_SUCCESS;
}
