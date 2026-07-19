#include "lv_port_disp.h"
#include "ili9341.h"
#include "SPI_board.h"

#define LCD_WIDTH   ILI9341_WIDTH
#define LCD_HEIGHT  ILI9341_HEIGHT
#define LCD_FB_BPP  2

#define LCD_STRIPE_HEIGHT  (40)
#define LCD_FB_COLS        (LCD_WIDTH)
#define LCD_FB_SIZE        (LCD_FB_COLS * LCD_STRIPE_HEIGHT * LCD_FB_BPP)

static uint8_t s_frameBuffer[2][LCD_FB_SIZE] __attribute__((aligned(4)));

static void LCD_FlushDisplay(lv_display_t *disp, const lv_area_t *area,
                             uint8_t *color_p)
{
    lv_coord_t x1 = area->x1;
    lv_coord_t y1 = area->y1;
    lv_coord_t x2 = area->x2;
    lv_coord_t y2 = area->y2;
    uint8_t data[4];

    LcdDcLow();
    Spi0TxByte(ILI9341_CMD_COLADDR);
    LcdDcHigh();
    data[0] = (uint8_t)((x1 >> 8) & 0xFF);
    data[1] = (uint8_t)(x1 & 0xFF);
    data[2] = (uint8_t)((x2 >> 8) & 0xFF);
    data[3] = (uint8_t)(x2 & 0xFF);
    Spi0TxBuffer(data, 4);

    LcdDcLow();
    Spi0TxByte(ILI9341_CMD_PAGEADDR);
    LcdDcHigh();
    data[0] = (uint8_t)((y1 >> 8) & 0xFF);
    data[1] = (uint8_t)(y1 & 0xFF);
    data[2] = (uint8_t)((y2 >> 8) & 0xFF);
    data[3] = (uint8_t)(y2 & 0xFF);
    Spi0TxBuffer(data, 4);

    LcdDcLow();
    Spi0TxByte(ILI9341_CMD_GRAM);
    LcdDcHigh();

    uint32_t send_size =
        (uint32_t)(x2 - x1 + 1) * (uint32_t)(y2 - y1 + 1) * LCD_FB_BPP;
    Spi0TxBuffer(color_p, send_size);

    lv_disp_flush_ready(disp);
}

void lv_port_disp_init(void)
{
    ILI9341_Init();

    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);

    lv_display_set_buffers(disp,
                           s_frameBuffer[0],
                           s_frameBuffer[1],
                           sizeof(s_frameBuffer[0]),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_display_set_flush_cb(disp, LCD_FlushDisplay);
}
