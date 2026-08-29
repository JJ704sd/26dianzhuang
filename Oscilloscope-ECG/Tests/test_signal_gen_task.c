#include "signal_gen_task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hw_key.h"
#include "mid_lcd.h"
#include "mid_pwm.h"

static uint16_t stub_pwm_period = 777U;
static uint16_t stub_pwm_duty = 321U;
static uint8_t stub_pwm_state = PWM_OFF;
static unsigned int stub_out_of_bounds;
static unsigned int stub_saw_title;
static unsigned int stub_saw_frequency;
static unsigned int stub_saw_duty;

static void expect_long(const char *name, long actual, long expected)
{
    if (actual != expected)
    {
        fprintf(stderr, "FAIL: %s: got %ld, expected %ld\n",
                name, actual, expected);
        exit(EXIT_FAILURE);
    }
}

void set_pwm_duty(uint16_t duty)
{
    stub_pwm_duty = duty;
}

uint16_t get_pwm_duty(void)
{
    return stub_pwm_duty;
}

void set_pwm_period(uint16_t period)
{
    stub_pwm_period = period;
}

uint16_t get_pwm_period(void)
{
    return stub_pwm_period;
}

void set_pwm_state(uint8_t state)
{
    stub_pwm_state = state;
}

uint8_t get_pwm_state(void)
{
    return stub_pwm_state;
}

uint16_t get_pwm_out_freq(void)
{
    return (stub_pwm_period == 0U) ? 0U :
           (uint16_t)(1000000UL / stub_pwm_period);
}

void TFT_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend,
              uint16_t yend, uint16_t color)
{
    (void)color;
    if ((xsta >= 160U) || (xend > 160U) ||
        (ysta >= 128U) || (yend > 128U))
    {
        stub_out_of_bounds++;
    }
}

void TFT_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    (void)color;
    if ((x >= 160U) || (y >= 128U))
    {
        stub_out_of_bounds++;
    }
}

void TFT_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2,
                  uint16_t y2, uint16_t color)
{
    (void)color;
    if ((x1 >= 160U) || (x2 >= 160U) ||
        (y1 >= 128U) || (y2 >= 128U))
    {
        stub_out_of_bounds++;
    }
}

void TFT_ShowString(uint16_t x, uint16_t y, const uint8_t *text,
                    uint16_t foreground, uint16_t background,
                    uint8_t size, uint8_t mode)
{
    (void)foreground;
    (void)background;
    (void)size;
    (void)mode;
    if ((x >= 160U) || (y >= 128U))
    {
        stub_out_of_bounds++;
    }
    if (strcmp((const char *)text, "SIGNAL GEN") == 0)
    {
        stub_saw_title = 1U;
    }
    if (strncmp((const char *)text, "FREQ", 4U) == 0)
    {
        stub_saw_frequency = 1U;
    }
    if (strncmp((const char *)text, "DUTY", 4U) == 0)
    {
        stub_saw_duty = 1U;
    }
}

static void reset_stubs(void)
{
    stub_pwm_period = 777U;
    stub_pwm_duty = 321U;
    stub_pwm_state = PWM_OFF;
    stub_out_of_bounds = 0U;
    stub_saw_title = 0U;
    stub_saw_frequency = 0U;
    stub_saw_duty = 0U;
}

static void test_start_and_stop_restore_pwm_owner(void)
{
    reset_stubs();
    SignalGen_Init();
    SignalGen_Start();

    expect_long("active after start", SignalGen_IsActive(), 1);
    expect_long("default frequency", SignalGen_GetFrequencyHz(), 1000);
    expect_long("default duty", SignalGen_GetDutyPercent(), 50);
    expect_long("default period", stub_pwm_period, 1000);
    expect_long("default pulse", stub_pwm_duty, 500);
    expect_long("output enabled", stub_pwm_state, PWM_ON);

    SignalGen_Stop();
    expect_long("inactive after stop", SignalGen_IsActive(), 0);
    expect_long("restored period", stub_pwm_period, 777);
    expect_long("restored pulse", stub_pwm_duty, 321);
    expect_long("restored state", stub_pwm_state, PWM_OFF);
}

static void test_parameters_are_clamped_and_applied(void)
{
    reset_stubs();
    SignalGen_Init();
    SignalGen_Start();

    SignalGen_SetFrequencyHz(1U);
    expect_long("minimum frequency clamp", SignalGen_GetFrequencyHz(), 20);
    expect_long("minimum frequency period", stub_pwm_period, 50000);

    SignalGen_SetFrequencyHz(50000U);
    expect_long("maximum frequency clamp", SignalGen_GetFrequencyHz(), 20000);
    expect_long("maximum frequency period", stub_pwm_period, 50);

    SignalGen_SetDutyPercent(0U);
    expect_long("minimum duty clamp", SignalGen_GetDutyPercent(), 5);
    expect_long("minimum duty pulse", stub_pwm_duty, 3);

    SignalGen_SetDutyPercent(100U);
    expect_long("maximum duty clamp", SignalGen_GetDutyPercent(), 95);
    expect_long("maximum duty pulse", stub_pwm_duty, 48);
    SignalGen_Stop();
}

static void test_keys_and_encoder_adjust_output(void)
{
    reset_stubs();
    SignalGen_Init();
    SignalGen_Start();

    SignalGen_KeyHandle(KEY2_Pin, KeyPress);
    expect_long("K2 next frequency", SignalGen_GetFrequencyHz(), 2000);
    SignalGen_KeyHandle(KEY1_Pin, KeyPress);
    expect_long("K1 previous frequency", SignalGen_GetFrequencyHz(), 1000);
    SignalGen_KeyHandle(KEY3_Pin, KeyPress);
    expect_long("K3 duty increase", SignalGen_GetDutyPercent(), 55);
    SignalGen_KeyHandle(KEY3_Pin, KeyDoublePress);
    expect_long("K3 duty decrease", SignalGen_GetDutyPercent(), 50);

    SignalGen_Rotate(1);
    expect_long("encoder increases frequency", SignalGen_GetFrequencyHz(), 2000);
    SignalGen_Rotate(-1);
    expect_long("encoder decreases frequency", SignalGen_GetFrequencyHz(), 1000);

    SignalGen_KeyHandle(KEYD_Pin, KeyPress);
    expect_long("knob key disables output", stub_pwm_state, PWM_OFF);
    SignalGen_KeyHandle(KEYD_Pin, KeyPress);
    expect_long("knob key enables output", stub_pwm_state, PWM_ON);
    SignalGen_Stop();
}

static void test_ui_uses_english_labels_and_safe_coordinates(void)
{
    reset_stubs();
    SignalGen_Init();
    SignalGen_Start();
    SignalGen_StaticUI();
    SignalGen_ShowUI();

    expect_long("title drawn", stub_saw_title, 1);
    expect_long("frequency drawn", stub_saw_frequency, 1);
    expect_long("duty drawn", stub_saw_duty, 1);
    expect_long("coordinates stay on screen", stub_out_of_bounds, 0);
    SignalGen_Stop();
}

int main(void)
{
    test_start_and_stop_restore_pwm_owner();
    test_parameters_are_clamped_and_applied();
    test_keys_and_encoder_adjust_output();
    test_ui_uses_english_labels_and_safe_coordinates();
    puts("signal_gen_task tests passed");
    return EXIT_SUCCESS;
}
