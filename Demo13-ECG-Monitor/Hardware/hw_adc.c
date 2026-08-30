#include "hw_adc.h"

#include "systick.h"

void mx_adc_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_ADC);
    rcu_adc_clock_config(RCU_ADCCK_AHB_DIV9);

    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_3);

    adc_special_function_config(ADC_CONTINUOUS_MODE, DISABLE);
    adc_special_function_config(ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(ADC_INSERTED_CHANNEL_AUTO, DISABLE);
    adc_data_alignment_config(ADC_DATAALIGN_RIGHT);
    adc_channel_length_config(ADC_REGULAR_CHANNEL, 1U);
    adc_regular_channel_config(0U, ADC_CHANNEL_3, ADC_SAMPLETIME_13POINT5);
    adc_tempsensor_vrefint_enable();
    /* Startup Vref calibration uses software-triggered conversions. */
    adc_external_trigger_source_config(ADC_REGULAR_CHANNEL, ADC_EXTTRIG_REGULAR_NONE);
    adc_external_trigger_config(ADC_REGULAR_CHANNEL, ENABLE);

    adc_enable();
    delay_1ms(1U);
    adc_calibration_enable();
}

void mx_adc_dma_init(uint32_t adc_value, uint32_t number)
{
    dma_parameter_struct dma_data_parameter;

    /* TIMER0 remains disabled until ADC and DMA are ready. */
    adc_external_trigger_source_config(ADC_REGULAR_CHANNEL,
                                       ADC_EXTTRIG_REGULAR_T0_CH0);
    rcu_periph_clock_enable(RCU_DMA);
    nvic_irq_enable(DMA_Channel0_IRQn, 0U);
    dma_channel_disable(DMA_CH0);
    dma_deinit(DMA_CH0);
    dma_interrupt_flag_clear(DMA_CH0, DMA_INT_FLAG_G);

    dma_data_parameter.periph_addr = (uint32_t)(&ADC_RDATA);
    dma_data_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_data_parameter.memory_addr = adc_value;
    dma_data_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_data_parameter.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    dma_data_parameter.memory_width = DMA_MEMORY_WIDTH_16BIT;
    dma_data_parameter.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_data_parameter.number = number;
    dma_data_parameter.priority = DMA_PRIORITY_HIGH;

    dma_init(DMA_CH0, &dma_data_parameter);
    dma_circulation_enable(DMA_CH0);
    dma_interrupt_enable(DMA_CH0, DMA_CHXCTL_HTFIE | DMA_CHXCTL_FTFIE);
    dma_channel_enable(DMA_CH0);
    adc_dma_mode_enable();
}
