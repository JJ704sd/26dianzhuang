#include "mid_pwm.h"

#include <stdio.h>
#include <stdlib.h>

static uint16_t hardware_arr;
static uint16_t hardware_compare;
static unsigned int update_events;
static unsigned int timer_running;

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

int main(void)
{
    set_pwm_period(1000U);
    require(hardware_arr == 999U,
            "a 1000-tick PWM period must program ARR=999");
    require(get_pwm_out_freq() == 1000U,
            "reported and hardware PWM frequency should agree");

    set_pwm_duty(1200U);
    require(get_pwm_duty() == 1000U && hardware_compare == 1000U,
            "duty must not exceed the configured period");

    set_pwm_period(0U);
    require(get_pwm_period() == 1U && hardware_arr == 0U,
            "zero period must clamp to one timer tick");

    update_events = 0U;
    set_pwm_state(PWM_ON);
    require(timer_running == 1U && update_events == 1U,
            "enabling PWM must commit shadow registers first");
    set_pwm_state(PWM_OFF);
    require(timer_running == 0U, "disabling PWM must stop the timer");

    puts("PWM tests passed");
    return EXIT_SUCCESS;
}
