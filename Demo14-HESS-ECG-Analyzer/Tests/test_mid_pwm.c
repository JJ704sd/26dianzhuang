#include "mid_pwm.h"
#include "main.h"

#include <stdio.h>
#include <stdlib.h>

static uint16_t hardware_arr;
static uint16_t hardware_compare;
static unsigned int update_events;
static unsigned int timer_running;
static uint32_t pa2_mode;
static unsigned int pa2_is_low;

static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

void timer_channel_output_pulse_value_config(uint32_t timer,
                                              uint16_t channel,
                                              uint16_t pulse)
{
    (void)timer;
    (void)channel;
    hardware_compare = pulse;
}

void timer_autoreload_value_config(uint32_t timer, uint16_t autoreload)
{
    (void)timer;
    hardware_arr = autoreload;
}

void timer_event_software_generate(uint32_t timer, uint16_t event)
{
    (void)timer;
    (void)event;
    update_events++;
}

void timer_enable(uint32_t timer)
{
    (void)timer;
    timer_running = 1U;
}

void timer_disable(uint32_t timer)
{
    (void)timer;
    timer_running = 0U;
}

void gpio_mode_set(uint32_t port, uint32_t mode, uint32_t pupd,
                   uint32_t pin)
{
    (void)port;
    (void)pupd;
    (void)pin;
    pa2_mode = mode;
}

void gpio_af_set(uint32_t port, uint32_t af, uint32_t pin)
{
    (void)port;
    (void)af;
    (void)pin;
}

void gpio_bit_reset(uint32_t port, uint32_t pin)
{
    (void)port;
    (void)pin;
    pa2_is_low = 1U;
}

int main(void)
{
    set_pwm_period(1000U);
    require(hardware_arr == 999U, "1000 ticks must program ARR=999");
    require(get_pwm_out_freq() == 1000U, "reported PWM frequency mismatch");

    set_pwm_duty(1200U);
    require(get_pwm_duty() == 1000U && hardware_compare == 1000U,
            "duty must clamp to period");

    set_pwm_period(0U);
    require(get_pwm_period() == 1U && hardware_arr == 0U,
            "zero period must clamp to one tick");

    set_pwm_period(500U);
    set_pwm_duty(250U);
    require(get_pwm_out_freq() == 2000U && hardware_compare == 250U,
            "2000 Hz at 50 percent configuration mismatch");

    update_events = 0U;
    pa2_mode = 0U;
    set_pwm_state(PWM_ON);
    require(timer_running == 1U && update_events == 1U &&
            pa2_mode == GPIO_MODE_AF,
            "enable must restore PA2 alternate function and commit shadow registers");
    pa2_is_low = 0U;
    set_pwm_state(PWM_OFF);
    require(timer_running == 0U && pa2_mode == GPIO_MODE_OUTPUT &&
            pa2_is_low == 1U,
            "disable must stop TIMER14 and drive PA2 low");

    puts("PWM tests passed");
    return EXIT_SUCCESS;
}
