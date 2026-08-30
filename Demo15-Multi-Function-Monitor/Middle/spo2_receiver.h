#ifndef SPO2_RECEIVER_H
#define SPO2_RECEIVER_H

#include <stdint.h>

#include "spo2_core.h"

typedef enum
{
    SPO2_SOURCE_WAITING = 0,
    SPO2_SOURCE_DUTY_CODED,
    SPO2_SOURCE_DUAL_CHANNEL
} spo2_source_t;

typedef struct
{
    spo2_core_t dual_core;
    spo2_core_result_t duty_latest;
    spo2_core_result_t latest;
    uint32_t duty_sum;
    uint16_t duty_minimum;
    uint16_t duty_maximum;
    uint16_t duty_count;
    uint16_t duty_rail_count;
    uint8_t tag_mask;
    spo2_source_t source;
} spo2_receiver_t;

void SpO2Receiver_Init(spo2_receiver_t *receiver);
spo2_core_result_t SpO2Receiver_ProcessSample(spo2_receiver_t *receiver,
                                              uint16_t raw_adc_sample,
                                              uint16_t reconstructed_sample,
                                              uint8_t wavelength_tag);
spo2_core_result_t SpO2Receiver_GetResult(const spo2_receiver_t *receiver);
spo2_source_t SpO2Receiver_GetSource(const spo2_receiver_t *receiver);

#endif
