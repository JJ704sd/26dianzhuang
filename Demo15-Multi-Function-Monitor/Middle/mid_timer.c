#include "mid_timer.h"

static __IO uint16_t key_timer_value;
static __IO uint16_t tft_timer_value;
static uint8_t key_timer_bit = RUN_MS_TIMER;
static uint8_t tft_timer_bit = RUN_MS_TIMER;
static mid_timer_callback_t periodic_callback;
static uint16_t periodic_callback_period = 1U;
static uint16_t periodic_callback_count;

static __IO uint16_t ccnumber;
static __IO uint32_t freq;
static __IO uint16_t readvalue1;
static __IO uint16_t readvalue2;
static __IO uint16_t freq_age_ms = 1000U;

void TIMER15_IRQHandler(void)
{
    if(freq_age_ms < 1000U){
        freq_age_ms++;
    }
    periodic_callback_count++;
    if(periodic_callback_count >= periodic_callback_period){
        periodic_callback_count = 0U;
        if(periodic_callback != 0){
            periodic_callback();
        }
    }

    if(key_timer_bit == RUN_MS_TIMER){
        key_timer_value++;
        if(key_timer_value >= 10000U){
            key_timer_value = 0U;
        }
    }
    if(tft_timer_bit == RUN_MS_TIMER){
        tft_timer_value++;
        if(tft_timer_value >= 10000U){
            tft_timer_value = 0U;
        }
    }
    timer_interrupt_flag_clear(TIMER15, TIMER_INT_FLAG_UP);
}

void mid_timer_register_periodic_callback(mid_timer_callback_t callback, uint16_t period_ms)
{
    periodic_callback = callback;
    periodic_callback_period = (period_ms == 0U) ? 1U : period_ms;
    periodic_callback_count = 0U;
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

void TIMER2_IRQHandler(void)
{
    uint32_t count;

    if(timer_interrupt_flag_get(TIMER2, TIMER_INT_FLAG_CH0) != RESET){
        if(ccnumber == 0U){
            readvalue1 = timer_channel_capture_value_register_read(TIMER2, TIMER_CH_0);
            ccnumber = 1U;
        }else{
            readvalue2 = timer_channel_capture_value_register_read(TIMER2, TIMER_CH_0);
            if(readvalue2 > readvalue1){
                count = readvalue2 - readvalue1;
            }else{
                count = (0xFFFFU - readvalue1) + readvalue2;
            }
            if(count != 0U){
                freq = 1000000U / count;
                freq_age_ms = 0U;
            }
            readvalue1 = 0U;
            readvalue2 = 0U;
            ccnumber = 0U;
        }
        timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_CH0);
    }
}

uint32_t get_freq_value(void)
{
    if(freq_age_ms > 500U){
        return 0U;
    }
    return freq;
}
