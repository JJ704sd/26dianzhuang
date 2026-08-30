#ifndef HESS_ANALYZER_H
#define HESS_ANALYZER_H

#include <stdint.h>

void HESSAnalyzer_Init(void);
void HESSAnalyzer_ProcessSample(uint16_t adc_sample);
void HESSAnalyzer_DrawStatic(void);
void HESSAnalyzer_ShowUI(uint8_t running, const char *timebase_label);
uint8_t HESSAnalyzer_HasNewPeak(void);
uint16_t HESSAnalyzer_GetBpm(void);
uint16_t HESSAnalyzer_GetRrMs(void);

#endif
