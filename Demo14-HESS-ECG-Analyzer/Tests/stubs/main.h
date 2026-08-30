#ifndef TEST_STUB_MAIN_H
#define TEST_STUB_MAIN_H

#include <stdint.h>

#define TIMER14 14U
#define TIMER_CH_0 0U
#define TIMER_EVENT_SRC_UPG 1U
#define GPIOA 0U
#define GPIO_MODE_OUTPUT 1U
#define GPIO_MODE_AF 2U
#define GPIO_PUPD_NONE 0U
#define GPIO_PIN_2 2U
#define GPIO_PIN_13 13U
#define GPIO_PIN_14 14U
#define GPIO_PIN_15 15U
#define GPIO_AF_0 0U

#define KEY1_Pin GPIO_PIN_13
#define KEY2_Pin GPIO_PIN_14
#define KEY3_Pin GPIO_PIN_15

void timer_channel_output_pulse_value_config(uint32_t timer,
                                              uint16_t channel,
                                              uint16_t pulse);
void timer_autoreload_value_config(uint32_t timer, uint16_t autoreload);
void timer_event_software_generate(uint32_t timer, uint16_t event);
void timer_enable(uint32_t timer);
void timer_disable(uint32_t timer);
void gpio_mode_set(uint32_t port, uint32_t mode, uint32_t pupd,
                   uint32_t pin);
void gpio_af_set(uint32_t port, uint32_t af, uint32_t pin);
void gpio_bit_reset(uint32_t port, uint32_t pin);

#endif
