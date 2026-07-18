/*
 * ssd1306.h
 *
 * AM3352 (StarterWare) port of the SSD1306 OLED driver
 * originally written for STM32 by Andriy Honcharenko / Roberto Benjami.
 *
 *  - Geometry/commands/font drawing logic are unchanged.
 *  - I2C transport is replaced with ssd1306_I2C_WriteCommand() and
 *    ssd1306_I2C_WriteData(), which the application wires to the
 *    StarterWare I2C1 master at init time.
 */

#ifndef SSD1306_H_
#define SSD1306_H_

#include "ssd1306_defines.h"
#include "fonts.h"
#include <stdint.h>
#include <string.h>

/* I2C address: 7-bit shifted left by 1 for the controller. */
#define SSD1306_I2C_ADDR       (SSD1306_ADDRESS << 1)

#ifdef SSD1306_128X64
#define SSD1306_GEOMETRY       GEOMETRY_128_64
#define SSD1306_WIDTH          128
#define SSD1306_HEIGHT         64
#elif defined(SSD1306_128X32)
#define SSD1306_GEOMETRY       GEOMETRY_128_32
#define SSD1306_WIDTH          128
#define SSD1306_HEIGHT         32
#endif

#define SSD1306_BUFFER_SIZE   (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

/* SSD1306 command set */
#define CHARGEPUMP            0x8D
#define COLUMNADDR            0x21
#define COMSCANDEC            0xC8
#define COMSCANINC            0xC0
#define DISPLAYALLON          0xA5
#define DISPLAYALLON_RESUME   0xA4
#define DISPLAYOFF            0xAE
#define DISPLAYON             0xAF
#define EXTERNALVCC           0x01
#define INVERTDISPLAY         0xA7
#define MEMORYMODE            0x20
#define NORMALDISPLAY         0xA6
#define PAGEADDR              0x22
#define SEGREMAP              0xA0
#define SETCOMPINS            0xDA
#define SETCONTRAST           0x81
#define SETDISPLAYCLOCKDIV    0xD5
#define SETDISPLAYOFFSET      0xD3
#define SETHIGHCOLUMN         0x10
#define SETLOWCOLUMN          0x00
#define SETMULTIPLEX          0xA8
#define SETPRECHARGE          0xD9
#define SETSEGMENTREMAP       0xA1
#define SETSTARTLINE          0x40
#define SETVCOMDETECT         0xDB
#define SWITCHCAPVCC          0x02

#define SWAP_INT16_T(a, b) { int16_t t = a; a = b; b = t; }

typedef enum {
    Black = 0x00,
    White = 0x01,
    Inverse = 0x02
} SSD1306_COLOR;

typedef enum {
    GEOMETRY_128_64 = 0,
    GEOMETRY_128_32 = 1
} SSD1306_Geometry;

typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t  Inverted;
    SSD1306_COLOR Color;
    uint8_t  Initialized;
} SSD1306_t;

typedef struct {
    uint8_t x;
    uint8_t y;
} SSD1306_VERTEX;

/* ----- I2C transport hooks supplied by the application layer ----- */

/* Send one command byte (control byte 0x00 + cmd). */
void ssd1306_I2C_WriteCommand(uint8_t command);

/* Send a block of data bytes (control byte 0x40 + payload). */
void ssd1306_I2C_WriteData(uint8_t *data, uint16_t size);

/* ----- Public driver API ----- */
uint16_t ssd1306_GetWidth(void);
uint16_t ssd1306_GetHeight(void);
SSD1306_COLOR ssd1306_GetColor(void);
void ssd1306_SetColor(SSD1306_COLOR color);
uint8_t ssd1306_Init(void);
void ssd1306_Fill(void);
void ssd1306_DrawPixel(uint8_t x, uint8_t y);
void ssd1306_DrawBitmap(uint8_t X, uint8_t Y, uint8_t W, uint8_t H, const uint8_t* pBMP);
void ssd1306_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void ssd1306_DrawVerticalLine(int16_t x, int16_t y, int16_t length);
void ssd1306_DrawHorizontalLine(int16_t x, int16_t y, int16_t length);
void ssd1306_DrawRect(int16_t x, int16_t y, int16_t width, int16_t height);
void ssd1306_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3);
void ssd1306_FillRect(int16_t xMove, int16_t yMove, int16_t width, int16_t height);
void ssd1306_DrawArc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep);
void ssd1306_DrawCircle(int16_t x0, int16_t y0, int16_t radius);
void ssd1306_FillCircle(int16_t x0, int16_t y0, int16_t radius);
void ssd1306_Polyline(const SSD1306_VERTEX *par_vertex, uint16_t par_size);
void ssd1306_DrawCircleQuads(int16_t x0, int16_t y0, int16_t radius, uint8_t quads);
void ssd1306_DrawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress);
char ssd1306_WriteChar(char ch, FontDef Font);
char ssd1306_WriteString(char* str, FontDef Font);
void ssd1306_SetCursor(uint8_t x, uint8_t y);
void ssd1306_Clear(void);

void ssd1306_UpdateScreen(void);

#define ssd1306_DisplayOn()             ssd1306_I2C_WriteCommand(DISPLAYON)
#define ssd1306_DisplayOff()            ssd1306_I2C_WriteCommand(DISPLAYOFF)
#define ssd1306_InvertDisplay()         ssd1306_I2C_WriteCommand(INVERTDISPLAY)
#define ssd1306_NormalDisplay()         ssd1306_I2C_WriteCommand(NORMALDISPLAY)
#define ssd1306_ResetOrientation()      { ssd1306_I2C_WriteCommand(SEGREMAP); ssd1306_I2C_WriteCommand(COMSCANINC); }
#define ssd1306_FlipScreenVertically()  { ssd1306_I2C_WriteCommand(SEGREMAP | 0x01); ssd1306_I2C_WriteCommand(COMSCANDEC); }
#define ssd1306_MirrorScreen()          { ssd1306_I2C_WriteCommand(SEGREMAP | 0x01); ssd1306_I2C_WriteCommand(COMSCANINC); }
#define ssd1306_MirrorFlipScreen()      { ssd1306_I2C_WriteCommand(SEGREMAP); ssd1306_I2C_WriteCommand(COMSCANDEC); }

#endif /* SSD1306_H_ */
