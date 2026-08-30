#ifndef SPO2_CORE_H
#define SPO2_CORE_H

#include <stdint.h>

#define SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL 500U

typedef enum
{
    SPO2_CHANNEL_RED = 0,
    SPO2_CHANNEL_IR = 1
} spo2_channel_t;

typedef struct
{
    uint8_t percent;
    uint16_t ratio_q8;
    uint8_t valid;
    uint8_t updated;
} spo2_core_result_t;

typedef struct
{
    uint32_t sum_red;
    uint32_t sum_ir;
    uint32_t square_sum_red;
    uint32_t square_sum_ir;
    uint16_t count_red;
    uint16_t count_ir;
    uint16_t rail_count;
    uint16_t window_age;
    spo2_core_result_t latest;
} spo2_core_t;

void SpO2Core_Init(spo2_core_t *core);
uint16_t SpO2Core_ReconstructInputSample(uint16_t adc_sample,
                                        uint16_t vref_calibration);
spo2_core_result_t SpO2Core_ProcessSample(spo2_core_t *core,
                                         uint16_t adc_sample,
                                         spo2_channel_t channel);
spo2_core_result_t SpO2Core_GetResult(const spo2_core_t *core);

#endif
