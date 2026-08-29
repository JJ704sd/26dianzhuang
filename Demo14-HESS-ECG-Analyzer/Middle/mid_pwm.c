#include "mid_pwm.h"

#include "main.h"

static uint8_t pwm_state = PWM_OFF;
static uint16_t pwm_period = 1000U;
static uint16_t pwm_duty = 500U;

void set_pwm_duty(uint16_t duty)
{
    if (duty > pwm_period)
    {
        duty = pwm_period;
    }
    pwm_duty = duty;
    timer_channel_output_pulse_value_config(TIMER14, TIMER_CH_0, duty);
}

uint16_t get_pwm_duty(void)
{
    return pwm_duty;
}

void set_pwm_period(uint16_t period)
{
    if (period == 0U)
    {
        period = 1U;
    }
    pwm_period = period;
    timer_autoreload_value_config(TIMER14, (uint16_t)(period - 1U));
    if (pwm_duty > pwm_period)
    {
        set_pwm_duty(pwm_period);
    }
}

uint16_t get_pwm_period(void)
{
    return pwm_period;
}

void set_pwm_state(uint8_t state)
{
    if (state == PWM_ON)
    {
        gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_2);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2);
        timer_event_software_generate(TIMER14, TIMER_EVENT_SRC_UPG);
        timer_enable(TIMER14);
        pwm_state = PWM_ON;
    }
    else if (state == PWM_OFF)
    {
        timer_disable(TIMER14);
        gpio_bit_reset(GPIOA, GPIO_PIN_2);
        gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_2);
        pwm_state = PWM_OFF;
    }
}

uint8_t get_pwm_state(void)
{
    return pwm_state;
}

uint16_t get_pwm_out_freq(void)
{
    return (uint16_t)(PWM_TIMER_FREQ_HZ / pwm_period);
}
