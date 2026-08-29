#include "freq.h"
#include "main.h"
#include "timer_math.h"

#define FREQ_TIMER_HZ 1000000U
#define FREQ_COUNTER_MAX 0xFFFFU
#define FREQ_TIMEOUT_WRAPS 4U

static volatile uint8_t capture_active = 0U;
static volatile uint16_t first_capture = 0U;
static volatile uint16_t overflow_count = 0U;

extern struct Oscilloscope oscilloscope;

void Init_FreqTimer(void)
{
    timer_ic_parameter_struct timer_icinitpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_TIMER2);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
    gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_6);
    nvic_irq_enable(TIMER2_IRQn, 2U);

    timer_deinit(TIMER2);
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler = 71U;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = FREQ_COUNTER_MAX;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_init(TIMER2, &timer_initpara);

    timer_channel_input_struct_para_init(&timer_icinitpara);
    timer_icinitpara.icpolarity = TIMER_IC_POLARITY_RISING;
    timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
    timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV1;
    timer_icinitpara.icfilter = 0x00U;
    timer_input_capture_config(TIMER2, TIMER_CH_0, &timer_icinitpara);

    timer_auto_reload_shadow_enable(TIMER2);
    timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_CH0);
    timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TIMER2, TIMER_INT_CH0);
    timer_interrupt_enable(TIMER2, TIMER_INT_UP);
    timer_enable(TIMER2);
}

void TIMER2_IRQHandler(void)
{
    const uint8_t capture_pending =
        (timer_interrupt_flag_get(TIMER2, TIMER_INT_FLAG_CH0) == SET) ? 1U : 0U;
    const uint8_t update_pending =
        (timer_interrupt_flag_get(TIMER2, TIMER_INT_FLAG_UP) == SET) ? 1U : 0U;
    uint16_t captured = 0U;

    if (capture_pending != 0U) {
        captured = timer_channel_capture_value_register_read(TIMER2, TIMER_CH_0);
    }

    if (update_pending != 0U) {
        if ((capture_active != 0U) &&
            ((capture_pending == 0U) ||
             (timer_pending_wrap_precedes_capture(captured, FREQ_COUNTER_MAX) != 0U))) {
            overflow_count++;
            if (overflow_count >= FREQ_TIMEOUT_WRAPS) {
                capture_active = 0U;
                overflow_count = 0U;
                oscilloscope.gatherFreq = 0U;
            }
        }
        timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_UP);
    }

    if (capture_pending != 0U) {
        if (capture_active == 0U) {
            first_capture = captured;
            overflow_count = 0U;
            capture_active = 1U;
        } else {
            const uint32_t elapsed =
                timer_capture_elapsed(first_capture, captured,
                                      overflow_count, FREQ_COUNTER_MAX);
            oscilloscope.gatherFreq =
                (elapsed == 0U) ? 0U : (FREQ_TIMER_HZ / elapsed);
            capture_active = 0U;
            overflow_count = 0U;
        }
        timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_CH0);
    }
}
