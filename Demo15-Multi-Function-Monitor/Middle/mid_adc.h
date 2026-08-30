#ifndef __MID_ADC_H
#define __MID_ADC_H

#include "main.h"

#define ADC_CONVERT_FINSIH    0x01U
#define ADC_CONVERT_UN_FINSIH 0x02U
#define ADC_NUM               1000U

typedef void (*mid_adc_dma_callback_t)(const uint16_t *samples, uint16_t count);

void Set_ADC_Channel(uint8_t channel);
uint16_t Get_ADC_Sample(void);
uint16_t Get_ADC_Average(uint16_t num);
void mid_adc_start_dma(uint16_t *buffer, uint16_t count,
                       mid_adc_dma_callback_t callback);
uint8_t get_adc_convert_value(void);
void set_adc_convert_value(uint8_t bit);

#endif
