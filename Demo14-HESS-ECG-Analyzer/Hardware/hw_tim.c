#include "hw_tim.h"

#define ADC_ECG_TIMER_PERIOD 3999U
#define ADC_ECG_TIMER_COMPARE 2000U

void mx_tim0_adc_init(void)
{
    timer_parameter_struct timer_initpara;
    timer_oc_parameter_struct timer_ocinitpara;

    rcu_periph_clock_enable(RCU_TIMER0);
    timer_deinit(TIMER0);
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler = 71U;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = ADC_ECG_TIMER_PERIOD;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0U;
    timer_init(TIMER0, &timer_initpara);

    timer_channel_output_struct_para_init(&timer_ocinitpara);
    timer_ocinitpara.outputstate = TIMER_CCX_ENABLE;
    timer_ocinitpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocinitpara.ocpolarity = TIMER_OC_POLARITY_HIGH;
    timer_ocinitpara.ocnpolarity = TIMER_OCN_POLARITY_HIGH;
    timer_ocinitpara.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
    timer_ocinitpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER0, TIMER_CH_0, &timer_ocinitpara);

    /* One compare event in each 4 ms period triggers one ECG conversion. */
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0,
                                            ADC_ECG_TIMER_COMPARE);
    timer_channel_output_mode_config(TIMER0, TIMER_CH_0, TIMER_OC_MODE_PWM1);
    timer_channel_output_shadow_config(TIMER0, TIMER_CH_0,
                                       TIMER_OC_SHADOW_DISABLE);
    timer_auto_reload_shadow_enable(TIMER0);
    timer_primary_output_config(TIMER0, ENABLE);
    timer_disable(TIMER0);
}

void mx_tim15_init(void)
{
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER15);
    timer_deinit(TIMER15);
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler = 71U;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = 999U;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0U;
    timer_init(TIMER15, &timer_initpara);
    timer_interrupt_flag_clear(TIMER15, TIMER_INT_FLAG_UP);
    nvic_irq_enable(TIMER15_IRQn, 0U);
    timer_interrupt_enable(TIMER15, TIMER_INT_UP);
    timer_disable(TIMER15);
}
