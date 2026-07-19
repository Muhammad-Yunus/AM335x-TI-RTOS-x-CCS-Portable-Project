#ifndef _ILI9341_H_
#define _ILI9341_H_

#include <stdint.h>

/* ---------------------------------------------------------------------- */
/* ILI9341 command set (subset used by the demo)                          */
/* ---------------------------------------------------------------------- */
#define ILI9341_CMD_SLEEPOUT      0x11
#define ILI9341_CMD_DISPLAYOFF    0x28
#define ILI9341_CMD_DISPLAYON     0x29
#define ILI9341_CMD_COLADDR       0x2A
#define ILI9341_CMD_PAGEADDR      0x2B
#define ILI9341_CMD_GRAM          0x2C
#define ILI9341_CMD_MAC           0x36
#define ILI9341_CMD_PIXELFORMAT   0x3A
#define ILI9341_CMD_FRC           0xB1
#define ILI9341_CMD_DFC           0xB6
#define ILI9341_CMD_PWR1          0xC0
#define ILI9341_CMD_PWR2          0xC1
#define ILI9341_CMD_VCOM1         0xC5
#define ILI9341_CMD_VCOM2         0xC7
#define ILI9341_CMD_PWRA          0xCB
#define ILI9341_CMD_PWRB          0xCF
#define ILI9341_CMD_PGAMMA        0xE0
#define ILI9341_CMD_NGAMMA        0xE1
#define ILI9341_CMD_DTCA          0xE8
#define ILI9341_CMD_DTCB          0xEA
#define ILI9341_CMD_PWRSEQ        0xED
#define ILI9341_CMD_3GAMMAEN      0xF2
#define ILI9341_CMD_PRC           0xF7
#define ILI9341_CMD_GAMMA         0x26

/* 120ms is necessary after reset for loading ID bytes, VCOM setting,
 * and other settings from NV memory to registers. */
#define ILI9341_RESET_CANCEL_MS   120U

/* LCD geometry — ILI9341 2.8" in landscape. */
#define ILI9341_WIDTH             320
#define ILI9341_HEIGHT            240

/* RGB565 colour helpers. */
#define ILI9341_COLOR_BLACK       0x0000
#define ILI9341_COLOR_WHITE       0xFFFF
#define ILI9341_COLOR_RED         0xF800
#define ILI9341_COLOR_GREEN       0x07E0
#define ILI9341_COLOR_BLUE        0x001F
#define ILI9341_COLOR_CYAN        0x07FF
#define ILI9341_COLOR_MAGENTA     0xF81F
#define ILI9341_COLOR_YELLOW      0xFFE0
#define ILI9341_COLOR_ORANGE      0xFD20
#define ILI9341_COLOR_GREY        0x8410

/* RGB565 to two-byte big-endian payload (matches ILI9341 pixel format). */
#define ILI9341_HI(c)             ((uint8_t)(((c) >> 8) & 0xFF))
#define ILI9341_LO(c)             ((uint8_t)((c) & 0xFF))

/* Low-level driver entry points implemented in ili9341.c. The driver
 * targets the AM3352 SPI0 + DC/RST GPIO already wired in main.c.
 *
 * Function pointers let the same driver be reused if we want to route
 * SPI transactions elsewhere; for now they always point to the
 * AM3352-specific helpers. */
typedef void (*ili9341_send_byte_fn)(uint8_t b);
typedef void (*ili9341_send_buffer_fn)(const uint8_t *buf, uint32_t len);

/* Initialise GPIO for DC/RST, pulse reset, run the ILI9341 boot
 * sequence, and set landscape orientation (MV=0, MX=0, MY=1, BGR=1).
 * Must be called after SPI0 is up. */
void ILI9341_Init(void);

/* Set the rectangular window for subsequent pixel writes. */
void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0,
                               uint16_t x1, uint16_t y1);

/* Fill the entire screen with one RGB565 colour. */
void ILI9341_FillScreen(uint16_t color);

/* Fill a rectangle with one RGB565 colour. */
void ILI9341_FillRect(uint16_t x, uint16_t y,
                      uint16_t w, uint16_t h, uint16_t color);

/* Draw a single pixel. */
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

/* Draw a 1-pixel-thick line (Bresenham). */
void ILI9341_DrawLine(int16_t x0, int16_t y0,
                      int16_t x1, int16_t y1, uint16_t color);

/* Draw the outline / hollow rectangle. */
void ILI9341_DrawRect(uint16_t x, uint16_t y,
                      uint16_t w, uint16_t h, uint16_t color);

/* Draw a filled circle (midpoint). */
void ILI9341_DrawCircle(int16_t cx, int16_t cy,
                        int16_t r, uint16_t color);
void ILI9341_FillCircle(int16_t cx, int16_t cy,
                        int16_t r, uint16_t color);

/* Draw a 5x7 monospace ASCII character at (x, y) using the font
 * selected by the caller via fonts.h (Font5x7). */
void ILI9341_DrawChar(uint16_t x, uint16_t y,
                      char c, uint16_t fg, uint16_t bg);

/* Draw a null-terminated string at (x, y). */
void ILI9341_DrawString(uint16_t x, uint16_t y,
                        const char *s, uint16_t fg, uint16_t bg);

/* Convenience used by the demo to push a single RGB565 pixel stream
 * (already-formatted MSB/LSB byte pairs) into the GRAM window. */
void ILI9341_WritePixels(const uint16_t *pixels, uint32_t count);

#endif /* _ILI9341_H_ */
