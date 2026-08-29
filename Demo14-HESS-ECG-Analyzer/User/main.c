#include "gd32e23x.h"
#include "systick.h"

#include "ecg_acq_task.h"
#include "hess_splash.h"
#include "hw_adc.h"
#include "hw_ec11.h"
#include "hw_gpio.h"
#include "hw_key.h"
#include "hw_lcdinit.h"
#include "hw_led.h"
#include "hw_spi.h"
#include "hw_tim.h"
#include "main.h"
#include "mid_adc.h"
#include "mid_lcd.h"
#include "mid_timer.h"

#define ECG_UI_REFRESH_MS 100U

enum led_instance { led1 = 0, led2 };
enum key_instance { key1 = 0, key2, key3, keyd };

static struct led_class led_handle[2];
static struct key_class key_handle[4];
static struct ec11_class ec11_handle;

static void scan_keys(void)
{
    key_scanf(&key_handle[key1]);
    key_scanf(&key_handle[key2]);
    key_scanf(&key_handle[key3]);
    key_scanf(&key_handle[keyd]);
}

static void dispatch_key(struct key_class *key)
{
    if (key->key_state != KEY_NoPress)
    {
        ECGAcq_KeyHandle((uint16_t)key->key_pin, key->key_state);
        key->key_state = KEY_NoPress;
    }
}

int main(void)
{
    systick_config();
    mx_gpio_init();
    mx_spi0_init();
    mx_adc_init();
    mx_tim2_init();
    mx_tim14_init();
    mx_tim15_init();
    Set_ADC_Channel(ADC_CHANNEL_3);

    led_handle[led1] = led_init(LED1_GPIO_Port, LED1_Pin, RESET);
    led_handle[led2] = led_init(LED2_GPIO_Port, LED2_Pin, RESET);
    key_handle[key1] = key_init(KEY1_GPIO_Port, KEY1_Pin, RESET);
    key_handle[key2] = key_init(KEY2_GPIO_Port, KEY2_Pin, RESET);
    key_handle[key3] = key_init(KEY3_GPIO_Port, KEY3_Pin, RESET);
    key_handle[keyd] = key_init(KEYD_GPIO_Port, KEYD_Pin, RESET);
    ec11_handle = ec11_init(KEYA_GPIO_Port, KEYA_Pin,
                            KEYB_GPIO_Port, KEYB_Pin);

    delay_1ms(1000U);
    TFT_Init();
    delay_1ms(1000U);
    TFT_Init();

    HESS_Splash_Show();
    ECGAcq_Init();
    ECGAcq_Start();
    ECGAcq_StaticUI();
    timer_enable(TIMER15);
    timer_enable(TIMER2);

    while (1)
    {
        if (get_key_timer_value() >= 10U)
        {
            scan_keys();
            set_key_timer_value(0U);
        }

        if (get_tft_timer_value() >= ECG_UI_REFRESH_MS)
        {
            set_tft_timer_value(0U);
            ECGAcq_ShowUI();
        }

        dispatch_key(&key_handle[key1]);
        dispatch_key(&key_handle[key2]);
        dispatch_key(&key_handle[key3]);
        dispatch_key(&key_handle[keyd]);

        if (ec11_handle.ec11_direction != ec11_static)
        {
            ECGAcq_Rotate((ec11_handle.ec11_direction == ec11_forward) ?
                          1 : -1);
            ec11_handle.ec11_direction = ec11_static;
        }

        if ((ECGAcq_GetQuality() == ECG_SIGNAL_POOR) ||
            (ECGAcq_GetQuality() == ECG_SIGNAL_LEAD_OFF) ||
            (ECGAcq_GetQuality() == ECG_SIGNAL_CLIPPED))
        {
            led_turn_on(&led_handle[led1]);
        }
        else
        {
            led_turn_off(&led_handle[led1]);
        }
        if (ECGAcq_IsRunning() != 0U)
        {
            led_turn_on(&led_handle[led2]);
        }
        else
        {
            led_turn_off(&led_handle[led2]);
        }
    }
}

void App_TimerTick1ms(void)
{
    ECGAcq_TimerTick1ms();
}

void EXTI4_15_IRQHandler(void)
{
    if (exti_interrupt_flag_get(EXTI_4) != RESET)
    {
        ec11_exti_callback(&ec11_handle);
    }
}
