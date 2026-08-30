#include "spo2_receiver.h"

#include <stddef.h>

#define SPO2_DUTY_WINDOW_SAMPLES  SPO2_CORE_WINDOW_SAMPLES_PER_CHANNEL
#define SPO2_DUTY_ADC_MAX_VALUE   4095U
#define SPO2_DUTY_RAIL_LOW        16U
#define SPO2_DUTY_RAIL_HIGH       4079U
#define SPO2_DUTY_MIN_SPAN        32U
#define SPO2_DUTY_MAX_RAIL_COUNT   0U
#define SPO2_DUAL_MAX_TAG_DWELL   25U

static void clear_duty_window(spo2_receiver_t *receiver)
{
    receiver->duty_sum = 0U;
    receiver->duty_minimum = SPO2_DUTY_ADC_MAX_VALUE;
    receiver->duty_maximum = 0U;
    receiver->duty_count = 0U;
    receiver->duty_rail_count = 0U;
}

static spo2_core_result_t process_duty_sample(spo2_receiver_t *receiver,
                                              uint16_t raw_adc_sample)
{
    spo2_core_result_t result = receiver->duty_latest;
    uint32_t denominator;
    uint32_t high_area;
    uint32_t duty_percent;
    uint16_t span;

    result.updated = 0U;
    if(raw_adc_sample > SPO2_DUTY_ADC_MAX_VALUE){
        raw_adc_sample = SPO2_DUTY_ADC_MAX_VALUE;
    }
    receiver->duty_sum += raw_adc_sample;
    if(raw_adc_sample < receiver->duty_minimum){
        receiver->duty_minimum = raw_adc_sample;
    }
    if(raw_adc_sample > receiver->duty_maximum){
        receiver->duty_maximum = raw_adc_sample;
    }
    if((raw_adc_sample <= SPO2_DUTY_RAIL_LOW) ||
       (raw_adc_sample >= SPO2_DUTY_RAIL_HIGH)){
        receiver->duty_rail_count++;
    }
    receiver->duty_count++;
    if(receiver->duty_count < SPO2_DUTY_WINDOW_SAMPLES){
        return result;
    }

    result.percent = 0U;
    result.ratio_q8 = 0U;
    result.valid = 0U;
    result.updated = 1U;
    span = (uint16_t)(receiver->duty_maximum - receiver->duty_minimum);
    if((span >= SPO2_DUTY_MIN_SPAN) &&
       (receiver->duty_rail_count <= SPO2_DUTY_MAX_RAIL_COUNT)){
        /* The analog front end is inverting. The normalized area below the
         * ADC-code maximum therefore equals the external signal's high-time
         * duty for a two-level waveform, without storing a 500-sample frame. */
        denominator = (uint32_t)span * receiver->duty_count;
        high_area = (uint32_t)receiver->duty_maximum * receiver->duty_count -
                    receiver->duty_sum;
        duty_percent = (high_area * 100U + (denominator / 2U)) /
                       denominator;

        /* Explicit laboratory encoding for an observed two-level window:
         * duty maps to the 70-100% simulated display range. */
        result.percent = (uint8_t)(70U +
                         ((30U * duty_percent + 50U) / 100U));
        result.valid = 1U;
    }
    receiver->duty_latest = result;
    clear_duty_window(receiver);
    return result;
}

static uint16_t prepare_dual_sample(uint16_t raw_adc_sample,
                                    uint16_t reconstructed_sample)
{
    /* Core rail counters must describe the ADC, not a legitimate 0 V or
     * full-scale value after analog-front-end reconstruction. */
    if(raw_adc_sample <= SPO2_DUTY_RAIL_LOW){
        return 0U;
    }
    if(raw_adc_sample >= SPO2_DUTY_RAIL_HIGH){
        return SPO2_DUTY_ADC_MAX_VALUE;
    }
    if(reconstructed_sample <= SPO2_DUTY_RAIL_LOW){
        return SPO2_DUTY_RAIL_LOW + 1U;
    }
    if(reconstructed_sample >= SPO2_DUTY_RAIL_HIGH){
        return SPO2_DUTY_RAIL_HIGH - 1U;
    }
    return reconstructed_sample;
}

static void discard_stale_unpaired_window(spo2_receiver_t *receiver,
                                          uint8_t wavelength_tag)
{
    const uint8_t red_is_stale = (uint8_t)(
        (wavelength_tag != 0U) && (receiver->tag_mask == 0x01U) &&
        (receiver->dual_core.count_ir == 0U) &&
        (receiver->dual_core.count_red > SPO2_DUAL_MAX_TAG_DWELL));
    const uint8_t ir_is_stale = (uint8_t)(
        (wavelength_tag == 0U) && (receiver->tag_mask == 0x02U) &&
        (receiver->dual_core.count_red == 0U) &&
        (receiver->dual_core.count_ir > SPO2_DUAL_MAX_TAG_DWELL));

    if((red_is_stale != 0U) || (ir_is_stale != 0U)){
        SpO2Core_Init(&receiver->dual_core);
        receiver->tag_mask = 0U;
    }
}

void SpO2Receiver_Init(spo2_receiver_t *receiver)
{
    if(receiver == NULL){
        return;
    }
    SpO2Core_Init(&receiver->dual_core);
    receiver->duty_latest.percent = 0U;
    receiver->duty_latest.ratio_q8 = 0U;
    receiver->duty_latest.valid = 0U;
    receiver->duty_latest.updated = 0U;
    receiver->latest.percent = 0U;
    receiver->latest.ratio_q8 = 0U;
    receiver->latest.valid = 0U;
    receiver->latest.updated = 0U;
    receiver->tag_mask = 0U;
    receiver->source = SPO2_SOURCE_WAITING;
    clear_duty_window(receiver);
}

spo2_core_result_t SpO2Receiver_ProcessSample(spo2_receiver_t *receiver,
                                              uint16_t raw_adc_sample,
                                              uint16_t reconstructed_sample,
                                              uint8_t wavelength_tag)
{
    spo2_core_result_t duty_result;
    spo2_core_result_t dual_result;
    uint16_t dual_sample;

    if(receiver == NULL){
        const spo2_core_result_t invalid = {0U, 0U, 0U, 0U};
        return invalid;
    }

    duty_result = process_duty_sample(receiver, raw_adc_sample);
    dual_sample = prepare_dual_sample(raw_adc_sample, reconstructed_sample);
    discard_stale_unpaired_window(receiver, wavelength_tag);

    if(wavelength_tag == 0U){
        receiver->tag_mask |= 0x01U;
        dual_result = SpO2Core_ProcessSample(&receiver->dual_core,
                                             dual_sample,
                                             SPO2_CHANNEL_RED);
    }else{
        receiver->tag_mask |= 0x02U;
        dual_result = SpO2Core_ProcessSample(&receiver->dual_core,
                                             dual_sample,
                                             SPO2_CHANNEL_IR);
    }

    if(dual_result.updated != 0U){
        if((receiver->tag_mask == 0x03U) && (dual_result.valid != 0U)){
            receiver->latest = dual_result;
            receiver->source = SPO2_SOURCE_DUAL_CHANNEL;
        }else if(receiver->duty_latest.valid != 0U){
            receiver->latest = receiver->duty_latest;
            receiver->source = SPO2_SOURCE_DUTY_CODED;
        }else{
            receiver->latest = duty_result;
            receiver->latest.valid = 0U;
            receiver->latest.updated = 1U;
            receiver->source = SPO2_SOURCE_WAITING;
        }
        receiver->tag_mask = 0U;
    }else if((receiver->source != SPO2_SOURCE_DUAL_CHANNEL) &&
             (duty_result.updated != 0U)){
        receiver->latest = duty_result;
        receiver->source = (duty_result.valid != 0U) ?
                           SPO2_SOURCE_DUTY_CODED : SPO2_SOURCE_WAITING;
    }else{
        receiver->latest.updated = 0U;
    }
    if((duty_result.updated != 0U) &&
       (receiver->source == SPO2_SOURCE_DUTY_CODED) &&
       (receiver->tag_mask != 0U) && (receiver->tag_mask != 0x03U)){
        /* A completed DUT generation is not half of a future RED/IR pair. */
        SpO2Core_Init(&receiver->dual_core);
        receiver->tag_mask = 0U;
    }
    return receiver->latest;
}

spo2_core_result_t SpO2Receiver_GetResult(const spo2_receiver_t *receiver)
{
    if(receiver == NULL){
        const spo2_core_result_t invalid = {0U, 0U, 0U, 0U};
        return invalid;
    }
    return receiver->latest;
}

spo2_source_t SpO2Receiver_GetSource(const spo2_receiver_t *receiver)
{
    if(receiver == NULL){
        return SPO2_SOURCE_WAITING;
    }
    return receiver->source;
}
