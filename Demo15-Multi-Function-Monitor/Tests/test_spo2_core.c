#include "spo2_core.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
    exit(EXIT_FAILURE); } } while (0)

static spo2_core_result_t feed_square_pair(spo2_core_t *core,
                                           uint16_t red_dc,
                                           uint16_t red_ac,
                                           uint16_t ir_dc,
                                           uint16_t ir_ac)
{
    uint16_t i;
    spo2_core_result_t result = {0U, 0U, 0U, 0U};
    for(i = 0U; i < SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL; ++i){
        const int32_t sign = ((i % 20U) < 10U) ? 1 : -1;
        result = SpO2Core_ProcessSample(
            core, (uint16_t)((int32_t)red_dc + sign * red_ac),
            SPO2_CHANNEL_RED);
        result = SpO2Core_ProcessSample(
            core, (uint16_t)((int32_t)ir_dc + sign * ir_ac),
            SPO2_CHANNEL_IR);
    }
    return result;
}

int main(void)
{
    spo2_core_t core;
    spo2_core_result_t result;
    uint16_t i;
    uint16_t reconstructed;

    reconstructed = SpO2Core_ReconstructInputSample(2479U, 1500U);
    CHECK(reconstructed >= 1235U);
    CHECK(reconstructed <= 1245U);
    CHECK(SpO2Core_ReconstructInputSample(2000U, 0U) == 0U);

    SpO2Core_Init(&core);
    result = feed_square_pair(&core, 2000U, 44U, 2000U, 100U);
    CHECK(result.updated != 0U);
    CHECK(result.valid != 0U);
    CHECK(result.percent == 99U);
    CHECK(result.ratio_q8 >= 105U);
    CHECK(result.ratio_q8 <= 120U);

    result = feed_square_pair(&core, 2000U, 100U, 2000U, 100U);
    CHECK(result.valid != 0U);
    CHECK(result.percent == 85U);

    /* A waveform configured as 2 Vpp with +1 V offset can briefly touch the
     * lower rail. A few endpoint samples are not the same as sustained
     * clipping and must not suppress an otherwise valid teaching result. */
    SpO2Core_Init(&core);
    for(i = 0U; i < SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL; ++i){
        const int32_t sign = ((i % 20U) < 10U) ? 1 : -1;
        const uint16_t red = ((i % 50U) == 0U) ? 0U :
                             (uint16_t)(2000 + sign * 100);
        const uint16_t ir = ((i % 50U) == 0U) ? 0U :
                            (uint16_t)(2000 + sign * 100);
        (void)SpO2Core_ProcessSample(&core, red, SPO2_CHANNEL_RED);
        result = SpO2Core_ProcessSample(&core, ir, SPO2_CHANNEL_IR);
    }
    CHECK(result.valid != 0U);
    CHECK(result.percent == 85U);

    SpO2Core_Init(&core);
    result = feed_square_pair(&core, 2000U, 0U, 2000U, 0U);
    CHECK(result.updated != 0U);
    CHECK(result.valid == 0U);

    SpO2Core_Init(&core);
    result = feed_square_pair(&core, 4095U, 100U, 2000U, 100U);
    CHECK(result.updated != 0U);
    CHECK(result.valid == 0U);

    SpO2Core_Init(&core);
    for(i = 0U; i < 2000U; ++i){
        result = SpO2Core_ProcessSample(&core, 2000U, SPO2_CHANNEL_RED);
    }
    CHECK(result.updated != 0U);
    CHECK(result.valid == 0U);
    CHECK(core.count_red == 0U);
    CHECK(core.count_ir == 0U);

    puts("SpO2 ratio-of-ratios tests passed");
    return EXIT_SUCCESS;
}
