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
#include "main.h"
#include "mid_adc.h"
#include "mid_lcd.h"
#include "mid_timer.h"
#include "osc_task.h"

#define OSC_UI_REFRESH_MS 500U

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
        key_scanf_handle(key->key_pin, key->key_state);
        key->key_state = KEY_NoPress;
    }
}

int main(void)
{
    uint16_t adc_vref_value;
    uint8_t step_value;

    systick_config();
    mx_gpio_init();
    mx_spi0_init();
    mx_adc_init();
    mx_tim2_init();
    mx_tim14_init();
    mx_tim15_init();

    led_handle[led1] = led_init(LED1_GPIO_Port, LED1_Pin, RESET);
    led_handle[led2] = led_init(LED2_GPIO_Port, LED2_Pin, RESET);
    key_handle[key1] = key_init(KEY1_GPIO_Port, KEY1_Pin, RESET);
    key_handle[key2] = key_init(KEY2_GPIO_Port, KEY2_Pin, RESET);
    key_handle[key3] = key_init(KEY3_GPIO_Port, KEY3_Pin, RESET);
    key_handle[keyd] = key_init(KEYD_GPIO_Port, KEYD_Pin, RESET);
    ec11_handle = ec11_init(KEYA_GPIO_Port, KEYA_Pin,
                            KEYB_GPIO_Port, KEYB_Pin);

    Set_ADC_Channel(ADC_CHANNEL_17);
    adc_vref_value = Get_ADC_Average(200U);

    delay_1ms(1000U);
    TFT_Init();
    delay_1ms(1000U);
    TFT_Init();
    TFT_Fill(0U, 0U, 160U, 128U, BLACK);
    TFT_StaticUI();

    Set_ADC_Channel(ADC_CHANNEL_3);
    clear_adc_value();
    Register_oscShowData();
    timer_enable(TIMER15);
    timer_enable(TIMER2);

    while (1)
    {
        if (get_key_timer_value() >= 10U)
        {
            scan_keys();
            set_key_timer_value(0U);
        }

        if (get_tft_timer_value() >= OSC_UI_REFRESH_MS)
        {
            set_key_bit_value(PAUSE_MS_TIMER);
            set_tft_bit_value(PAUSE_MS_TIMER);
            if (get_osc_stop_bit() == OSC_RUN)
            {
                osc_waveShow(adc_vref_value);
            }
            TFT_ShowUI();
            set_tft_timer_value(0U);
            set_key_bit_value(RUN_MS_TIMER);
            set_tft_bit_value(RUN_MS_TIMER);
        }

        dispatch_key(&key_handle[key1]);
        dispatch_key(&key_handle[key2]);
        dispatch_key(&key_handle[key3]);
        dispatch_key(&key_handle[keyd]);

        if (ec11_handle.ec11_direction != ec11_static)
        {
            step_value = get_step_value();
            if (ec11_handle.ec11_direction == ec11_forward)
            {
                if (step_value > 1U)
                {
                    step_value--;
                }
            }
            else if (step_value < 10U)
            {
                step_value++;
            }
            set_step_value(step_value);
            ec11_handle.ec11_direction = ec11_static;
        }
    }
}

void App_TimerTick1ms(void)
{
}

void EXTI4_15_IRQHandler(void)
{
    if (exti_interrupt_flag_get(EXTI_4) != RESET)
    {
        ec11_exti_callback(&ec11_handle);
    }
}
