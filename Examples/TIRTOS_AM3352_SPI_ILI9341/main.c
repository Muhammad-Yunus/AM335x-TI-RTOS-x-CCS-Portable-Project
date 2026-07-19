/* ==========================================================================
 * SYS/BIOS + TI-RTOS - ILI9341 LCD Demo on AM3352 (BeagleBone Black)
 *
 * HW mapping:
 *   CS   = P9_17  = SPI0_CS0  (HW CS,  mode 0)
 *   DC   = P8_26  = GPIO1_29
 *   RST  = P8_19  = GPIO0_22
 *   SCLK = P9_22  = SPI0_SCLK
 *   MOSI = P9_18  = SPI0_D1
 * ========================================================================*/

/* -----------------------------------------------------------------------
 * SYS/BIOS & XDC
 * -----------------------------------------------------------------------*/
#include <xdc/std.h>
#include <xdc/runtime/Error.h>

/* -----------------------------------------------------------------------
 * SYS/BIOS kernel
 * -----------------------------------------------------------------------*/
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* -----------------------------------------------------------------------
 * Board init (pinmux, clocks, UART stdio)
 * -----------------------------------------------------------------------*/
#include <ti/board/board.h>

/* -----------------------------------------------------------------------
 * UART driver (debug output)
 * -----------------------------------------------------------------------*/
#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>

/* -----------------------------------------------------------------------
 * GPIO driver
 * -----------------------------------------------------------------------*/
#include <ti/drv/gpio/GPIO.h>

/* -----------------------------------------------------------------------
 * Board-specific headers
 * -----------------------------------------------------------------------*/
#include "GPIO_board.h"
#include "SPI_board.h"

/* -----------------------------------------------------------------------
 * ILI9341 LCD driver
 * -----------------------------------------------------------------------*/
#include "ili9341.h"
#include "delay.h"

/* -----------------------------------------------------------------------
 * Constants
 * -----------------------------------------------------------------------*/
#define LCD_TASK_PRIORITY     (1)
#define LCD_TASK_STACK_SIZE   (0x2000)

/* -----------------------------------------------------------------------
 * Forward declarations
 * -----------------------------------------------------------------------*/
Void lcd_demo_task(UArg arg0, UArg arg1);

/* -----------------------------------------------------------------------
 * Board init helper - pinmux + module clocks + UART stdio
 * -----------------------------------------------------------------------*/
static void Board_initGPIO(void)
{
    Board_initCfg boardCfg;

    boardCfg = BOARD_INIT_PINMUX_CONFIG |
               BOARD_INIT_MODULE_CLOCK |
               BOARD_INIT_UART_STDIO;

    Board_init(boardCfg);
}

/* ====================================================================== *
 * ILI9341 low-level SPI & GPIO hooks
 *
 * These are called by ili9341.c and must match the extern declarations
 * in that file.  All run from the LCD task (scheduler active), so the
 * PDK SPI driver in blocking mode works correctly.
 * ====================================================================== */

/* RX drain buffer used by PDK SPI_transfer */
static uint8_t lcdRxBuf[SPI_LCD_CHUNK];

/* delay() — RTOS-aware, yields the task */
void delay(uint32_t ms)
{
    Task_sleep(ms);
}

/* Spi0TxByte — send 1 byte via PDK SPI driver with hardware CS */
void Spi0TxByte(uint8_t b)
{
    SPI_Transaction transaction;
    bool ok;

    transaction.count = 1;
    transaction.txBuf = &b;
    transaction.rxBuf = lcdRxBuf;

    ok = SPI_transfer(gSpiHandle, &transaction);
    if (!ok)
    {
        UART_printf("SPI TX1 fail status=%u\n", transaction.status);
        while (1) {}
    }
}

/* Spi0TxBuffer — send len bytes via Spi0TxByte (per-byte) */
void Spi0TxBuffer(const uint8_t *buf, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++)
        Spi0TxByte(buf[i]);
}

/* GPIO hooks — toggle DC and RST lines */
void LcdDcLow(void)   { GPIO_write(LCD_DC,  GPIO_PIN_VAL_LOW);  }
void LcdDcHigh(void)  { GPIO_write(LCD_DC,  GPIO_PIN_VAL_HIGH); }
void LcdRstLow(void)  { GPIO_write(LCD_RST, GPIO_PIN_VAL_LOW);  }
void LcdRstHigh(void) { GPIO_write(LCD_RST, GPIO_PIN_VAL_HIGH); }

/* ====================================================================== *
 * Demo scenes — colour bands, shapes, text, pixel grid
 * Ported from the StarterWare reference AM3352_SPI_ILI9341/main.c
 * ====================================================================== */

static const uint16_t COLOR_BAND[] = {
    ILI9341_COLOR_RED,    ILI9341_COLOR_ORANGE, ILI9341_COLOR_YELLOW,
    ILI9341_COLOR_GREEN,  ILI9341_COLOR_CYAN,   ILI9341_COLOR_BLUE,
    ILI9341_COLOR_MAGENTA,ILI9341_COLOR_WHITE
};
#define COLOR_BAND_COUNT (sizeof(COLOR_BAND) / sizeof(COLOR_BAND[0]))

static void Demo_ClearAndHeader(const char *title, uint16_t bg)
{
    ILI9341_FillScreen(bg);
    ILI9341_DrawString(10, 10, title, ILI9341_COLOR_WHITE, bg);
    ILI9341_DrawString(10, 30, "AM3352 SPI0 + ILI9341",
                       ILI9341_COLOR_YELLOW, bg);
}

static void Demo_ColorBands(void)
{
    uint32_t i;
    uint16_t band_h;
    uint16_t y;

    Demo_ClearAndHeader("Color bands (8x40 px)", ILI9341_COLOR_BLACK);
    band_h = (ILI9341_HEIGHT - 50) / COLOR_BAND_COUNT;

    for (i = 0; i < COLOR_BAND_COUNT; i++)
    {
        y = (uint16_t)(50 + i * band_h);
        ILI9341_FillRect(0, y, ILI9341_WIDTH, band_h, COLOR_BAND[i]);
    }
    delay(1500);
}

static void Demo_Shapes(void)
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

static void Demo_Text(void)
{
    Demo_ClearAndHeader("Text", ILI9341_COLOR_BLUE);

    ILI9341_DrawString(10,  60,  "Hello AM3352!",  ILI9341_COLOR_WHITE,
                       ILI9341_COLOR_BLUE);
    ILI9341_DrawString(10,  80,  "ILI9341 240x320", ILI9341_COLOR_YELLOW,
                       ILI9341_COLOR_BLUE);
    ILI9341_DrawString(10, 100,  "SPI0 @ 10 MHz",   ILI9341_COLOR_GREEN,
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

static void Demo_PixelGrid(void)
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

/* ====================================================================== *
 * lcd_demo_task — initialise ILI9341, then cycle through demo scenes
 * ====================================================================== */
Void lcd_demo_task(UArg arg0, UArg arg1)
{
    UART_printf("\n=== ILI9341 LCD DEMO ===\n");
    UART_printf("SPI0 @ 12 MHz, HW CS, DC=P8_26, RST=P8_19\n\n");

    UART_printf("Calling ILI9341_Init()...\n");
    ILI9341_Init();
    UART_printf("ILI9341_Init() returned\n");

    UART_printf("Calling FillScreen(RED)...\n");
    ILI9341_FillScreen(ILI9341_COLOR_RED);
    UART_printf("FillScreen(RED) returned\n");

    UART_printf("starting demo cycle\n");

    while (1)
    {
        UART_printf("Demo_ColorBands...\n");
        Demo_ColorBands();
        UART_printf("Demo_Shapes...\n");
        Demo_Shapes();
        UART_printf("Demo_Text...\n");
        Demo_Text();
        UART_printf("Demo_PixelGrid...\n");
        Demo_PixelGrid();
        UART_printf("Cycle complete\n");
    }
}

/* ====================================================================== *
 * main() — hardware init sequence, then start SYS/BIOS
 *
 * Order:
 *   1. Board_init      — basic pinmux, module clocks, UART stdio
 *   2. Gpio0ClockEnable — enable GPIO0 module clock (CM_WKUP domain)
 *   3. SpiPinMuxSetup  — configure all SPI + LCD-GPIO pads
 *   4. GPIO_init       — TI GPIO driver init
 *   5. Spi0ClockEnable — enable SPI0 module clock (CM_PER domain)
 *   6. SpiInit         — open SPI0, configure bitrate / mode / CS
 *   7. Create task, start BIOS
 * ====================================================================== */
int main(void)
{
    Error_Block eb;
    Task_Params taskParams;

    Board_initGPIO();

    UART_printf("Board_init done\n");

    Gpio0ClockEnable();
    SpiPinMuxSetup();
    GPIO_init();

    UART_printf("GPIO init done\n");

    Spi0ClockEnable();
    SpiInit();

    if (gSpiHandle == NULL)
    {
        UART_printf("ERROR: SPI_init FAILED\n");
        while (1) {}
    }
    UART_printf("SPI init OK\n");

    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.priority = LCD_TASK_PRIORITY;
    taskParams.stackSize = LCD_TASK_STACK_SIZE;
    Task_create((Task_FuncPtr)lcd_demo_task, &taskParams, &eb);

    BIOS_start();
    return 0;
}
