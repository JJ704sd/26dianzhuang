#include "vital_trace.h"

#include <stddef.h>

void VitalTrace_Init(vital_trace_state_t *state,
                     uint16_t width,
                     uint16_t span_samples)
{
    if(state == NULL){
        return;
    }
    state->width = width;
    state->span_samples = span_samples;
    state->cursor_column = 0U;
    state->column_phase = 0U;
    state->bucket_first = 0;
    state->bucket_minimum = 0;
    state->bucket_maximum = 0;
    state->bucket_last = 0;
    state->bucket_count = 0U;
}

uint8_t VitalTrace_PushSample(vital_trace_state_t *state,
                              int8_t sample,
                              vital_trace_column_t *column)
{
    if((state == NULL) || (column == NULL) ||
       (state->width == 0U) || (state->span_samples == 0U)){
        return 0U;
    }

    if(state->bucket_count == 0U){
        state->bucket_first = sample;
        state->bucket_minimum = sample;
        state->bucket_maximum = sample;
    }else{
        if(sample < state->bucket_minimum){
            state->bucket_minimum = sample;
        }
        if(sample > state->bucket_maximum){
            state->bucket_maximum = sample;
        }
    }
    state->bucket_last = sample;
    state->bucket_count++;
    state->column_phase += state->width;

    if(state->column_phase < state->span_samples){
        return 0U;
    }
    state->column_phase -= state->span_samples;

    column->column = state->cursor_column;
    column->first = state->bucket_first;
    column->minimum = state->bucket_minimum;
    column->maximum = state->bucket_maximum;
    column->last = state->bucket_last;
    column->connect_previous = (uint8_t)(state->cursor_column != 0U);

    state->cursor_column++;
    if(state->cursor_column >= state->width){
        state->cursor_column = 0U;
    }
    state->bucket_count = 0U;
    return 1U;
}
