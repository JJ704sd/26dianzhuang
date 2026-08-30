#include "signal_output.h"

#include "mid_pwm.h"

#define PWM_CLOCK_HZ             1000000UL
#define PWM_DAC_PERIOD_TICKS     59U
#define PWM_DAC_DUTY_MIN_TICKS   6U
#define PWM_DAC_DUTY_MAX_TICKS   53U
#define ECG_PRESET_COUNT         2U

static const uint16_t square_hz[] = {500U, 1000U, 2000U};
static const uint16_t sine_hz[] = {50U, 100U, 200U};
static const uint16_t ecg_bpm_presets[ECG_PRESET_COUNT] = {60U, 80U};
static const uint16_t ecg_period_ms[ECG_PRESET_COUNT] = {1000U, 750U};
static const uint8_t sine_lut[32] =
{
    128U,153U,177U,199U,218U,234U,245U,253U,
    255U,253U,245U,234U,218U,199U,177U,153U,
    128U,103U,79U,57U,38U,22U,11U,3U,
    0U,3U,11U,22U,38U,57U,79U,103U
};

static volatile signal_output_mode_t mode;
static volatile uint8_t enabled;
static uint8_t square_index;
static uint8_t sine_index;
static volatile uint8_t ecg_preset_index;
static uint32_t sine_phase_milli;
static volatile uint16_t ecg_phase_ms;

static uint16_t ecg_duty(uint16_t phase_ms, uint16_t period_ms)
{
    uint16_t phase = (uint16_t)(((uint32_t)phase_ms * 1000U) / period_ms);
    int16_t value = 0;

    if ((phase >= 80U) && (phase < 130U))
    {
        value = (int16_t)((phase - 80U) * 6U);
    }
    else if ((phase >= 130U) && (phase < 180U))
    {
        value = (int16_t)((180U - phase) * 6U);
    }
    else if ((phase >= 260U) && (phase < 285U))
    {
        value = -(int16_t)((phase - 260U) * 12U);
    }
    else if ((phase >= 285U) && (phase < 310U))
    {
        value = (int16_t)((phase - 285U) * 36U - 300);
    }
    else if ((phase >= 310U) && (phase < 345U))
    {
        value = (int16_t)(600 - (int16_t)((phase - 310U) * 22U));
    }
    else if ((phase >= 420U) && (phase < 540U))
    {
        value = (int16_t)((phase - 420U) * 2U);
    }
    else if ((phase >= 540U) && (phase < 660U))
    {
        value = (int16_t)((660U - phase) * 2U);
    }
    if (value < -500) { value = -500; }
    if (value > 500) { value = 500; }
    return (uint16_t)(PWM_DAC_DUTY_MIN_TICKS +
                      (((int32_t)value + 500) *
                       (PWM_DAC_DUTY_MAX_TICKS - PWM_DAC_DUTY_MIN_TICKS)) /
                      1000);
}

static void apply_mode(void)
{
    set_pwm_state(PWM_OFF);
    sine_phase_milli = 0U;
    ecg_phase_ms = 0U;
    if (mode == SIGNAL_OUTPUT_SQUARE)
    {
        const uint16_t period = (uint16_t)(PWM_CLOCK_HZ / square_hz[square_index]);
        set_pwm_period(period);
        set_pwm_duty((uint16_t)(period / 2U));
    }
    else
    {
        /* TIMER0 samples the ADC every 50 us. A 50-tick PWM carrier would be
         * phase-locked to that sampling interval, so averaging would see one
         * carrier phase instead of its duty. 59 is coprime with 50 and makes
         * the 80-conversion ECG decimator cover the complete PWM phase set. */
        set_pwm_period(PWM_DAC_PERIOD_TICKS);
        set_pwm_duty(PWM_DAC_PERIOD_TICKS / 2U);
    }
    if (enabled != 0U) { set_pwm_state(PWM_ON); }
}

void SignalOutput_Init(void)
{
    mode = SIGNAL_OUTPUT_SQUARE;
    enabled = 0U;
    square_index = 1U;
    sine_index = 1U;
    ecg_preset_index = 0U;
    apply_mode();
}

void SignalOutput_Tick1ms(void)
{
    if ((enabled == 0U) || (mode == SIGNAL_OUTPUT_SQUARE)) { return; }
    if (mode == SIGNAL_OUTPUT_SINE)
    {
        uint8_t index;
        sine_phase_milli += (uint32_t)sine_hz[sine_index] * 32U;
        if (sine_phase_milli >= 32000U) { sine_phase_milli -= 32000U; }
        index = (uint8_t)(sine_phase_milli / 1000U);
        set_pwm_duty((uint16_t)(PWM_DAC_DUTY_MIN_TICKS +
                     ((uint16_t)sine_lut[index] *
                      (PWM_DAC_DUTY_MAX_TICKS - PWM_DAC_DUTY_MIN_TICKS)) /
                     255U));
    }
    else
    {
        const uint8_t preset = ecg_preset_index;
        const uint16_t period_ms = ecg_period_ms[preset];
        set_pwm_duty(ecg_duty(ecg_phase_ms, period_ms));
        ecg_phase_ms++;
        if (ecg_phase_ms >= period_ms) { ecg_phase_ms = 0U; }
    }
}

void SignalOutput_SetEnabled(uint8_t value)
{
    enabled = (value != 0U) ? 1U : 0U;
    set_pwm_state((enabled != 0U) ? PWM_ON : PWM_OFF);
}

void SignalOutput_NextMode(void)
{
    mode = (mode == SIGNAL_OUTPUT_ECG) ? SIGNAL_OUTPUT_SQUARE :
           (signal_output_mode_t)(mode + 1);
    apply_mode();
}

void SignalOutput_Adjust(int8_t direction)
{
    if (direction == 0) { return; }
    if (mode == SIGNAL_OUTPUT_SQUARE)
    {
        square_index = (direction > 0) ? (uint8_t)((square_index + 1U) % 3U) :
                       ((square_index == 0U) ? 2U : (uint8_t)(square_index - 1U));
    }
    else if (mode == SIGNAL_OUTPUT_SINE)
    {
        sine_index = (direction > 0) ? (uint8_t)((sine_index + 1U) % 3U) :
                     ((sine_index == 0U) ? 2U : (uint8_t)(sine_index - 1U));
    }
    else
    {
        (void)direction;
        ecg_preset_index = (uint8_t)(1U - ecg_preset_index);
    }
    apply_mode();
}

void SignalOutput_SelectEcg(void)
{
    mode = SIGNAL_OUTPUT_ECG;
    apply_mode();
}

void SignalOutput_ToggleEcgPreset(void)
{
    ecg_preset_index = (uint8_t)(1U - ecg_preset_index);
    ecg_phase_ms = 0U;
    if (mode == SIGNAL_OUTPUT_ECG)
    {
        apply_mode();
    }
}

void SignalOutput_SetEcgBpm(uint16_t bpm)
{
    ecg_preset_index = (bpm == 80U) ? 1U : 0U;
    ecg_phase_ms = 0U;
    if (mode == SIGNAL_OUTPUT_ECG)
    {
        apply_mode();
    }
}

signal_output_mode_t SignalOutput_GetMode(void) { return mode; }
uint8_t SignalOutput_IsEnabled(void) { return enabled; }
uint16_t SignalOutput_GetValue(void)
{
    if (mode == SIGNAL_OUTPUT_SQUARE) { return square_hz[square_index]; }
    if (mode == SIGNAL_OUTPUT_SINE) { return sine_hz[sine_index]; }
    return ecg_bpm_presets[ecg_preset_index];
}
uint16_t SignalOutput_GetEcgPeriodMs(void)
{
    return ecg_period_ms[ecg_preset_index];
}
const char *SignalOutput_GetModeText(void)
{
    if (mode == SIGNAL_OUTPUT_SQUARE) { return "SQU"; }
    if (mode == SIGNAL_OUTPUT_SINE) { return "SIN"; }
    return "ECG";
}
