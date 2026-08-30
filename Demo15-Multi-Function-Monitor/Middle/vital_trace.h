#ifndef VITAL_TRACE_H
#define VITAL_TRACE_H

#include <stdint.h>

typedef struct
{
    uint16_t width;
    uint16_t span_samples;
    uint16_t cursor_column;
    uint32_t column_phase;
    int8_t bucket_first;
    int8_t bucket_minimum;
    int8_t bucket_maximum;
    int8_t bucket_last;
    uint16_t bucket_count;
} vital_trace_state_t;

typedef struct
{
    uint16_t column;
    int8_t first;
    int8_t minimum;
    int8_t maximum;
    int8_t last;
    uint8_t connect_previous;
} vital_trace_column_t;

void VitalTrace_Init(vital_trace_state_t *state,
                     uint16_t width,
                     uint16_t span_samples);

uint8_t VitalTrace_PushSample(vital_trace_state_t *state,
                              int8_t sample,
                              vital_trace_column_t *column);

#endif
