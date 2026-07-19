/* ==========================================================================
 * SYS/BIOS + TI-RTOS - LVGL Music Demo on AM3352 (BeagleBone Black)
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
#include <ti/sysbios/knl/Clock.h>
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
 * CSL register access (for Spi0TxBufferFast)
 * -----------------------------------------------------------------------*/
#include <ti/csl/hw_types.h>
#include <ti/starterware/include/hw/soc_am335x.h>

/* -----------------------------------------------------------------------
 * LVGL headers
 * -----------------------------------------------------------------------*/
#include "lvgl.h"
#include "lv_port_disp.h"
#include "demos/music/lv_demo_music.h"

/* -----------------------------------------------------------------------
 * Constants
 * -----------------------------------------------------------------------*/
#define LCD_TASK_PRIORITY     (1)
#define LCD_TASK_STACK_SIZE   (0x2000)

/* SPI0 register offsets for burst writes (bypass PDK driver for speed) */
#define MCSPI_CH0_CHSTAT      (0x208)
#define MCSPI_CH0_TX          (0x20C)
#define MCSPI_CH0STAT_TXFFF   (0x04u)
#define MCSPI_CH0STAT_EOT     (0x40u)
#define MCSPI_CH0STAT_TXS     (0x02u)

/* -----------------------------------------------------------------------
 *  Forward declarations
 * -----------------------------------------------------------------------*/
Void lvgl_demo_task(UArg arg0, UArg arg1);
Void lvgl_tick(UArg arg0);

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

/* delay() - RTOS-aware, yields the task */
void delay(uint32_t ms)
{
    Task_sleep(ms);
}

/* Spi0TxByte - send 1 byte via PDK SPI driver with hardware CS */
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

/* Spi0TxBuffer - send len bytes via PDK SPI driver (chunked, polling) */
void Spi0TxBuffer(const uint8_t *buf, uint32_t len)
{
    while (len > 0)
    {
        uint32_t chunk = (len > SPI_LCD_CHUNK) ? SPI_LCD_CHUNK : len;
        SPI_Transaction transaction;

        transaction.count = chunk;
        transaction.txBuf = (void *)buf;
        transaction.rxBuf = lcdRxBuf;

        SPI_transfer(gSpiHandle, &transaction);

        buf += chunk;
        len -= chunk;
    }
}

/* GPIO hooks - toggle DC and RST lines */
void LcdDcLow(void)   { GPIO_write(LCD_DC,  GPIO_PIN_VAL_LOW);  }
void LcdDcHigh(void)  { GPIO_write(LCD_DC,  GPIO_PIN_VAL_HIGH); }
void LcdRstLow(void)  { GPIO_write(LCD_RST, GPIO_PIN_VAL_LOW);  }
void LcdRstHigh(void) { GPIO_write(LCD_RST, GPIO_PIN_VAL_HIGH); }

/* ====================================================================== *
 * Spi0TxBufferFast - SPI burst transfer for LVGL flush callback
 *
 * Uses direct register writes (bypass PDK driver) to keep CS asserted
 * for the entire buffer, necessary for ILI9341 GRAM pixel bursts.
 * ====================================================================== */
void Spi0TxBufferFast(const uint8_t *buf, uint32_t len)
{
    if (len == 0) return;

    uint32_t i = 0;
    while (i < len)
    {
        uint32_t burst = (len - i > 192u) ? 192u : (len - i);
        uint32_t j;
        for (j = 0; j < burst; j++)
            HWREG(SOC_SPI_0_REGS + MCSPI_CH0_TX) = buf[i + j];
        i += burst;
        if (i < len)
        {
            int timeout = 100000;
            while ((HWREG(SOC_SPI_0_REGS + MCSPI_CH0_CHSTAT) & MCSPI_CH0STAT_TXFFF)
                   && --timeout) {}
        }
    }
    {
        int timeout = 500000;
        while (!(HWREG(SOC_SPI_0_REGS + MCSPI_CH0_CHSTAT) & MCSPI_CH0STAT_EOT)
               && --timeout) {}
    }
    (void)HWREG(SOC_SPI_0_REGS + MCSPI_CH0_TX);
}

/* ====================================================================== *
 * lvgl_tick - SYS/BIOS Clock callback for LVGL internal timekeeping
 * ====================================================================== */
Void lvgl_tick(UArg arg0)
{
    lv_tick_inc(5);
}

/* ====================================================================== *
 * lvgl_demo_task - ILI9341 + LVGL init, then lv_demo_music
 * ====================================================================== */
Void lvgl_demo_task(UArg arg0, UArg arg1)
{
    UART_printf("\n=== LVGL DEMO ===\n");

    ILI9341_Init();
    lv_init();
    lv_port_disp_init();

    UART_printf("Starting Music Demo...\n");
    lv_demo_music();
    UART_printf("Music Demo running\n");

    while (1)
    {
        lv_timer_handler();
        Task_sleep(5);
    }
}

/* ====================================================================== *
 * main() - hardware init sequence, then start SYS/BIOS
 *
 * Order:
 *   1. Board_init      - basic pinmux, module clocks, UART stdio
 *   2. Gpio0ClockEnable - enable GPIO0 module clock (CM_WKUP domain)
 *   3. SpiPinMuxSetup  - configure all SPI + LCD-GPIO pads
 *   4. GPIO_init       - TI GPIO driver init
 *   5. Spi0ClockEnable - enable SPI0 module clock (CM_PER domain)
 *   6. SpiInit         - open SPI0, configure bitrate / mode / CS
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

    /* Create LVGL 5-ms tick clock */
    {
        Clock_Params clockParams;
        Clock_Params_init(&clockParams);
        clockParams.period = 5;
        clockParams.startFlag = TRUE;
        Clock_create((Clock_FuncPtr)lvgl_tick, 5, &clockParams, &eb);
    }

    /* Create LVGL demo task */
    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.priority = LCD_TASK_PRIORITY;
    taskParams.stackSize = LCD_TASK_STACK_SIZE;
    Task_create((Task_FuncPtr)lvgl_demo_task, &taskParams, &eb);

    BIOS_start();
    return 0;
}
