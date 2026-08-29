#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer_math.h"

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
            "ordinary captures should subtract directly");
    require(timer_capture_delta(65535U, 0U, 65535U) == 1U,
            "a one-tick counter wrap must not become zero");
    require(timer_capture_delta(7U, 7U, 65535U) == 65536U,
            "equal captures represent one complete counter period");
    require(timer_capture_elapsed(0U, 16960U, 15U, 65535U) == 1000000U,
            "overflow accumulation should preserve a one-second interval");
    require(timer_capture_elapsed(65535U, 0U, 0U, 65535U) == 1U,
            "a pending wrap should still count as one tick");
    require(timer_pending_wrap_precedes_capture(5U, 65535U) != 0U,
            "a low capture with both flags belongs after the pending wrap");
    require(timer_pending_wrap_precedes_capture(65530U, 65535U) == 0U,
            "a high capture with both flags belongs before the pending wrap");
    puts("timer math tests passed");
    return EXIT_SUCCESS;
}
