#include "gd32e23x.h"
#include "systick.h"
#include "led.h"
#include "tft.h"
#include "tft_init.h"
#include "adc.h"
#include "usart.h"
#include "timer.h"
#include "key.h"
#include "freq.h"
#include "scope_math.h"
#include "main.h"

struct Oscilloscope oscilloscope = {0};

static void Init_Oscilloscope(struct Oscilloscope *value);
static void Process_Captured_Frame(struct Oscilloscope *value);

int main(void)
{
    Init_Oscilloscope(&oscilloscope);
    systick_config();
    Init_LED_GPIO();

    delay_1ms(1000U);
    TFT_Init();
    TFT_Fill(0, 0, 160, 128, BLACK);

    Init_USART(115200U);
    ADC_DMA_Init();
    Init_ADC();
    Init_PWM_Output(oscilloscope.timerPeriod, oscilloscope.pwmOut);
    Init_EC11_GPIO();
    Init_Key_GPIO();
    Init_FreqTimer();
    TFT_StaticUI();
    TFT_ShowUI(&oscilloscope);

    while (1) {
        Key_Sacnf(&oscilloscope);
        KEYD_SCAN(&oscilloscope);
        Key_Handle(&oscilloscope);

        if ((oscilloscope.showbit != 0U) && (oscilloscope.paused == 0U)) {
            oscilloscope.showbit = 0U;
            Process_Captured_Frame(&oscilloscope);
            ADC_StartCapture();
            TFT_ShowUI(&oscilloscope);
        }
    }
}

static void Process_Captured_Frame(struct Oscilloscope *value)
{
    scope_frame_info_t frame;
    uint16_t i;

    for (i = 0U; i < OSC_CAPTURE_COUNT; ++i) {
        const float adc_voltage = ((float)Get_ADC_Value(i) * 3.3f) / 4095.0f;
        value->voltageValue[i] = 5.0f - (2.0f * adc_voltage);
    }

    if (scope_analyze_frame(value->voltageValue,
                            OSC_CAPTURE_COUNT,
                            OSC_DISPLAY_WIDTH,
                            &frame) == 0U) {
        value->vpp = 0.0f;
        return;
    }

    value->vpp = frame.peak_to_peak;
    for (i = frame.trigger_index;
         i < (uint16_t)(frame.trigger_index + OSC_DISPLAY_WIDTH);
         ++i) {
        const int16_t plot_value =
            scope_scale_to_plot(value->voltageValue[i],
                                frame.minimum,
                                frame.maximum,
                                OSC_PLOT_HEIGHT);
        drawCurve(80U, plot_value);
    }
}

static void Init_Oscilloscope(struct Oscilloscope *value)
{
    value->showbit = 0U;
    value->sampletime = ADC_SAMPLETIME_239POINT5;
    value->keyValue = 0U;
    value->outputEnabled = 0U;
    value->paused = 0U;
    value->gatherFreq = 0U;
    value->outputFreq = 1000U;
    value->pwmOut = 500U;
    value->timerPeriod = 1000U;
    value->vpp = 0.0f;
}
