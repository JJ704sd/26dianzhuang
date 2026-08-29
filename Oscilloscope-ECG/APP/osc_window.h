#ifndef OSC_WINDOW_H
#define OSC_WINDOW_H

#include <stdint.h>

uint8_t osc_window_find(const int16_t *samples,
                        uint16_t count,
                        int16_t threshold,
                        uint16_t width,
                        uint16_t *start);

uint8_t osc_window_find_auto(const int16_t *samples,
                             uint16_t count,
                             uint16_t width,
                             uint16_t pretrigger,
                             uint16_t min_span,
                             uint16_t *start);

#endif
