#include "gd32e23x.h"
#include "systick.h"

#include "hw_adc.h"
#include "hw_ec11.h"
#include "hw_gpio.h"
#include "hw_key.h"
#include "hw_lcdinit.h"
#include "hw_led.h"
#include "hw_spi.h"
#include "hw_tim.h"
#include "hess_splash.h"
#include "main.h"
#include "mid_adc.h"
#include "mid_lcd.h"
#include "mid_pwm.h"
#include "mid_timer.h"
#include "osc_task.h"
#include "signal_output.h"

#define MODE_SWITCH_HOLD_MS 2000U
#define RANGE_SWITCH_HOLD_MS 2000U
#define ECG_VIEW_HOLD_MS     2000U

enum led_instance
{
    led1 = 0,
    led2
};

enum key_instance
{
    key1 = 0,
    key2,
    key3,
    keyd
};

static struct led_class led_handle[2];
static struct key_class key_handle[4];
static struct ec11_class ec11_handle;
static volatile uint16_t key2_hold_ms;
static volatile uint16_t key2_hold_period;
static volatile uint16_t key2_hold_duty;
static volatile uint8_t key2_hold_raw_output;
static volatile uint8_t mode_switch_pending;
static volatile uint8_t key2_switch_latched;
static volatile uint16_t key1_hold_ms;
static volatile uint8_t range_switch_pending;
static volatile uint8_t key1_switch_latched;
static volatile uint16_t key3_hold_ms;
static volatile uint16_t key3_hold_period;
static volatile uint16_t key3_hold_duty;
static volatile uint16_t key3_hold_bpm;
static volatile uint8_t key3_hold_ecg_view;
static volatile uint8_t ecg_view_switch_pending;
static volatile uint8_t key3_switch_latched;

static void update_status_led(void)
{
    if(get_pwm_state() == PWM_ON){
        if(led_handle[led2].led_state != LED_ON){
            led_turn_on(&led_handle[led2]);
        }
    }else if(led_handle[led2].led_state != LED_OFF){
        led_turn_off(&led_handle[led2]);
    }
}

static void mode_switch_timer_callback(void)
{
    SignalOutput_Tick1ms();

    if(gpio_input_bit_get(KEY1_GPIO_Port, KEY1_Pin) == RESET){
        if(key1_hold_ms < RANGE_SWITCH_HOLD_MS){
            key1_hold_ms++;
        }
        if((key1_hold_ms >= RANGE_SWITCH_HOLD_MS) &&
           (Demo15_GetMode() == DEMO15_MODE_OSCILLOSCOPE) &&
           (Demo15_IsScopeEcgView() == 0U) &&
           (key1_switch_latched == 0U)){
            range_switch_pending = 1U;
            key1_switch_latched = 1U;
        }
    }else{
        key1_hold_ms = 0U;
        key1_switch_latched = 0U;
    }

    if(gpio_input_bit_get(KEY2_GPIO_Port, KEY2_Pin) == RESET){
        if(key2_hold_ms == 0U){
            key2_hold_period = get_pwm_period();
            key2_hold_duty = get_pwm_duty();
            key2_hold_raw_output =
                (uint8_t)((Demo15_GetMode() != DEMO15_MODE_ECG_MONITOR) &&
                          !((Demo15_GetMode() == DEMO15_MODE_OSCILLOSCOPE) &&
                            (Demo15_IsScopeEcgView() != 0U)));
        }
        if(key2_hold_ms < MODE_SWITCH_HOLD_MS){
            key2_hold_ms++;
        }
        if((key2_hold_ms >= MODE_SWITCH_HOLD_MS) &&
           (key2_switch_latched == 0U)){
            mode_switch_pending = 1U;
            key2_switch_latched = 1U;
        }
    }else{
        key2_hold_ms = 0U;
        key2_switch_latched = 0U;
    }

    if(gpio_input_bit_get(KEY3_GPIO_Port, KEY3_Pin) == RESET){
        if(key3_hold_ms == 0U){
            key3_hold_period = get_pwm_period();
            key3_hold_duty = get_pwm_duty();
            key3_hold_bpm = SignalOutput_GetValue();
            key3_hold_ecg_view = Demo15_IsScopeEcgView();
        }
        if(key3_hold_ms < ECG_VIEW_HOLD_MS){
            key3_hold_ms++;
        }
        if((key3_hold_ms >= ECG_VIEW_HOLD_MS) &&
           (Demo15_GetMode() == DEMO15_MODE_OSCILLOSCOPE) &&
           (key3_switch_latched == 0U)){
            ecg_view_switch_pending = 1U;
            key3_switch_latched = 1U;
        }
    }else{
        key3_hold_ms = 0U;
        key3_switch_latched = 0U;
    }
}

int main(void)
{
    uint16_t key_timer_value;
    uint16_t tft_timer_value;
    uint16_t adc_vref_value;
    uint8_t timebase_index;
    uint8_t osc_stop_bit;

    systick_config();

    mx_gpio_init();
    mx_spi0_init();
    mx_adc_init();
    mx_tim0_init();
    mx_tim2_init();
    mx_tim14_init();
    mx_tim15_init();

    led_handle[led1] = led_init(LED1_GPIO_Port, LED1_Pin, RESET);
    led_handle[led2] = led_init(LED2_GPIO_Port, LED2_Pin, RESET);
    led_turn_on(&led_handle[led1]);
    led_turn_off(&led_handle[led2]);

    key_handle[key1] = key_init(KEY1_GPIO_Port, KEY1_Pin, RESET);
    key_handle[key2] = key_init(KEY2_GPIO_Port, KEY2_Pin, RESET);
    key_handle[key3] = key_init(KEY3_GPIO_Port, KEY3_Pin, RESET);
    key_handle[keyd] = key_init(KEYD_GPIO_Port, KEYD_Pin, RESET);

    ec11_handle = ec11_init(KEYA_GPIO_Port, KEYA_Pin, KEYB_GPIO_Port, KEYB_Pin);

    Set_ADC_Channel(ADC_CHANNEL_17);
    adc_vref_value = Get_ADC_Average(200U);

    SignalOutput_Init();
    set_pwm_period(1000U);
    set_pwm_duty(500U);
    set_pwm_state(PWM_ON);

    delay_1ms(1000U);
    TFT_Init();
    delay_1ms(1000U);
    TFT_Init();

    HESS_Splash_Show();
    Set_ADC_Channel(ADC_CHANNEL_3);
    ECG_Init(adc_vref_value);
    ECG_AcquisitionStart();
    mid_timer_register_periodic_callback(mode_switch_timer_callback, 1U);

    TFT_StaticUI();
    timer_enable(TIMER15);
    timer_enable(TIMER0);
    timer_enable(TIMER2);

    while(1){
        key_timer_value = get_key_timer_value();
        tft_timer_value = get_tft_timer_value();

        if(mode_switch_pending != 0U){
            __disable_irq();
            mode_switch_pending = 0U;
            __enable_irq();
            /* A short press may already have been reported while the key was
             * held. Restore the PWM values captured at the press edge. */
            if(key2_hold_raw_output != 0U){
                set_pwm_period(key2_hold_period);
                set_pwm_duty(key2_hold_duty);
            }
            Demo15_SelectNextMode();
            key_handle[key2].key_state = KEY_NoPress;
        }

        if(range_switch_pending != 0U){
            __disable_irq();
            range_switch_pending = 0U;
            __enable_irq();
            if(Demo15_GetMode() == DEMO15_MODE_OSCILLOSCOPE){
                toggle_scope_small_signal();
            }
            /* Undo the PWM short-press side effect because this press was
             * ultimately used to cycle the scope vertical range. */
            if(get_pwm_state() == PWM_ON){
                set_pwm_state(PWM_OFF);
            }else{
                set_pwm_state(PWM_ON);
            }
            key_handle[key1].key_state = KEY_NoPress;
        }

        if(ecg_view_switch_pending != 0U){
            __disable_irq();
            ecg_view_switch_pending = 0U;
            if(key3_hold_ecg_view != 0U){
                SignalOutput_SetEcgBpm(key3_hold_bpm);
            }else{
                set_pwm_period(key3_hold_period);
                set_pwm_duty(key3_hold_duty);
            }
            __enable_irq();
            toggle_scope_ecg_view();
            key_handle[key3].key_state = KEY_NoPress;
        }

        if(key_timer_value >= 10U){
            key_scanf(&key_handle[key1]);
            key_scanf(&key_handle[key2]);
            key_scanf(&key_handle[key3]);
            key_scanf(&key_handle[keyd]);
            set_key_timer_value(0U);
        }

        if(tft_timer_value >= 80U){
            set_key_bit_value(PAUSE_MS_TIMER);
            set_tft_bit_value(PAUSE_MS_TIMER);
            osc_stop_bit = get_osc_stop_bit();
            if(osc_stop_bit == OSC_RUN){
                osc_waveShow(adc_vref_value);
            }
            TFT_ShowUI();
            set_tft_timer_value(0U);
            set_key_bit_value(RUN_MS_TIMER);
            set_tft_bit_value(RUN_MS_TIMER);
        }

        if(key_handle[key1].key_state != KEY_NoPress){
            key_scanf_handle(key_handle[key1].key_pin, key_handle[key1].key_state);
            key_handle[key1].key_state = KEY_NoPress;
        }
        if(key_handle[key2].key_state != KEY_NoPress){
            key_scanf_handle(key_handle[key2].key_pin, key_handle[key2].key_state);
            key_handle[key2].key_state = KEY_NoPress;
        }
        if(key_handle[key3].key_state != KEY_NoPress){
            key_scanf_handle(key_handle[key3].key_pin, key_handle[key3].key_state);
            key_handle[key3].key_state = KEY_NoPress;
        }
        if(key_handle[keyd].key_state != KEY_NoPress){
            key_scanf_handle(key_handle[keyd].key_pin, key_handle[keyd].key_state);
            key_handle[keyd].key_state = KEY_NoPress;
        }

        if(ec11_handle.ec11_direction != ec11_static){
            if((Demo15_GetMode() != DEMO15_MODE_OSCILLOSCOPE) ||
               (Demo15_IsScopeEcgView() != 0U)){
                timebase_index = get_ecg_timebase_index();
                if(ec11_handle.ec11_direction == ec11_forward){
                    if(timebase_index > 0U){
                        timebase_index--;
                    }
                }else if(timebase_index < (ECG_TIMEBASE_COUNT - 1U)){
                    timebase_index++;
                }
                set_ecg_timebase_index(timebase_index);
            }else{
                timebase_index = get_scope_timebase_index();
                if(ec11_handle.ec11_direction == ec11_forward){
                    if(timebase_index > 0U){
                        timebase_index--;
                    }
                }else if(timebase_index < (SCOPE_TIMEBASE_COUNT - 1U)){
                    timebase_index++;
                }
                set_scope_timebase_index(timebase_index);
            }
            ec11_handle.ec11_direction = ec11_static;
        }
        update_status_led();
    }
}

void EXTI4_15_IRQHandler(void)
{
    if(exti_interrupt_flag_get(EXTI_4) != RESET){
        ec11_exti_callback(&ec11_handle);
    }
}
