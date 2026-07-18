#include <stdlib.h>
#include <math.h>
#include "ssd1306.h"

#include <ti/drv/i2c/I2C.h>

extern I2C_Handle g_i2cHandle;

static SSD1306_t SSD1306;
static uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE];
static SSD1306_Geometry display_geometry = SSD1306_GEOMETRY;

static const uint16_t width(void)  { return SSD1306_WIDTH; };
static const uint16_t height(void) { return SSD1306_HEIGHT; };

uint16_t ssd1306_GetWidth(void)  { return SSD1306_WIDTH; }
uint16_t ssd1306_GetHeight(void) { return SSD1306_HEIGHT; }

SSD1306_COLOR ssd1306_GetColor(void)         { return SSD1306.Color; }
void          ssd1306_SetColor(SSD1306_COLOR c) { SSD1306.Color = c; }

uint8_t ssd1306_Init(void)
{
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;
    SSD1306.Color = Black;
    SSD1306.Initialized = 1;

    ssd1306_I2C_WriteCommand(DISPLAYOFF);
    ssd1306_I2C_WriteCommand(SETDISPLAYCLOCKDIV);
    ssd1306_I2C_WriteCommand(0xF0);
    ssd1306_I2C_WriteCommand(SETMULTIPLEX);
    ssd1306_I2C_WriteCommand(height() - 1);
    ssd1306_I2C_WriteCommand(SETDISPLAYOFFSET);
    ssd1306_I2C_WriteCommand(0x00);
    ssd1306_I2C_WriteCommand(SETSTARTLINE);
    ssd1306_I2C_WriteCommand(CHARGEPUMP);
    ssd1306_I2C_WriteCommand(0x14);
    ssd1306_I2C_WriteCommand(MEMORYMODE);
    ssd1306_I2C_WriteCommand(0x00);
    ssd1306_I2C_WriteCommand(SEGREMAP);
    ssd1306_I2C_WriteCommand(COMSCANINC);
    ssd1306_I2C_WriteCommand(SETCOMPINS);

    if (display_geometry == GEOMETRY_128_64) ssd1306_I2C_WriteCommand(0x12);
    else                                     ssd1306_I2C_WriteCommand(0x02);

    ssd1306_I2C_WriteCommand(SETCONTRAST);
    if (display_geometry == GEOMETRY_128_64) ssd1306_I2C_WriteCommand(0xCF);
    else                                     ssd1306_I2C_WriteCommand(0x8F);

    ssd1306_I2C_WriteCommand(SETPRECHARGE);
    ssd1306_I2C_WriteCommand(0xF1);
    ssd1306_I2C_WriteCommand(SETVCOMDETECT);
    ssd1306_I2C_WriteCommand(0x40);
    ssd1306_I2C_WriteCommand(DISPLAYALLON_RESUME);
    ssd1306_I2C_WriteCommand(NORMALDISPLAY);
    ssd1306_I2C_WriteCommand(0x2e);
    ssd1306_I2C_WriteCommand(DISPLAYON);

    ssd1306_Clear();
    ssd1306_UpdateScreen();

    return 1;
}

void ssd1306_Fill(void)
{
    uint32_t i;
    for (i = 0; i < sizeof(SSD1306_Buffer); i++) {
        SSD1306_Buffer[i] = (SSD1306.Color == Black) ? 0x00 : 0xFF;
    }
}

void ssd1306_DrawPixel(uint8_t x, uint8_t y)
{
    SSD1306_COLOR color = SSD1306.Color;

    if (x >= ssd1306_GetWidth() || y >= ssd1306_GetHeight()) {
        return;
    }
    if (SSD1306.Inverted) {
        color = (SSD1306_COLOR) !color;
    }
    if (color == White) {
        SSD1306_Buffer[x + (y / 8) * width()] |=  1 << (y % 8);
    } else {
        SSD1306_Buffer[x + (y / 8) * width()] &= ~(1 << (y % 8));
    }
}

void ssd1306_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) { SWAP_INT16_T(x0, y0); SWAP_INT16_T(x1, y1); }
    if (x0 > x1) { SWAP_INT16_T(x0, x1); SWAP_INT16_T(y0, y1); }

    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++) {
        if (steep) ssd1306_DrawPixel(y0, x0);
        else       ssd1306_DrawPixel(x0, y0);
        err -= dy;
        if (err < 0) { y0 += ystep; err += dx; }
    }
}

void ssd1306_DrawHorizontalLine(int16_t x, int16_t y, int16_t length)
{
    if (y < 0 || y >= height()) return;

    if (x < 0) { length += x; x = 0; }
    if ((x + length) > width()) { length = (width() - x); }
    if (length <= 0) return;

    uint8_t *bufferPtr = SSD1306_Buffer;
    bufferPtr += (y >> 3) * width();
    bufferPtr += x;

    uint8_t drawBit = 1 << (y & 7);

    switch (SSD1306.Color) {
        case White:   while (length--) { *bufferPtr++ |= drawBit; } break;
        case Black:   drawBit = ~drawBit;
                      while (length--) { *bufferPtr++ &= drawBit; } break;
        case Inverse: while (length--) { *bufferPtr++ ^= drawBit; } break;
    }
}

void ssd1306_DrawVerticalLine(int16_t x, int16_t y, int16_t length)
{
    if (x < 0 || x >= width()) return;

    if (y < 0) { length += y; y = 0; }
    if ((y + length) > height()) { length = (height() - y); }
    if (length <= 0) return;

    uint8_t yOffset = y & 7;
    uint8_t drawBit;
    uint8_t *bufferPtr = SSD1306_Buffer;
    bufferPtr += (y >> 3) * width();
    bufferPtr += x;

    if (yOffset) {
        yOffset = 8 - yOffset;
        drawBit = ~(0xFF >> (yOffset));
        if (length < yOffset) drawBit &= (0xFF >> (yOffset - length));

        switch (SSD1306.Color) {
            case White:   *bufferPtr |=  drawBit; break;
            case Black:   *bufferPtr &= ~drawBit; break;
            case Inverse: *bufferPtr ^=  drawBit; break;
        }
        if (length < yOffset) return;
        length -= yOffset;
        bufferPtr += width();
    }

    if (length >= 8) {
        switch (SSD1306.Color) {
            case White:
            case Black:
                drawBit = (SSD1306.Color == White) ? 0xFF : 0x00;
                do { *bufferPtr = drawBit; bufferPtr += width(); length -= 8; } while (length >= 8);
                break;
            case Inverse:
                do { *bufferPtr = ~(*bufferPtr); bufferPtr += width(); length -= 8; } while (length >= 8);
                break;
        }
    }

    if (length > 0) {
        drawBit = (1 << (length & 7)) - 1;
        switch (SSD1306.Color) {
            case White:   *bufferPtr |=  drawBit; break;
            case Black:   *bufferPtr &= ~drawBit; break;
            case Inverse: *bufferPtr ^=  drawBit; break;
        }
    }
}

void ssd1306_DrawRect(int16_t x, int16_t y, int16_t width, int16_t height)
{
    ssd1306_DrawHorizontalLine(x, y, width);
    ssd1306_DrawVerticalLine(x, y, height);
    ssd1306_DrawVerticalLine(x + width - 1, y, height);
    ssd1306_DrawHorizontalLine(x, y + height - 1, width);
}

void ssd1306_FillRect(int16_t xMove, int16_t yMove, int16_t width, int16_t height)
{
    int16_t x;
    for (x = xMove; x < xMove + width; x++) {
        ssd1306_DrawVerticalLine(x, yMove, height);
    }
}

void ssd1306_DrawTriangle(uint16_t x1, uint16_t y1,
                          uint16_t x2, uint16_t y2,
                          uint16_t x3, uint16_t y3)
{
    ssd1306_DrawLine(x1, y1, x2, y2);
    ssd1306_DrawLine(x2, y2, x3, y3);
    ssd1306_DrawLine(x3, y3, x1, y1);
}

void ssd1306_Polyline(const SSD1306_VERTEX *par_vertex, uint16_t par_size)
{
    uint16_t i;
    if (par_vertex != 0) {
        for (i = 1; i < par_size; i++) {
            ssd1306_DrawLine(par_vertex[i - 1].x, par_vertex[i - 1].y,
                             par_vertex[i].x,     par_vertex[i].y);
        }
    }
}

static float ssd1306_DegToRad(float par_deg)
{
    return par_deg * 3.14f / 180.0f;
}

static uint16_t ssd1306_NormalizeTo0_360(uint16_t par_deg)
{
    uint16_t loc_angle;
    if (par_deg <= 360) {
        loc_angle = par_deg;
    } else {
        loc_angle = par_deg % 360;
        loc_angle = ((par_deg != 0) ? par_deg : 360);
    }
    return loc_angle;
}

void ssd1306_DrawArc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep)
{
    #define CIRCLE_APPROXIMATION_SEGMENTS 36
    float approx_degree;
    uint32_t approx_segments;
    uint8_t xp1, xp2, yp1, yp2;
    uint32_t count = 0;
    uint32_t loc_sweep = 0;
    float rad;

    loc_sweep = ssd1306_NormalizeTo0_360(sweep);

    count = (ssd1306_NormalizeTo0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_degree = loc_sweep / (float)approx_segments;
    while (count < approx_segments) {
        rad = ssd1306_DegToRad(count * approx_degree);
        xp1 = x + (int8_t)(sin(rad) * radius);
        yp1 = y + (int8_t)(cos(rad) * radius);
        count++;
        if (count != approx_segments) rad = ssd1306_DegToRad(count * approx_degree);
        else                          rad = ssd1306_DegToRad(loc_sweep);
        xp2 = x + (int8_t)(sin(rad) * radius);
        yp2 = y + (int8_t)(cos(rad) * radius);
        ssd1306_DrawLine(xp1, yp1, xp2, yp2);
    }
}

void ssd1306_DrawCircle(int16_t x0, int16_t y0, int16_t radius)
{
    int16_t x = 0, y = radius;
    int16_t dp = 1 - radius;
    do {
        if (dp < 0) dp = dp + 2 * (++x) + 3;
        else        dp = dp + 2 * (++x) - 2 * (--y) + 5;

        ssd1306_DrawPixel(x0 + x, y0 + y);
        ssd1306_DrawPixel(x0 - x, y0 + y);
        ssd1306_DrawPixel(x0 + x, y0 - y);
        ssd1306_DrawPixel(x0 - x, y0 - y);
        ssd1306_DrawPixel(x0 + y, y0 + x);
        ssd1306_DrawPixel(x0 - y, y0 + x);
        ssd1306_DrawPixel(x0 + y, y0 - x);
        ssd1306_DrawPixel(x0 - y, y0 - x);
    } while (x < y);

    ssd1306_DrawPixel(x0 + radius, y0);
    ssd1306_DrawPixel(x0, y0 + radius);
    ssd1306_DrawPixel(x0 - radius, y0);
    ssd1306_DrawPixel(x0, y0 - radius);
}

void ssd1306_FillCircle(int16_t x0, int16_t y0, int16_t radius)
{
    int16_t x = 0, y = radius;
    int16_t dp = 1 - radius;
    do {
        if (dp < 0) dp = dp + 2 * (++x) + 3;
        else        dp = dp + 2 * (++x) - 2 * (--y) + 5;

        ssd1306_DrawHorizontalLine(x0 - x, y0 - y, 2 * x);
        ssd1306_DrawHorizontalLine(x0 - x, y0 + y, 2 * x);
        ssd1306_DrawHorizontalLine(x0 - y, y0 - x, 2 * y);
        ssd1306_DrawHorizontalLine(x0 - y, y0 + x, 2 * y);
    } while (x < y);
    ssd1306_DrawHorizontalLine(x0 - radius, y0, 2 * radius);
}

void ssd1306_DrawCircleQuads(int16_t x0, int16_t y0, int16_t radius, uint8_t quads)
{
    int16_t x = 0, y = radius;
    int16_t dp = 1 - radius;
    while (x < y) {
        if (dp < 0) dp = dp + 2 * (++x) + 3;
        else        dp = dp + 2 * (++x) - 2 * (--y) + 5;
        if (quads & 0x1) {
            ssd1306_DrawPixel(x0 + x, y0 - y);
            ssd1306_DrawPixel(x0 + y, y0 - x);
        }
        if (quads & 0x2) {
            ssd1306_DrawPixel(x0 - y, y0 - x);
            ssd1306_DrawPixel(x0 - x, y0 - y);
        }
        if (quads & 0x4) {
            ssd1306_DrawPixel(x0 - y, y0 + x);
            ssd1306_DrawPixel(x0 - x, y0 + y);
        }
        if (quads & 0x8) {
            ssd1306_DrawPixel(x0 + x, y0 + y);
            ssd1306_DrawPixel(x0 + y, y0 + x);
        }
    }
    if ((quads & 0x1) && (quads & 0x8)) ssd1306_DrawPixel(x0 + radius, y0);
    if ((quads & 0x4) && (quads & 0x8)) ssd1306_DrawPixel(x0, y0 + radius);
    if ((quads & 0x2) && (quads & 0x4)) ssd1306_DrawPixel(x0 - radius, y0);
    if ((quads & 0x1) && (quads & 0x2)) ssd1306_DrawPixel(x0, y0 - radius);
}

void ssd1306_DrawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress)
{
    uint16_t radius = height / 2;
    uint16_t xRadius = x + radius;
    uint16_t yRadius = y + radius;
    uint16_t doubleRadius = 2 * radius;
    uint16_t innerRadius = radius - 2;

    ssd1306_SetColor(White);
    ssd1306_DrawCircleQuads(xRadius, yRadius, radius, 0x06);
    ssd1306_DrawHorizontalLine(xRadius, y, width - doubleRadius + 1);
    ssd1306_DrawHorizontalLine(xRadius, y + height, width - doubleRadius + 1);
    ssd1306_DrawCircleQuads(x + width - radius, yRadius, radius, 0x09);

    uint16_t maxProgressWidth = (width - doubleRadius + 1) * progress / 100;

    ssd1306_FillCircle(xRadius, yRadius, innerRadius);
    ssd1306_FillRect(xRadius + 1, y + 2, maxProgressWidth, height - 3);
    ssd1306_FillCircle(xRadius + maxProgressWidth, yRadius, innerRadius);
}

void ssd1306_DrawBitmap(uint8_t X, uint8_t Y, uint8_t W, uint8_t H, const uint8_t* pBMP)
{
    uint8_t pX, pY, tmpCh, bL;
    pY = Y;
    while (pY < Y + H) {
        pX = X;
        while (pX < X + W) {
            bL = 0;
            tmpCh = *pBMP++;
            if (tmpCh) {
                while (bL < 8) {
                    if (tmpCh & 0x01) ssd1306_DrawPixel(pX, pY + bL);
                    tmpCh >>= 1;
                    if (tmpCh) bL++;
                    else { pX++; break; }
                }
            } else {
                pX++;
            }
        }
        pY += 8;
    }
}

char ssd1306_WriteChar(char ch, FontDef Font)
{
    uint32_t i, b, j;

    if (width()  <= (SSD1306.CurrentX + Font.FontWidth) ||
        height() <= (SSD1306.CurrentY + Font.FontHeight)) {
        return 0;
    }

    for (i = 0; i < Font.FontHeight; i++) {
        b = Font.data[(ch - 32) * Font.FontHeight + i];
        for (j = 0; j < Font.FontWidth; j++) {
            if ((b << j) & 0x8000) {
                ssd1306_DrawPixel(SSD1306.CurrentX + j, SSD1306.CurrentY + i);
            } else {
                SSD1306.Color = !SSD1306.Color;
                ssd1306_DrawPixel(SSD1306.CurrentX + j, SSD1306.CurrentY + i);
                SSD1306.Color = !SSD1306.Color;
            }
        }
    }

    SSD1306.CurrentX += Font.FontWidth;
    return ch;
}

char ssd1306_WriteString(char* str, FontDef Font)
{
    while (*str) {
        if (ssd1306_WriteChar(*str, Font) != *str) {
            return *str;
        }
        str++;
    }
    return *str;
}

void ssd1306_SetCursor(uint8_t x, uint8_t y)
{
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

void ssd1306_Clear(void)
{
    memset(SSD1306_Buffer, 0, SSD1306_BUFFER_SIZE);
}

/* ----- I2C transport using PDK I2C driver ----- */

void ssd1306_I2C_WriteCommand(uint8_t command)
{
    uint8_t buf[2];
    I2C_Transaction trans;

    buf[0] = 0x00;
    buf[1] = command;

    I2C_transactionInit(&trans);
    trans.writeBuf = buf;
    trans.writeCount = 2;
    trans.slaveAddress = SSD1306_ADDRESS;
    trans.timeout = 1000;

    I2C_transfer(g_i2cHandle, &trans);
}

void ssd1306_I2C_WriteData(uint8_t *data, uint16_t size)
{
    while (size > 0)
    {
        uint8_t buf[17];
        uint16_t chunk = (size > 16) ? 16 : size;
        uint16_t i;
        I2C_Transaction trans;

        buf[0] = 0x40;
        for (i = 0; i < chunk; i++)
            buf[1 + i] = data[i];

        I2C_transactionInit(&trans);
        trans.writeBuf = buf;
        trans.writeCount = chunk + 1;
        trans.slaveAddress = SSD1306_ADDRESS;
        trans.timeout = 1000;

        I2C_transfer(g_i2cHandle, &trans);

        data += chunk;
        size -= chunk;
    }
}

void ssd1306_UpdateScreen(void)
{
    uint8_t i;
    for (i = 0; i < SSD1306_HEIGHT / 8; i++) {
        ssd1306_I2C_WriteCommand(0xB0 + i);
        ssd1306_I2C_WriteCommand(SETLOWCOLUMN);
        ssd1306_I2C_WriteCommand(SETHIGHCOLUMN);
        ssd1306_I2C_WriteData(&SSD1306_Buffer[SSD1306_WIDTH * i], width());
    }
}
