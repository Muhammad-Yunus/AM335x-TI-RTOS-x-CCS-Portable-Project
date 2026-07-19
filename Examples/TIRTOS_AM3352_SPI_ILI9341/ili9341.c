/*
 * ili9341.c — ILI9341 2.8" TFT driver, ported to AM3352 StarterWare SPI0.
 *
 *   - SPI0 channel 0 is the bus (CLK = P9_22, MOSI = P9_18, CS = P9_17).
 *   - DC  = P8_26 = GPIO1[29]  (data/command select)
 *   - RST = P8_19 = GPIO0[22]  (active-low reset)
 *
 * Pixel format: RGB565, MSB first on the wire
 * (ILI9341_CMD_PIXELFORMAT=0x55 selects 16bit/pixel and RGB order).
 *
 * The SPI0 transmit helpers (Spi0TxByte / Spi0TxBuffer) and
 * DC/RST GPIO hooks (LcdDcLow / LcdDcHigh / LcdRstLow / LcdRstHigh) are
 * implemented in main.c. This file only calls those hooks.
 */

#include "ili9341.h"
#include "fonts.h"
#include "delay.h"      /* delay() in ILI9341_Reset() and ILI9341_Init() */



/* ---------------------------------------------------------------------- */
/* Low-level hooks provided by main.c.                                    */
/* ---------------------------------------------------------------------- */
extern void Spi0TxByte(uint8_t b);
extern void Spi0TxBuffer(const uint8_t *buf, uint32_t len);
extern void LcdDcLow(void);
extern void LcdDcHigh(void);
extern void LcdRstLow(void);
extern void LcdRstHigh(void);

/* ---------------------------------------------------------------------- */
/* Local helpers                                                          */
/* ---------------------------------------------------------------------- */
static void ILI9341_WriteCommand(uint8_t cmd)
{
    LcdDcLow();
    Spi0TxByte(cmd);
}

static void ILI9341_WriteData(uint8_t data)
{
    LcdDcHigh();
    Spi0TxByte(data);
}

static void ILI9341_WriteDataBuffer(const uint8_t *buf, uint32_t len)
{
    LcdDcHigh();
    Spi0TxBuffer(buf, len);
}

static void ILI9341_Reset(void)
{
    LcdRstHigh();
    delay(10);
    LcdRstLow();
    delay(10);
    LcdRstHigh();
    delay(ILI9341_RESET_CANCEL_MS);
}

/* ---------------------------------------------------------------------- */
/* Boot sequence — registers from the STM32H7 reference driver            */
/* (ILI9341_NUCLEO_H753ZI/Core/Src/ili9341.c).                            */
/* ---------------------------------------------------------------------- */
void ILI9341_Init(void)
{
    ILI9341_Reset();

    ILI9341_WriteCommand(ILI9341_CMD_DISPLAYOFF);

    ILI9341_WriteCommand(ILI9341_CMD_PWRA);
    ILI9341_WriteData(0x39);
    ILI9341_WriteData(0x2C);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x34);
    ILI9341_WriteData(0x02);

    ILI9341_WriteCommand(ILI9341_CMD_PWRB);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x83);
    ILI9341_WriteData(0x30);

    ILI9341_WriteCommand(ILI9341_CMD_DTCA);
    ILI9341_WriteData(0x85);
    ILI9341_WriteData(0x01);
    ILI9341_WriteData(0x79);

    ILI9341_WriteCommand(ILI9341_CMD_DTCB);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x00);

    ILI9341_WriteCommand(ILI9341_CMD_PWRSEQ);
    ILI9341_WriteData(0x64);
    ILI9341_WriteData(0x03);
    ILI9341_WriteData(0x12);
    ILI9341_WriteData(0x81);

    ILI9341_WriteCommand(ILI9341_CMD_PRC);
    ILI9341_WriteData(0x20);

    ILI9341_WriteCommand(ILI9341_CMD_PWR1);
    ILI9341_WriteData(0x26);

    ILI9341_WriteCommand(ILI9341_CMD_PWR2);
    ILI9341_WriteData(0x11);

    ILI9341_WriteCommand(ILI9341_CMD_VCOM1);
    ILI9341_WriteData(0x35);
    ILI9341_WriteData(0x3E);

    ILI9341_WriteCommand(ILI9341_CMD_VCOM2);
    ILI9341_WriteData(0xBE);

    /* Landscape (rotation 0 in our orientation table): MV=0, MX=0,
     * MY=1, BGR=1 -> 0x28, same value used by the STM32 reference. */
    ILI9341_WriteCommand(ILI9341_CMD_MAC);
    ILI9341_WriteData(0x28);

    ILI9341_WriteCommand(ILI9341_CMD_PIXELFORMAT);
    ILI9341_WriteData(0x55);   /* 16 bits / pixel */

    ILI9341_WriteCommand(ILI9341_CMD_FRC);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x1F);

    ILI9341_WriteCommand(ILI9341_CMD_DFC);
    ILI9341_WriteData(0x0A);
    ILI9341_WriteData(0x82);
    ILI9341_WriteData(0x27);
    ILI9341_WriteData(0x00);

    ILI9341_WriteCommand(ILI9341_CMD_3GAMMAEN);
    ILI9341_WriteData(0x00);

    ILI9341_WriteCommand(ILI9341_CMD_COLADDR);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0xEF);

    ILI9341_WriteCommand(ILI9341_CMD_PAGEADDR);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x01);
    ILI9341_WriteData(0x3F);

    ILI9341_WriteCommand(ILI9341_CMD_GAMMA);
    ILI9341_WriteData(0x01);

    ILI9341_WriteCommand(ILI9341_CMD_PGAMMA);
    {
        static const uint8_t pg[] = {
            0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
            0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00
        };
        ILI9341_WriteDataBuffer(pg, sizeof(pg));
    }

    ILI9341_WriteCommand(ILI9341_CMD_NGAMMA);
    {
        static const uint8_t ng[] = {
            0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
            0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F
        };
        ILI9341_WriteDataBuffer(ng, sizeof(ng));
    }

    ILI9341_WriteCommand(ILI9341_CMD_SLEEPOUT);
    ILI9341_WriteCommand(ILI9341_CMD_DISPLAYON);
    delay(50);
}

/* ---------------------------------------------------------------------- */
/* Address window + raw pixel stream                                      */
/* ---------------------------------------------------------------------- */
void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0,
                               uint16_t x1, uint16_t y1)
{
    ILI9341_WriteCommand(ILI9341_CMD_COLADDR);
    ILI9341_WriteData((uint8_t)(x0 >> 8));
    ILI9341_WriteData((uint8_t)(x0 & 0xFF));
    ILI9341_WriteData((uint8_t)(x1 >> 8));
    ILI9341_WriteData((uint8_t)(x1 & 0xFF));

    ILI9341_WriteCommand(ILI9341_CMD_PAGEADDR);
    ILI9341_WriteData((uint8_t)(y0 >> 8));
    ILI9341_WriteData((uint8_t)(y0 & 0xFF));
    ILI9341_WriteData((uint8_t)(y1 >> 8));
    ILI9341_WriteData((uint8_t)(y1 & 0xFF));

    ILI9341_WriteCommand(ILI9341_CMD_GRAM);
}

void ILI9341_WritePixels(const uint16_t *pixels, uint32_t count)
{
    /* In-place MSB/LSB conversion + DC high + block SPI send. */
    static uint8_t scratch[2 * 64];

    const uint8_t *src = (const uint8_t *)pixels;
    uint32_t total = count * 2;
    uint32_t off = 0;
    uint32_t i;       /* C89: declare at top of block */

    while (off < total)
    {
        uint32_t chunk = total - off;
        if (chunk > sizeof(scratch))
            chunk = sizeof(scratch);

        for (i = 0; i < chunk; i += 2)
        {
            scratch[i]     = src[off + i];     /* already MSB */
            scratch[i + 1] = src[off + i + 1]; /* already LSB */
        }

        LcdDcHigh();
        Spi0TxBuffer(scratch, chunk);
        off += chunk;
    }
}

void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    uint8_t px[2];

    if (x >= ILI9341_WIDTH || y >= ILI9341_HEIGHT)
        return;

    px[0] = ILI9341_HI(color);
    px[1] = ILI9341_LO(color);
    ILI9341_SetAddressWindow(x, y, x, y);
    ILI9341_WriteDataBuffer(px, 2);
}

/* ---------------------------------------------------------------------- */
/* Filled primitives                                                      */
/* ---------------------------------------------------------------------- */
void ILI9341_FillRect(uint16_t x, uint16_t y,
                      uint16_t w, uint16_t h, uint16_t color)
{
    /* C89 declarations first. */
    static uint8_t row[ILI9341_WIDTH * 2];
    uint8_t hi, lo;
    uint16_t i, r;

    if (w == 0 || h == 0) return;
    if (x >= ILI9341_WIDTH || y >= ILI9341_HEIGHT) return;
    if (x + w > ILI9341_WIDTH)  w = ILI9341_WIDTH - x;
    if (y + h > ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;

    ILI9341_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    hi = ILI9341_HI(color);
    lo = ILI9341_LO(color);

    for (i = 0; i < w; i++)
    {
        row[2 * i]     = hi;
        row[2 * i + 1] = lo;
    }

    LcdDcHigh();
    for (r = 0; r < h; r++)
        Spi0TxBuffer(row, (uint32_t)w * 2);
}

void ILI9341_FillScreen(uint16_t color)
{
    ILI9341_FillRect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}

void ILI9341_DrawRect(uint16_t x, uint16_t y,
                      uint16_t w, uint16_t h, uint16_t color)
{
    ILI9341_FillRect(x,         y,         w, 1, color);
    ILI9341_FillRect(x,         y + h - 1, w, 1, color);
    ILI9341_FillRect(x,         y,         1, h, color);
    ILI9341_FillRect(x + w - 1, y,         1, h, color);
}

/* Bresenham. Integer-only, public-domain implementation. */
void ILI9341_DrawLine(int16_t x0, int16_t y0,
                      int16_t x1, int16_t y1, uint16_t color)
{
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    int16_t cur_x = x0, cur_y = y0;
    int16_t e2;        /* C89: declare at top of block */

    while (1)
    {
        ILI9341_DrawPixel((uint16_t)cur_x, (uint16_t)cur_y, color);
        if (cur_x == x1 && cur_y == y1) break;

        e2 = (int16_t)(err << 1);
        if (e2 > -dy) { err -= dy; cur_x = (int16_t)(cur_x + sx); }
        if (e2 <  dx) { err += dx; cur_y = (int16_t)(cur_y + sy); }
    }
}

/* Midpoint circle + filled variant. */
static void ILI9341_DrawCircle8(int16_t cx, int16_t cy,
                                int16_t x, int16_t y, uint16_t color)
{
    ILI9341_DrawPixel((uint16_t)(cx + x), (uint16_t)(cy + y), color);
    ILI9341_DrawPixel((uint16_t)(cx - x), (uint16_t)(cy + y), color);
    ILI9341_DrawPixel((uint16_t)(cx + x), (uint16_t)(cy - y), color);
    ILI9341_DrawPixel((uint16_t)(cx - x), (uint16_t)(cy - y), color);
    ILI9341_DrawPixel((uint16_t)(cx + y), (uint16_t)(cy + x), color);
    ILI9341_DrawPixel((uint16_t)(cx - y), (uint16_t)(cy + x), color);
    ILI9341_DrawPixel((uint16_t)(cx + y), (uint16_t)(cy - x), color);
    ILI9341_DrawPixel((uint16_t)(cx - y), (uint16_t)(cy - x), color);
}

void ILI9341_DrawCircle(int16_t cx, int16_t cy,
                        int16_t r, uint16_t color)
{
    if (r <= 0) return;

    int16_t x = 0, y = r, err = 1 - r;
    while (x <= y)
    {
        ILI9341_DrawCircle8(cx, cy, x, y, color);
        int16_t e2 = err;
        if (e2 < 0)     err = (int16_t)(err + 2 * x + 3);
        else { y--; err = (int16_t)(err + 2 * (x - y) + 5); }
        x++;
    }
}

void ILI9341_FillCircle(int16_t cx, int16_t cy,
                        int16_t r, uint16_t color)
{
    if (r <= 0) return;

    int16_t x = 0, y = r, err = 1 - r;
    while (x <= y)
    {
        ILI9341_FillRect((uint16_t)(cx - x), (uint16_t)(cy - y),
                         (uint16_t)(2 * x + 1), (uint16_t)(2 * y + 1),
                         color);
        ILI9341_FillRect((uint16_t)(cx - y), (uint16_t)(cy - x),
                         (uint16_t)(2 * y + 1), (uint16_t)(2 * x + 1),
                         color);
        int16_t e2 = err;
        if (e2 < 0)     err = (int16_t)(err + 2 * x + 3);
        else { y--; err = (int16_t)(err + 2 * (x - y) + 5); }
        x++;
    }
}

/* ---------------------------------------------------------------------- */
/* Text primitives using Font5x7 (5 cols x 7 rows, ASCII 0x20..0x7E).      */
/* ---------------------------------------------------------------------- */
void ILI9341_DrawChar(uint16_t x, uint16_t y,
                      char c, uint16_t fg, uint16_t bg)
{
    /* C89: declarations at the top of a block. */
    static uint8_t colbuf[FONT5X7_HEIGHT * 2];
    const uint8_t *glyph;
    uint8_t fg_hi, fg_lo, bg_hi, bg_lo;
    uint8_t col, row, bits;

    if ((uint8_t)c < FONT5X7_FIRST || (uint8_t)c > FONT5X7_LAST)
        c = '?';
    glyph = Font5x7[(uint8_t)c - FONT5X7_FIRST];

    fg_hi = ILI9341_HI(fg);
    fg_lo = ILI9341_LO(fg);
    bg_hi = ILI9341_HI(bg);
    bg_lo = ILI9341_LO(bg);

    /* Build each glyph column's worth of RGB565 pixels (7 rows tall).
     * Address window is reset for every column so the controller can't
     * auto-wrap to the next column and corrupt the layout. */
    for (col = 0; col < FONT5X7_WIDTH; col++)
    {
        bits = glyph[col];
        for (row = 0; row < FONT5X7_HEIGHT; row++)
        {
            if (bits & (uint8_t)(1U << row))
            {
                colbuf[2 * row]     = fg_hi;
                colbuf[2 * row + 1] = fg_lo;
            }
            else
            {
                colbuf[2 * row]     = bg_hi;
                colbuf[2 * row + 1] = bg_lo;
            }
        }

        ILI9341_SetAddressWindow((uint16_t)(x + col), y,
                                 (uint16_t)(x + col),
                                 (uint16_t)(y + FONT5X7_HEIGHT - 1));
        LcdDcHigh();
        Spi0TxBuffer(colbuf, (uint32_t)FONT5X7_HEIGHT * 2);
    }

    /* Trailing blank column between characters (1 px wide, glyph tall). */
    ILI9341_FillRect((uint16_t)(x + FONT5X7_WIDTH), y,
                     1, FONT5X7_HEIGHT, bg);
}

void ILI9341_DrawString(uint16_t x, uint16_t y,
                        const char *s, uint16_t fg, uint16_t bg)
{
    uint16_t cur_x = x;
    while (*s)
    {
        ILI9341_DrawChar(cur_x, y, *s, fg, bg);
        cur_x = (uint16_t)(cur_x + FONT5X7_WIDTH + 1);
        s++;
    }
}
