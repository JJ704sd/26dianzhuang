#ifndef TEST_MID_LCD_H
#define TEST_MID_LCD_H

#include <stdint.h>

#define WHITE     0xFFFFU
#define BLACK     0x0000U
#define RED       0xF800U
#define GREEN     0x07E0U
#define CYAN      0x7FFFU
#define YELLOW    0xFFE0U
#define GRAY      0x8430U
#define DARKBLUE  0x01CFU
#define GRAYBLUE  0x5458U

void TFT_Fill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
              uint16_t color);
void TFT_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void TFT_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  uint16_t color);
void TFT_ShowString(uint16_t x, uint16_t y, const uint8_t *text,
                    uint16_t foreground, uint16_t background,
                    uint8_t size, uint8_t overlay);
void TFT_ShowChinese(uint16_t x, uint16_t y, uint8_t *text,
                     uint16_t foreground, uint16_t background,
                     uint8_t size, uint8_t overlay);

#endif
