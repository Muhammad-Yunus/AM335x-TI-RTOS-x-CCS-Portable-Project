/**
 * @file    simple_gfx.c
 * @brief   Simple graphics demo functions for ILI9341 LCD
 */

#include "simple_gfx.h"
#include "ili9341/ili9341.h"
#include "utils.h"

/* ============================================================================
 * Constants
 * ============================================================================ */

static const uint16_t COLOR_BAND[] = {
    ILI9341_COLOR_RED,    ILI9341_COLOR_ORANGE, ILI9341_COLOR_YELLOW,
    ILI9341_COLOR_GREEN,  ILI9341_COLOR_CYAN,   ILI9341_COLOR_BLUE,
    ILI9341_COLOR_MAGENTA,ILI9341_COLOR_WHITE
};
#define COLOR_BAND_COUNT (sizeof(COLOR_BAND) / sizeof(COLOR_BAND[0]))

/* ============================================================================
 * Static Helpers
 * ============================================================================ */

static void Demo_ClearAndHeader(const char *title, uint16_t bg)
{
    ILI9341_FillScreen(bg);
    ILI9341_DrawString(10, 10, title, ILI9341_COLOR_WHITE, bg);
    ILI9341_DrawString(10, 30, "AM3352 SPI1 + ILI9341",
                       ILI9341_COLOR_YELLOW, bg);
}

/* ============================================================================
 * Function Definitions
 * ============================================================================ */

void simple_gfx_demo_color_bands(void)
{
    uint32_t i;
    uint16_t band_h;
    uint16_t y;

    Demo_ClearAndHeader("Color bands (8x40 px)", ILI9341_COLOR_BLACK);
    band_h = (ILI9341_HEIGHT - 50) / COLOR_BAND_COUNT;

    for (i = 0; i < COLOR_BAND_COUNT; i++) {
        y = (uint16_t)(50 + i * band_h);
        ILI9341_FillRect(0, y, ILI9341_WIDTH, band_h, COLOR_BAND[i]);
    }
    delay(1500);
}

void simple_gfx_demo_shapes(void)
{
    Demo_ClearAndHeader("Shapes", ILI9341_COLOR_BLACK);

    ILI9341_DrawRect(20, 60,  90, 60, ILI9341_COLOR_WHITE);
    ILI9341_FillRect(130, 60, 90, 60, ILI9341_COLOR_RED);

    ILI9341_DrawCircle(80,  180, 35, ILI9341_COLOR_CYAN);
    ILI9341_FillCircle(190, 180, 35, ILI9341_COLOR_GREEN);

    ILI9341_DrawLine(230, 60,  310, 200, ILI9341_COLOR_YELLOW);
    ILI9341_DrawLine(230, 200, 310, 60,  ILI9341_COLOR_MAGENTA);

    delay(1500);
}

void simple_gfx_demo_text(void)
{
    Demo_ClearAndHeader("Text", ILI9341_COLOR_BLUE);

    ILI9341_DrawString(10,  60,  "Hello AM3352!",  ILI9341_COLOR_WHITE,
                       ILI9341_COLOR_BLUE);
    ILI9341_DrawString(10,  80,  "ILI9341 240x320", ILI9341_COLOR_YELLOW,
                       ILI9341_COLOR_BLUE);
    ILI9341_DrawString(10, 100,  "SPI1 @ DMA mode",   ILI9341_COLOR_GREEN,
                       ILI9341_COLOR_BLUE);

    ILI9341_DrawString(10, 140, "ABCDEFGHIJKLMN",
                       ILI9341_COLOR_WHITE, ILI9341_COLOR_BLUE);
    ILI9341_DrawString(10, 160, "OPQRSTUVWXYZ",
                       ILI9341_COLOR_WHITE, ILI9341_COLOR_BLUE);
    ILI9341_DrawString(10, 180, "abcdefghijklmn",
                       ILI9341_COLOR_CYAN, ILI9341_COLOR_BLUE);
    ILI9341_DrawString(10, 200, "opqrstuvwxyz",
                       ILI9341_COLOR_CYAN, ILI9341_COLOR_BLUE);
    ILI9341_DrawString(10, 220, "0123456789 !?.,:",
                       ILI9341_COLOR_WHITE, ILI9341_COLOR_BLUE);

    delay(1500);
}

void simple_gfx_demo_pixel_grid(void)
{
    uint16_t y;
    uint16_t x;

    Demo_ClearAndHeader("Pixel art border", ILI9341_COLOR_BLACK);

    for (y = 50; y < ILI9341_HEIGHT; y += 10)
        for (x = 0; x < ILI9341_WIDTH / 2; x += 10)
            ILI9341_DrawPixel(x, y,
                              (((x / 10) + (y / 10)) & 1)
                                ? ILI9341_COLOR_YELLOW
                                : ILI9341_COLOR_BLUE);

    ILI9341_FillRect(ILI9341_WIDTH / 2, 50,
                     ILI9341_WIDTH / 2, ILI9341_HEIGHT - 50,
                     ILI9341_COLOR_GREEN);

    ILI9341_DrawString(10, ILI9341_HEIGHT - 20,
                       "left: checker  | right: solid",
                       ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK);

    delay(2000);
}
