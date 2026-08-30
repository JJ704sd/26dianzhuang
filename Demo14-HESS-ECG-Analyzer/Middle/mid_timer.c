#include "mid_timer.h"

static __IO uint16_t key_timer_value;
static __IO uint16_t tft_timer_value;
static __IO uint32_t runtime_ms;
static volatile uint8_t key_timer_bit = RUN_MS_TIMER;
static volatile uint8_t tft_timer_bit = RUN_MS_TIMER;

void TIMER15_IRQHandler(void)
{
    timer_interrupt_flag_clear(TIMER15, TIMER_INT_FLAG_UP);
    runtime_ms++;
    App_TimerTick1ms();

    if (key_timer_bit == RUN_MS_TIMER)
    {
        key_timer_value++;
        if (key_timer_value >= 10000U)
        {
            key_timer_value = 0U;
        }
    }
    if (tft_timer_bit == RUN_MS_TIMER)
    {
        tft_timer_value++;
        if (tft_timer_value >= 10000U)
        {
            tft_timer_value = 0U;
        }
    }
}

uint32_t get_runtime_ms(void)
{
    return runtime_ms;
}

uint16_t get_key_timer_value(void)
{
    return key_timer_value;
}

void set_key_timer_value(uint16_t value)
{
    key_timer_value = value;
}

void set_key_bit_value(uint8_t value)
{
    key_timer_bit = value;
}

uint16_t get_tft_timer_value(void)
{
    return tft_timer_value;
}

void set_tft_timer_value(uint16_t value)
{
    tft_timer_value = value;
}

void set_tft_bit_value(uint8_t value)
{
    tft_timer_bit = value;
}
