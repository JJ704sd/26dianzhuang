#include "mid_adc.h"

#include "hw_adc.h"

void Set_ADC_Channel(uint8_t channel)
{
    adc_regular_channel_config(0U, channel, ADC_SAMPLETIME_13POINT5);
}

uint16_t Get_ADC_Latest(void)
{
    return adc_regular_data_read();
}
