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
    uint8_t differs = 0U;

    make_square(history, 200U, 0U);
    make_square(shifted, 200U, 5U);
    CHECK(ScopeView_CopyWindow(history, 200U, 200U, 0U, 200U, 0U,
                               SCOPE_VIEW_FREE, free_a, 100U, &info) == 100U);
    CHECK(ScopeView_CopyWindow(history, 200U, 200U, 0U, 200U, 2U,
                               SCOPE_VIEW_FREE, free_b, 100U, &info) == 100U);
    for (i = 0U; i < 100U; ++i)
    {
        if (free_a[i] != free_b[i]) { differs = 1U; }
    }
    CHECK(differs != 0U);

    CHECK(ScopeView_CopyWindow(history, 200U, 200U, 0U, 200U, 0U,
                               SCOPE_VIEW_RISING, trig_a, 100U, &info) == 100U);
    CHECK(info.trigger_found != 0U);
    CHECK(ScopeView_CopyWindow(shifted, 200U, 200U, 0U, 200U, 0U,
                               SCOPE_VIEW_RISING, trig_b, 100U, &info) == 100U);
    for (i = 0U; i < 100U; ++i) { CHECK(trig_a[i] == trig_b[i]); }

    puts("scope view tests passed");
    return EXIT_SUCCESS;
}
