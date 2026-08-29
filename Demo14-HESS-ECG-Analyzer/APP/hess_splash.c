#include "hess_splash.h"

#include <stdint.h>

#include "mid_lcd.h"
#include "systick.h"

#define HESS_SCREEN_WIDTH 160U
#define HESS_SCREEN_HEIGHT 128U
#define HESS_HEART_CENTER_X 80
#define HESS_HEART_TOP_Y 5

typedef struct
{
    int8_t x;
    int8_t y;
} hess_point_t;

static const hess_point_t heart_outline[] =
{
    {  0,  8}, { -5,  2}, {-11,  0}, {-17,  3}, {-20,  9},
    {-19, 15}, {-15, 21}, { -8, 28}, {  0, 36}, {  8, 28},
    { 15, 21}, { 19, 15}, { 20,  9}, { 17,  3}, { 11,  0},
    {  5,  2}, {  0,  8}
};

static const hess_point_t ecg_trace[] =
{
    {-68, 18}, {-38, 18}, {-31, 16}, {-27, 20}, {-22, 18},
    {-15, 18}, {-10, 11}, { -5, 28}, {  1,  4}, {  7, 24},
    { 12, 18}, { 22, 18}, { 27, 15}, { 33, 18}, { 68, 18}
};

static void hess_draw_heart(void)
{
    uint16_t i;

    for (i = 1U; i < (uint16_t)(sizeof(heart_outline) / sizeof(heart_outline[0])); ++i)
    {
        TFT_DrawLine((uint16_t)(HESS_HEART_CENTER_X + heart_outline[i - 1U].x),
                     (uint16_t)(HESS_HEART_TOP_Y + heart_outline[i - 1U].y),
                     (uint16_t)(HESS_HEART_CENTER_X + heart_outline[i].x),
                     (uint16_t)(HESS_HEART_TOP_Y + heart_outline[i].y),
                     RED);
        delay_1ms(18U);
    }
}

static void hess_draw_ecg_trace(void)
{
    uint16_t i;

    for (i = 1U; i < (uint16_t)(sizeof(ecg_trace) / sizeof(ecg_trace[0])); ++i)
    {
        TFT_DrawLine((uint16_t)(HESS_HEART_CENTER_X + ecg_trace[i - 1U].x),
                     (uint16_t)(HESS_HEART_TOP_Y + ecg_trace[i - 1U].y),
                     (uint16_t)(HESS_HEART_CENTER_X + ecg_trace[i].x),
                     (uint16_t)(HESS_HEART_TOP_Y + ecg_trace[i].y),
                     CYAN);
        delay_1ms(14U);
    }
}

static void hess_show_title(void)
{
    static const uint8_t title[] = "HESS";
    static const uint8_t subtitle[] = "ECG ANALYZER";
    uint16_t i;

    for (i = 0U; i < 4U; ++i)
    {
        TFT_ShowChar((uint16_t)(56U + (i * 12U)), 51U, title[i],
                     WHITE, BLACK, 24U, 0U);
        delay_1ms(75U);
    }

    TFT_ShowString(32U, 80U, subtitle, LIGHTGREEN, BLACK, 16U, 0U);
}

static void hess_show_progress(void)
{
    uint16_t x;

    TFT_DrawLine(20U, 108U, 140U, 108U, DARKBLUE);
    for (x = 20U; x <= 140U; x += 8U)
    {
        uint16_t end_x = (uint16_t)(x + 6U);
        if (end_x > 140U)
        {
            end_x = 140U;
        }
        TFT_DrawLine(x, 108U, end_x, 108U, GREEN);
        delay_1ms(12U);
    }
}

void HESS_Splash_Show(void)
{
    TFT_Fill(0U, 0U, HESS_SCREEN_WIDTH, HESS_SCREEN_HEIGHT, BLACK);
    hess_draw_heart();
    hess_draw_ecg_trace();
    hess_show_title();
    hess_show_progress();
    delay_1ms(260U);
}
