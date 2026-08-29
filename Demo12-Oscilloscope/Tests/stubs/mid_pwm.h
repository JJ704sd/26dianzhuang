#ifndef TEST_STUB_MID_PWM_H
#define TEST_STUB_MID_PWM_H

#include "main.h"

#define PWM_ON  0x01U
#define PWM_OFF 0x02U

void set_pwm_duty(uint16_t pwm_duty);
uint16_t get_pwm_duty(void);
void set_pwm_period(uint16_t pwm_period);
uint16_t get_pwm_period(void);
void set_pwm_state(uint8_t state);
uint8_t get_pwm_state(void);
uint16_t get_pwm_out_freq(void);

#endif
