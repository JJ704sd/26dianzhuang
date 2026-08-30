#include "timer_math.h"

#include <stdio.h>
#include <stdlib.h>

static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    require(timer_capture_delta(100U, 200U, 65535U) == 100U,
            "ordinary capture delta mismatch");
    require(timer_capture_delta(65535U, 0U, 65535U) == 1U,
            "single wrap delta mismatch");
    require(timer_capture_elapsed(0U, 16960U, 15U, 65535U) == 1000000U,
            "multi-wrap one-second interval mismatch");
    require(timer_pending_wrap_precedes_capture(5U, 65535U) != 0U,
            "low simultaneous capture must follow pending wrap");
    require(timer_pending_wrap_precedes_capture(65530U, 65535U) == 0U,
            "high simultaneous capture must precede pending wrap");
    require(timer_measurement_is_fresh(3000U, 500U, 2500U, 1U) != 0U,
            "timeout boundary must remain valid");
    require(timer_measurement_is_fresh(3001U, 500U, 2500U, 1U) == 0U,
            "stale measurement must expire");
    require(timer_measurement_is_fresh(5U, 0xFFFFFFF0U, 2500U, 1U) != 0U,
            "freshness must survive millisecond counter wrap");
    require(timer_measurement_is_fresh(10U, 10U, 2500U, 0U) == 0U,
            "invalid measurement must not appear fresh");

    puts("timer math tests passed");
    return EXIT_SUCCESS;
}
