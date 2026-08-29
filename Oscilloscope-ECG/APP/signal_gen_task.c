#include "signal_gen_task.h"

#include <stdio.h>

#include "hw_key.h"
#include "mid_lcd.h"
#include "mid_pwm.h"

#define SIGNAL_GEN_TIMER_HZ       1000000UL
#define SIGNAL_GEN_DEFAULT_HZ     1000UL
#define SIGNAL_GEN_DEFAULT_DUTY   50U
#define SIGNAL_GEN_DUTY_STEP      5U

typedef struct
{
    uint16_t period;
    uint16_t duty;
    uint8_t state;
} signal_gen_saved_pwm_t;

static const uint32_t frequency_presets_hz[] =
{
    20UL, 50UL, 100UL, 200UL, 500UL,
    1000UL, 2000UL, 5000UL, 10000UL, 20000UL
};

static signal_gen_saved_pwm_t saved_pwm;
static uint32_t frequency_hz = SIGNAL_GEN_DEFAULT_HZ;
static uint8_t duty_percent = SIGNAL_GEN_DEFAULT_DUTY;
static uint8_t output_enabled;
static uint8_t active;

static uint16_t signal_gen_period_ticks(uint32_t requested_hz)
{
    uint32_t ticks;

    ticks = (SIGNAL_GEN_TIMER_HZ + (requested_hz / 2UL)) / requested_hz;
    if (ticks < 2UL)
    {
        ticks = 2UL;
    }
    else if (ticks > 65535UL)
    {
        ticks = 65535UL;
    }
    return (uint16_t)ticks;
}

static uint16_t signal_gen_pulse_ticks(uint16_t period_ticks,
                                       uint8_t requested_duty)
{
    uint32_t pulse;

    pulse = ((uint32_t)period_ticks * requested_duty + 50UL) / 100UL;
    if (pulse == 0UL)
    {
        pulse = 1UL;
    }
    else if (pulse >= period_ticks)
    {
        pulse = (uint32_t)period_ticks - 1UL;
    }
    return (uint16_t)pulse;
}

static void signal_gen_apply(void)
{
    const uint16_t period_ticks = signal_gen_period_ticks(frequency_hz);
    const uint16_t pulse_ticks = signal_gen_pulse_ticks(period_ticks,
                                                        duty_percent);

    if (active == 0U)
    {
        return;
    }

    set_pwm_state(PWM_OFF);
    set_pwm_period(period_ticks);
    set_pwm_duty(pulse_ticks);
    set_pwm_state((output_enabled != 0U) ? PWM_ON : PWM_OFF);
}

static uint32_t signal_gen_next_frequency(uint32_t current_hz)
{
    uint8_t i;

    for (i = 0U; i < (uint8_t)(sizeof(frequency_presets_hz) /
                               sizeof(frequency_presets_hz[0])); ++i)
    {
        if (frequency_presets_hz[i] > current_hz)
        {
            return frequency_presets_hz[i];
        }
    }
    return SIGNAL_GEN_FREQUENCY_MAX_HZ;
}

static uint32_t signal_gen_previous_frequency(uint32_t current_hz)
{
    uint8_t i;
    uint32_t previous = SIGNAL_GEN_FREQUENCY_MIN_HZ;

    for (i = 0U; i < (uint8_t)(sizeof(frequency_presets_hz) /
                               sizeof(frequency_presets_hz[0])); ++i)
    {
        if (frequency_presets_hz[i] >= current_hz)
        {
            break;
        }
        previous = frequency_presets_hz[i];
    }
    return previous;
}

void SignalGen_Init(void)
{
    frequency_hz = SIGNAL_GEN_DEFAULT_HZ;
    duty_percent = SIGNAL_GEN_DEFAULT_DUTY;
    output_enabled = 0U;
    active = 0U;
}

void SignalGen_Start(void)
{
    if (active != 0U)
    {
        return;
    }

    saved_pwm.period = get_pwm_period();
    saved_pwm.duty = get_pwm_duty();
    saved_pwm.state = get_pwm_state();
    active = 1U;
    output_enabled = 1U;
    signal_gen_apply();
}

void SignalGen_Stop(void)
{
    if (active == 0U)
    {
        return;
    }

    active = 0U;
    output_enabled = 0U;
    set_pwm_state(PWM_OFF);
    set_pwm_period(saved_pwm.period);
    set_pwm_duty(saved_pwm.duty);
    set_pwm_state(saved_pwm.state);
}

void SignalGen_StaticUI(void)
{
    TFT_Fill(0U, 0U, 160U, 128U, BLACK);
    TFT_ShowString(2U, 2U, (const uint8_t *)"SIGNAL GEN",
                   WHITE, BLACK, 16U, 0U);
    TFT_ShowString(2U, 24U, (const uint8_t *)"PWM OUT: PA2",
                   GRAY, BLACK, 16U, 0U);
    TFT_ShowString(2U, 102U, (const uint8_t *)"K1-/K2+ K3:D",
                   GRAY, BLACK, 16U, 0U);
}

void SignalGen_ShowUI(void)
{
    char text[20];

    if (active == 0U)
    {
        return;
    }

    sprintf(text, "FREQ %5lu Hz ", (unsigned long)frequency_hz);
    TFT_ShowString(2U, 48U, (const uint8_t *)text,
                   CYAN, BLACK, 16U, 0U);
    sprintf(text, "DUTY %3u %%    ", (unsigned int)duty_percent);
    TFT_ShowString(2U, 68U, (const uint8_t *)text,
                   YELLOW, BLACK, 16U, 0U);
    TFT_ShowString(2U, 86U,
                   (const uint8_t *)((output_enabled != 0U) ?
                                     "OUT  ON " : "OUT  OFF"),
                   (output_enabled != 0U) ? GREEN : RED,
                   BLACK, 16U, 0U);
}

void SignalGen_KeyHandle(uint16_t key_pin, uint8_t key_state)
{
    if ((active == 0U) || (key_state == KEY_NoPress))
    {
        return;
    }

    if ((key_pin == KEY1_Pin) && (key_state == KeyPress))
    {
        SignalGen_SetFrequencyHz(signal_gen_previous_frequency(frequency_hz));
    }
    else if ((key_pin == KEY2_Pin) && (key_state == KeyPress))
    {
        SignalGen_SetFrequencyHz(signal_gen_next_frequency(frequency_hz));
    }
    else if (key_pin == KEY3_Pin)
    {
        if (key_state == KeyPress)
        {
            SignalGen_SetDutyPercent((uint8_t)(duty_percent +
                                               SIGNAL_GEN_DUTY_STEP));
        }
        else if (key_state == KeyDoublePress)
        {
            if (duty_percent > SIGNAL_GEN_DUTY_MIN_PERCENT)
            {
                SignalGen_SetDutyPercent((uint8_t)(duty_percent -
                                                   SIGNAL_GEN_DUTY_STEP));
            }
        }
    }
    else if ((key_pin == KEYD_Pin) && (key_state == KeyPress))
    {
        output_enabled = (output_enabled == 0U) ? 1U : 0U;
        set_pwm_state((output_enabled != 0U) ? PWM_ON : PWM_OFF);
    }
}

void SignalGen_Rotate(int8_t direction)
{
    if ((active == 0U) || (direction == 0))
    {
        return;
    }

    if (direction > 0)
    {
        SignalGen_SetFrequencyHz(signal_gen_next_frequency(frequency_hz));
    }
    else
    {
        SignalGen_SetFrequencyHz(signal_gen_previous_frequency(frequency_hz));
    }
}

void SignalGen_SetFrequencyHz(uint32_t requested_hz)
{
    if (requested_hz < SIGNAL_GEN_FREQUENCY_MIN_HZ)
    {
        requested_hz = SIGNAL_GEN_FREQUENCY_MIN_HZ;
    }
    else if (requested_hz > SIGNAL_GEN_FREQUENCY_MAX_HZ)
    {
        requested_hz = SIGNAL_GEN_FREQUENCY_MAX_HZ;
    }
    frequency_hz = requested_hz;
    signal_gen_apply();
}

uint32_t SignalGen_GetFrequencyHz(void)
{
    return frequency_hz;
}

void SignalGen_SetDutyPercent(uint8_t requested_duty)
{
    if (requested_duty < SIGNAL_GEN_DUTY_MIN_PERCENT)
    {
        requested_duty = SIGNAL_GEN_DUTY_MIN_PERCENT;
    }
    else if (requested_duty > SIGNAL_GEN_DUTY_MAX_PERCENT)
    {
        requested_duty = SIGNAL_GEN_DUTY_MAX_PERCENT;
    }
    duty_percent = requested_duty;
    signal_gen_apply();
}

uint8_t SignalGen_GetDutyPercent(void)
{
    return duty_percent;
}

uint8_t SignalGen_IsOutputEnabled(void)
{
    return output_enabled;
}

uint8_t SignalGen_IsActive(void)
{
    return active;
}
