#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#include "mid_font.h"
#pragma GCC diagnostic pop

#define CHECK(condition)                                                        \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                       \
        {                                                                       \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__,          \
                    #condition);                                                \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    } while (0)

static const typFNT_GB12 *find_glyph12(uint8_t high, uint8_t low)
{
    size_t i;

    for (i = 0U; i < (sizeof(tfont12) / sizeof(tfont12[0])); ++i)
    {
        if ((tfont12[i].Index[0] == high) && (tfont12[i].Index[1] == low))
        {
            return &tfont12[i];
        }
    }
    return NULL;
}

static const typFNT_GB16 *find_glyph16(uint8_t high, uint8_t low)
{
    size_t i;

    for (i = 0U; i < (sizeof(tfont16) / sizeof(tfont16[0])); ++i)
    {
        if ((tfont16[i].Index[0] == high) && (tfont16[i].Index[1] == low))
        {
            return &tfont16[i];
        }
    }
    return NULL;
}

static uint8_t row_pixel_count16(const typFNT_GB16 *glyph, uint8_t row)
{
    uint8_t byte_index;
    uint8_t bit;
    uint8_t count = 0U;

    for (byte_index = 0U; byte_index < 2U; ++byte_index)
    {
        uint8_t value = glyph->Msk[(uint8_t)(row * 2U + byte_index)];
        for (bit = 0U; bit < 8U; ++bit)
        {
            if ((value & (uint8_t)(1U << bit)) != 0U)
            {
                count++;
            }
        }
    }
    return count;
}

static void test_all_ui_glyphs_exist(void)
{
    static const uint8_t required12[][2] = {
        {0xCAU,0xE4U}, {0xC8U,0xEBU}, {0xD7U,0xB4U}, {0xCCU,0xACU},
        {0xB7U,0xF9U}, {0xD6U,0xB5U}, {0xC6U,0xB5U}, {0xC2U,0xCAU},
        {0xB3U,0xF6U}
    };
    static const uint8_t required16[][2] = {
        {0xCAU,0xBEU}, {0xB2U,0xA8U}, {0xBFU,0xAAU}, {0xB9U,0xD8U}
    };
    size_t i;

    for (i = 0U; i < (sizeof(required12) / sizeof(required12[0])); ++i)
    {
        CHECK(find_glyph12(required12[i][0], required12[i][1]) != NULL);
    }
    for (i = 0U; i < (sizeof(required16) / sizeof(required16[0])); ++i)
    {
        CHECK(find_glyph16(required16[i][0], required16[i][1]) != NULL);
    }
}

static void test_show_glyph_has_two_readable_horizontal_strokes(void)
{
    const typFNT_GB16 *glyph = find_glyph16(0xCAU, 0xBEU);

    CHECK(glyph != NULL);
    CHECK(row_pixel_count16(glyph, 1U) >= 6U);
    CHECK(row_pixel_count16(glyph, 3U) >= 12U);
}

int main(void)
{
    test_all_ui_glyphs_exist();
    test_show_glyph_has_two_readable_horizontal_strokes();
    puts("Chinese UI font tests passed");
    return EXIT_SUCCESS;
}
