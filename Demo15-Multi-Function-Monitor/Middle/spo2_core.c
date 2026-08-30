#include "spo2_core.h"

#include <stddef.h>

#define SPO2_ADC_MAX_VALUE          4095U
#define SPO2_RAIL_LOW_VALUE           16U
#define SPO2_RAIL_HIGH_VALUE        4079U
#define SPO2_MIN_DC_SCALED            32U
#define SPO2_MIN_AC_RMS_SCALED          2U
#define SPO2_MIN_RATIO_Q8              51U  /* R = 0.20 */
#define SPO2_MAX_RATIO_Q8             410U  /* R = 1.60 */
#define SPO2_MAX_RAIL_SAMPLES           50U  /* 5% of a paired window */
#define SPO2_UNBALANCED_TIMEOUT       2000U
#define SPO2_VREF_MV                  1210U
#define SPO2_FRONTEND_CENTER_MV       5000U
#define SPO2_INPUT_FULL_SCALE_MV      3300U

static uint32_t integer_sqrt(uint32_t value)
{
    uint32_t root = 0U;
    uint32_t bit = 1UL << 30;

    while(bit > value){
        bit >>= 2;
    }
    while(bit != 0U){
        if(value >= root + bit){
            value -= root + bit;
            root = (root >> 1) + bit;
        }else{
            root >>= 1;
        }
        bit >>= 2;
    }
    return root;
}

static void clear_window(spo2_core_t *core)
{
    core->sum_red = 0U;
    core->sum_ir = 0U;
    core->square_sum_red = 0U;
    core->square_sum_ir = 0U;
    core->count_red = 0U;
    core->count_ir = 0U;
    core->rail_count = 0U;
    core->window_age = 0U;
}

void SpO2Core_Init(spo2_core_t *core)
{
    if(core == NULL){
        return;
    }
    clear_window(core);
    core->latest.percent = 0U;
    core->latest.ratio_q8 = 0U;
    core->latest.valid = 0U;
    core->latest.updated = 0U;
}

uint16_t SpO2Core_ReconstructInputSample(uint16_t adc_sample,
                                        uint16_t vref_calibration)
{
    uint32_t adc_mv;
    uint32_t input_mv;

    if(vref_calibration == 0U){
        return 0U;
    }
    if(adc_sample > SPO2_ADC_MAX_VALUE){
        adc_sample = SPO2_ADC_MAX_VALUE;
    }
    adc_mv = ((uint32_t)adc_sample * SPO2_VREF_MV +
              (vref_calibration / 2U)) / vref_calibration;
    if((adc_mv * 2U) >= SPO2_FRONTEND_CENTER_MV){
        input_mv = 0U;
    }else{
        input_mv = SPO2_FRONTEND_CENTER_MV - (adc_mv * 2U);
    }
    if(input_mv > SPO2_INPUT_FULL_SCALE_MV){
        input_mv = SPO2_INPUT_FULL_SCALE_MV;
    }
    return (uint16_t)((input_mv * SPO2_ADC_MAX_VALUE +
                       (SPO2_INPUT_FULL_SCALE_MV / 2U)) /
                      SPO2_INPUT_FULL_SCALE_MV);
}

static spo2_core_result_t finish_window(spo2_core_t *core)
{
    uint32_t mean_red;
    uint32_t mean_ir;
    uint32_t variance_red;
    uint32_t variance_ir;
    uint32_t ac_red;
    uint32_t ac_ir;
    uint32_t pi_red_q15;
    uint32_t pi_ir_q15;
    uint32_t ratio_q8;
    int32_t percent;

    core->latest.updated = 1U;
    core->latest.valid = 0U;
    core->latest.percent = 0U;
    core->latest.ratio_q8 = 0U;

    mean_red = core->sum_red / SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL;
    mean_ir = core->sum_ir / SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL;
    if((mean_red < SPO2_MIN_DC_SCALED) ||
       (mean_ir < SPO2_MIN_DC_SCALED) ||
       (core->rail_count > SPO2_MAX_RAIL_SAMPLES)){
        clear_window(core);
        return core->latest;
    }

    variance_red = (core->square_sum_red /
                    SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL) -
                   (mean_red * mean_red);
    variance_ir = (core->square_sum_ir /
                   SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL) -
                  (mean_ir * mean_ir);
    ac_red = integer_sqrt(variance_red);
    ac_ir = integer_sqrt(variance_ir);
    if((ac_red < SPO2_MIN_AC_RMS_SCALED) ||
       (ac_ir < SPO2_MIN_AC_RMS_SCALED)){
        clear_window(core);
        return core->latest;
    }

    pi_red_q15 = (ac_red << 15U) / mean_red;
    pi_ir_q15 = (ac_ir << 15U) / mean_ir;
    if(pi_ir_q15 == 0U){
        clear_window(core);
        return core->latest;
    }
    ratio_q8 = (pi_red_q15 << 8U) / pi_ir_q15;
    if((ratio_q8 < SPO2_MIN_RATIO_Q8) ||
       (ratio_q8 > SPO2_MAX_RATIO_Q8)){
        clear_window(core);
        return core->latest;
    }

    /* TI SLAA655 teaching approximation: SpO2 = 110 - 25 * R.
     * Device-specific clinical accuracy requires empirical calibration. */
    percent = 110 - (int32_t)((25U * ratio_q8 + 128U) >> 8U);
    if(percent > 100){
        percent = 100;
    }else if(percent < 70){
        percent = 70;
    }
    core->latest.percent = (uint8_t)percent;
    core->latest.ratio_q8 = (uint16_t)ratio_q8;
    core->latest.valid = 1U;
    clear_window(core);
    return core->latest;
}

spo2_core_result_t SpO2Core_ProcessSample(spo2_core_t *core,
                                         uint16_t adc_sample,
                                         spo2_channel_t channel)
{
    uint32_t scaled;

    if(core == NULL){
        spo2_core_result_t invalid = {0U, 0U, 0U, 0U};
        return invalid;
    }
    core->latest.updated = 0U;
    if(adc_sample > SPO2_ADC_MAX_VALUE){
        adc_sample = SPO2_ADC_MAX_VALUE;
    }
    if(((adc_sample <= SPO2_RAIL_LOW_VALUE) ||
        (adc_sample >= SPO2_RAIL_HIGH_VALUE)) &&
       (core->rail_count < UINT16_MAX)){
        core->rail_count++;
    }
    if(core->window_age < UINT16_MAX){
        core->window_age++;
    }
    scaled = adc_sample >> 2U;

    if(channel == SPO2_CHANNEL_RED){
        if(core->count_red < SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL){
            core->sum_red += scaled;
            core->square_sum_red += scaled * scaled;
            core->count_red++;
        }
    }else if(core->count_ir < SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL){
        core->sum_ir += scaled;
        core->square_sum_ir += scaled * scaled;
        core->count_ir++;
    }

    if((core->count_red >= SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL) &&
       (core->count_ir >= SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL)){
        return finish_window(core);
    }
    if(core->window_age >= SPO2_UNBALANCED_TIMEOUT){
        core->latest.percent = 0U;
        core->latest.ratio_q8 = 0U;
        core->latest.valid = 0U;
        core->latest.updated = 1U;
        clear_window(core);
    }
    return core->latest;
}

spo2_core_result_t SpO2Core_GetResult(const spo2_core_t *core)
{
    if(core == NULL){
        spo2_core_result_t invalid = {0U, 0U, 0U, 0U};
        return invalid;
    }
    return core->latest;
}
