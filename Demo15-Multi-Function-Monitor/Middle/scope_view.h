#ifndef SCOPE_VIEW_H
#define SCOPE_VIEW_H

#include <stdint.h>

typedef enum
{
    SCOPE_VIEW_FREE = 0,
    SCOPE_VIEW_RISING,
    SCOPE_VIEW_FALLING
} scope_view_mode_t;

typedef struct
{
    uint16_t minimum;
    uint16_t maximum;
    uint16_t source_count;
    uint16_t trigger_index;
    uint8_t trigger_found;
} scope_view_info_t;

uint16_t ScopeView_CopyWindow(const uint16_t *history,
                              uint16_t history_size,
                              uint16_t history_count,
                              uint16_t write_index,
                              uint16_t span_samples,
                              uint16_t free_phase,
                              scope_view_mode_t mode,
                              uint16_t *output,
                              uint16_t output_capacity,
                              scope_view_info_t *info);

uint16_t ScopeView_CopyPacked12Window(const uint8_t *history,
                                      uint16_t history_size,
                                      uint16_t history_count,
                                      uint16_t write_index,
                                      uint16_t span_samples,
                                      uint16_t free_phase,
                                      scope_view_mode_t mode,
                                      uint16_t *output,
                                      uint16_t output_capacity,
                                      scope_view_info_t *info);

#endif
