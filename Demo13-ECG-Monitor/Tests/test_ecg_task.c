#include "ecg_task.h"

#include <stdio.h>
#include <stdlib.h>

#include "hw_key.h"
#include "mid_adc.h"
#include "mid_lcd.h"
#include "mid_pwm.h"

static uint16_t stub_adc_latest = 2048U;
static uint16_t stub_pwm_period = 777U;
static uint16_t stub_pwm_duty = 321U;
static uint8_t stub_pwm_state = PWM_OFF;
static unsigned int stub_duty_writes;
static unsigned int stub_grid_points;
static unsigned int stub_red_lines;
static unsigned int stub_draw_out_of_bounds;
static uint16_t stub_heart_clear_xend;

static void expect_int(const char *name, long actual, long expected)
{
    if (actual != expected)
    {
        fprintf(stderr, "FAIL: %s: got %ld, expected %ld\n",
                name, actual, expected);
        exit(EXIT_FAILURE);
    }
}

static void expect_range(const char *name, long actual, long minimum, long maximum)
{
    if ((actual < minimum) || (actual > maximum))
    {
        fprintf(stderr, "FAIL: %s: got %ld, expected %ld..%ld\n",
                name, actual, minimum, maximum);
        exit(EXIT_FAILURE);
    }
}

void set_pwm_duty(uint16_t duty)
{
    stub_pwm_duty = duty;
    stub_duty_writes++;
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

uint16_t Get_ADC_Latest(void)
{
    return stub_adc_latest;
}

void TFT_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend,
              uint16_t yend, uint16_t color)
{
    (void)yend; (void)color;
    if ((xsta == 27U) && (ysta == 3U))
    {
        stub_heart_clear_xend = xend;
    }
}

void TFT_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x >= 160U) || (y >= 128U))
    {
        stub_draw_out_of_bounds++;
    }
    if (color == DARKBLUE)
    {
        stub_grid_points++;
    }
}

void TFT_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2,
                  uint16_t y2, uint16_t color)
{
    if ((x1 >= 160U) || (x2 >= 160U) || (y1 >= 128U) || (y2 >= 128U))
    {
        stub_draw_out_of_bounds++;
    }
    if (color == RED)
    {
        stub_red_lines++;
    }
}

void TFT_ShowString(uint16_t x, uint16_t y, const uint8_t *text,
                    uint16_t foreground, uint16_t background,
                    uint8_t size, uint8_t mode)
{
    (void)x; (void)y; (void)text; (void)foreground; (void)background;
    (void)size; (void)mode;
}

static void reset_stubs(void)
{
    stub_adc_latest = 2048U;
    stub_pwm_period = 777U;
    stub_pwm_duty = 321U;
    stub_pwm_state = PWM_OFF;
    stub_duty_writes = 0U;
    stub_grid_points = 0U;
    stub_red_lines = 0U;
    stub_draw_out_of_bounds = 0U;
    stub_heart_clear_xend = 0U;
}

static void test_timer_boundary_and_pwm_restore(void)
{
    reset_stubs();
    ECG_Init();
    ECG_Start();

    expect_int("active after start", ECG_IsActive(), 1);
    expect_int("ECG PWM period", stub_pwm_period, 1000);
    expect_int("ECG PWM enabled", stub_pwm_state, PWM_ON);

    stub_duty_writes = 0U;
    ECG_TimerTick1ms();
    ECG_TimerTick1ms();
    ECG_TimerTick1ms();
    expect_int("no PWM sample before 4 ms", stub_duty_writes, 0);
    ECG_TimerTick1ms();
    expect_int("one PWM sample at 4 ms", stub_duty_writes, 1);

    ECG_Stop();
    expect_int("inactive after stop", ECG_IsActive(), 0);
    expect_int("restored period", stub_pwm_period, 777);
    expect_int("restored duty", stub_pwm_duty, 321);
    expect_int("restored state", stub_pwm_state, PWM_OFF);
}

static void test_adc_measurement_is_independent_from_output(void)
{
    unsigned int millisecond;
    unsigned int sample_index = 0U;

    reset_stubs();
    ECG_Init();
    ECG_Start();
    expect_int("default output BPM", ECG_GetOutputBpm(), 72);

    for (millisecond = 0U; millisecond < 6000U; ++millisecond)
    {
        if ((millisecond % 4U) == 0U)
        {
            const unsigned int phase = sample_index % 250U;
            stub_adc_latest = (phase < 5U) ? 3800U : 2048U;
            sample_index++;
        }
        ECG_TimerTick1ms();
    }

    expect_range("ADC-derived BPM", ECG_GetBpm(), 59, 61);
    expect_int("output BPM remains independent", ECG_GetOutputBpm(), 72);
    expect_int("periodic input has good signal quality", ECG_GetSignalQuality(),
               ECG_SIGNAL_GOOD);
    expect_int("normal rate has no alarm", ECG_GetAlarmFlags(), ECG_ALARM_NONE);
    ECG_Stop();
}

static void test_key_controls(void)
{
    reset_stubs();
    ECG_Init();
    ECG_Start();

    ECG_KeyHandle(KEY1_Pin, KeyPress);
    expect_int("K1 disables output", stub_pwm_state, PWM_OFF);
    ECG_KeyHandle(KEY1_Pin, KeyPress);
    expect_int("K1 enables output", stub_pwm_state, PWM_ON);

    ECG_KeyHandle(KEY2_Pin, KeyPress);
    expect_int("K2 increases output BPM", ECG_GetOutputBpm(), 78);
    ECG_KeyHandle(KEY2_Pin, KeyDoublePress);
    expect_int("K2 double press decreases output BPM", ECG_GetOutputBpm(), 72);

    ECG_SetDisplaySpan(0U);
    expect_int("display span lower clamp", ECG_GetDisplaySpan(), 1);
    ECG_SetDisplaySpan(255U);
    expect_int("display span upper clamp", ECG_GetDisplaySpan(), 4);

    while (ECG_GetOutputBpm() != 120U)
    {
        ECG_KeyHandle(KEY2_Pin, KeyPress);
    }
    ECG_KeyHandle(KEY2_Pin, KeyPress);
    expect_int("output BPM wraps above maximum", ECG_GetOutputBpm(), 60);
    ECG_KeyHandle(KEY2_Pin, KeyDoublePress);
    expect_int("output BPM wraps below minimum", ECG_GetOutputBpm(), 120);

    ECG_Stop();
}

static void test_ui_redraws_grid_and_heart_with_safe_coordinates(void)
{
    unsigned int tick;

    reset_stubs();
    ECG_Init();
    ECG_Start();
    ECG_StaticUI();
    expect_range("static ECG grid points", stub_grid_points, 1, 1000);

    stub_adc_latest = 2048U;
    for (tick = 0U; tick < 4U; ++tick)
    {
        ECG_TimerTick1ms();
    }
    stub_adc_latest = 3800U;
    for (tick = 0U; tick < 4U; ++tick)
    {
        ECG_TimerTick1ms();
    }

    stub_grid_points = 0U;
    stub_red_lines = 0U;
    ECG_ShowUI();
    expect_range("dynamic ECG grid redraw", stub_grid_points, 1, 1000);
    expect_int("expanded heart line count", stub_red_lines, 7);
    expect_int("heart clear avoids RUN label", stub_heart_clear_xend, 41);
    expect_int("ECG drawing stays on screen", stub_draw_out_of_bounds, 0);

    for (tick = 0U; tick < 200U; ++tick)
    {
        ECG_TimerTick1ms();
    }
    stub_red_lines = 0U;
    ECG_ShowUI();
    expect_int("resting heart line count", stub_red_lines, 3);
    ECG_Stop();
}

static void test_bpm_expires_when_input_stops(void)
{
    unsigned int millisecond;
    unsigned int sample_index = 0U;

    reset_stubs();
    ECG_Init();
    ECG_Start();
    for (millisecond = 0U; millisecond < 4000U; ++millisecond)
    {
        if ((millisecond % 4U) == 0U)
        {
            const unsigned int phase = sample_index % 250U;
            stub_adc_latest = (phase < 5U) ? 3800U : 2048U;
            sample_index++;
        }
        ECG_TimerTick1ms();
    }
    expect_range("BPM established before signal loss", ECG_GetBpm(), 59, 61);

    for (millisecond = 0U; millisecond < 3500U; ++millisecond)
    {
        stub_adc_latest = ((millisecond >= 2000U) && (millisecond < 2020U)) ?
                          3800U : 2048U;
        ECG_TimerTick1ms();
    }
    expect_int("invalid late peak does not preserve stale BPM", ECG_GetBpm(), 0);
    expect_int("BPM timeout activates signal alarm", ECG_GetAlarmFlags(),
               ECG_ALARM_SIGNAL_LOST);
    for (millisecond = 0U; millisecond < 1000U; ++millisecond)
    {
        stub_adc_latest = 2048U;
        ECG_TimerTick1ms();
    }
    expect_int("flat input reports signal loss", ECG_GetSignalQuality(),
               ECG_SIGNAL_LOST);
    ECG_Stop();
}

int main(void)
{
    test_timer_boundary_and_pwm_restore();
    test_adc_measurement_is_independent_from_output();
    test_key_controls();
    test_bpm_expires_when_input_stops();
    test_ui_redraws_grid_and_heart_with_safe_coordinates();
    puts("ECG task integration tests passed.");
    return EXIT_SUCCESS;
}
