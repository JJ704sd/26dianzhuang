#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

#define OSC_CAPTURE_COUNT 300U
#define OSC_DISPLAY_WIDTH 100U
#define OSC_PLOT_HEIGHT 50

struct Oscilloscope
{
    volatile uint8_t showbit;
    volatile uint8_t keyValue;
    uint8_t outputEnabled;
    uint8_t paused;
    uint16_t outputFreq;
    uint16_t pwmOut;
    uint32_t sampletime;
    uint32_t timerPeriod;
    volatile uint32_t gatherFreq;
    float vpp;
    float voltageValue[OSC_CAPTURE_COUNT];
};

#endif
