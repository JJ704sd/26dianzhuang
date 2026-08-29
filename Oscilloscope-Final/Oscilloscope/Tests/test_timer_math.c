#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer_math.h"

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    require(timer_capture_elapsed(100U, 200U, 0U, 65535U) == 100U,
            "ordinary captures should subtract directly");
    require(timer_capture_elapsed(65535U, 0U, 0U, 65535U) == 1U,
            "a one-tick wrap must not be lost");
    require(timer_capture_elapsed(0U, 16960U, 15U, 65535U) == 1000000U,
            "multiple wraps should preserve a one-second interval");
    require(timer_pending_wrap_precedes_capture(5U, 65535U) != 0U,
            "a low capture with both flags belongs after the pending wrap");
    require(timer_pending_wrap_precedes_capture(65530U, 65535U) == 0U,
            "a high capture with both flags belongs before the pending wrap");
    puts("timer math tests passed");
    return EXIT_SUCCESS;
}
