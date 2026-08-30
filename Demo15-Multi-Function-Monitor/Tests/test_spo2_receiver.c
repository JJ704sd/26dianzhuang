#include "spo2_receiver.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
    exit(EXIT_FAILURE); } } while (0)

static spo2_core_result_t feed_duty_window(spo2_receiver_t *receiver,
                                           uint8_t duty_percent,
                                           uint16_t period_samples,
                                           uint16_t phase_samples)
{
    spo2_core_result_t result = {0U, 0U, 0U, 0U};
    uint16_t i;

    for(i = 0U; i < SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL; i++){
        const uint16_t high_samples =
            (uint16_t)(((uint32_t)period_samples * duty_percent) / 100U);
        const uint8_t input_high = (uint8_t)(
            (((i + phase_samples) % period_samples) < high_samples));
        const uint16_t raw_adc = (input_high != 0U) ? 1200U : 3000U;
        const uint16_t reconstructed =
            (input_high != 0U) ? 2800U : 1000U;
        result = SpO2Receiver_ProcessSample(receiver, raw_adc,
                                            reconstructed, 0U);
    }
    return result;
}

int main(void)
{
    spo2_receiver_t receiver;
    spo2_core_result_t result = {0U, 0U, 0U, 0U};
    uint16_t i;

    SpO2Receiver_Init(&receiver);
    result = feed_duty_window(&receiver, 25U, 250U, 0U);
    CHECK(result.valid != 0U);
    CHECK(result.percent == 78U);
    CHECK(SpO2Receiver_GetSource(&receiver) == SPO2_SOURCE_DUTY_CODED);

    SpO2Receiver_Init(&receiver);
    result = feed_duty_window(&receiver, 50U, 125U, 31U);
    CHECK(result.valid != 0U);
    CHECK(result.percent == 85U);
    CHECK(SpO2Receiver_GetSource(&receiver) == SPO2_SOURCE_DUTY_CODED);

    SpO2Receiver_Init(&receiver);
    result = feed_duty_window(&receiver, 75U, 250U, 73U);
    CHECK(result.valid != 0U);
    CHECK(result.percent == 93U);
    CHECK(SpO2Receiver_GetSource(&receiver) == SPO2_SOURCE_DUTY_CODED);

    SpO2Receiver_Init(&receiver);
    for(i = 0U; i < SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL; i++){
        const int32_t sign = ((i % 20U) < 10U) ? 1 : -1;
        (void)SpO2Receiver_ProcessSample(
            &receiver, (uint16_t)(2000 + sign * 44),
            (uint16_t)(2000 + sign * 44), 0U);
        result = SpO2Receiver_ProcessSample(
            &receiver, (uint16_t)(2000 + sign * 100),
            (uint16_t)(2000 + sign * 100), 1U);
    }
    CHECK(result.valid != 0U);
    CHECK(result.percent == 99U);
    CHECK(SpO2Receiver_GetSource(&receiver) == SPO2_SOURCE_DUAL_CHANNEL);

    /* External 0 V after reconstruction is not an ADC rail. */
    SpO2Receiver_Init(&receiver);
    for(i = 0U; i < SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL; i++){
        const uint16_t reconstructed = ((i % 10U) < 2U) ? 0U : 2000U;
        (void)SpO2Receiver_ProcessSample(&receiver, 2000U,
                                         reconstructed, 0U);
        result = SpO2Receiver_ProcessSample(&receiver, 2000U,
                                            reconstructed, 1U);
    }
    CHECK(result.valid != 0U);
    CHECK(result.percent == 85U);
    CHECK(SpO2Receiver_GetSource(&receiver) == SPO2_SOURCE_DUAL_CHANNEL);

    /* Losing the PA6 pair must eventually retire 2CH and return to DUT. */
    for(i = 0U; i < 2000U; i++){
        const uint8_t input_high = (uint8_t)((i % 250U) < 125U);
        const uint16_t raw_adc = (input_high != 0U) ? 1200U : 3000U;
        const uint16_t reconstructed =
            (input_high != 0U) ? 2800U : 1000U;
        result = SpO2Receiver_ProcessSample(&receiver, raw_adc,
                                            reconstructed, 0U);
    }
    CHECK(result.valid != 0U);
    CHECK(result.percent == 85U);
    CHECK(SpO2Receiver_GetSource(&receiver) == SPO2_SOURCE_DUTY_CODED);

    /* A fresh pair after a DUT window must not reuse stale one-sided RED. */
    SpO2Receiver_Init(&receiver);
    result = feed_duty_window(&receiver, 50U, 250U, 0U);
    CHECK(result.valid != 0U);
    for(i = 0U; i < SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL; i++){
        const int32_t sign = ((i % 20U) < 10U) ? 1 : -1;
        (void)SpO2Receiver_ProcessSample(
            &receiver, 2000U, (uint16_t)(2000 + sign * 44), 0U);
        result = SpO2Receiver_ProcessSample(
            &receiver, 2000U, (uint16_t)(2000 + sign * 100), 1U);
    }
    CHECK(result.valid != 0U);
    CHECK(result.percent == 99U);
    CHECK(SpO2Receiver_GetSource(&receiver) == SPO2_SOURCE_DUAL_CHANNEL);

    SpO2Receiver_Init(&receiver);
    for(i = 0U; i < SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL; i++){
        result = SpO2Receiver_ProcessSample(&receiver, 2000U, 2000U, 0U);
    }
    CHECK(result.valid == 0U);
    CHECK(SpO2Receiver_GetSource(&receiver) == SPO2_SOURCE_WAITING);

    SpO2Receiver_Init(&receiver);
    for(i = 0U; i < SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL; i++){
        uint16_t raw_adc = ((i % 250U) < 125U) ? 1200U : 3000U;
        if(i == 100U){
            raw_adc = 0U;
        }
        result = SpO2Receiver_ProcessSample(&receiver, raw_adc, 2000U, 0U);
    }
    CHECK(result.updated != 0U);
    CHECK(result.valid == 0U);

    SpO2Receiver_Init(&receiver);
    result = feed_duty_window(&receiver, 50U, 250U, 0U);
    CHECK(result.valid != 0U);
    for(i = 0U; i < SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL; i++){
        result = SpO2Receiver_ProcessSample(&receiver, 2000U, 2000U, 0U);
    }
    CHECK(result.updated != 0U);
    CHECK(result.valid == 0U);
    CHECK(SpO2Receiver_GetSource(&receiver) == SPO2_SOURCE_WAITING);

    puts("SpO2 receiver tests passed");
    return EXIT_SUCCESS;
}
