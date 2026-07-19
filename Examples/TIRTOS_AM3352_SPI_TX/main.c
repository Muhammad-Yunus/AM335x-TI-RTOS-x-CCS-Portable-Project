/* ==========================================================================
 * SYS/BIOS + TI-RTOS - SPI TX Display Driver for AM3352 (BeagleBone Black)
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
 * Constants
 * -----------------------------------------------------------------------*/
#define BLINK_DELAY_MS        (500)   /* Task sleep interval */

/* -----------------------------------------------------------------------
 * Forward declarations
 * -----------------------------------------------------------------------*/
Void spi_tx_task(UArg arg0, UArg arg1);

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

/* -----------------------------------------------------------------------
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
 * -----------------------------------------------------------------------*/
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
    taskParams.priority = 1;
    taskParams.stackSize = 0x1000;
    Task_create((Task_FuncPtr)spi_tx_task, &taskParams, &eb);

    BIOS_start();
    return 0;
}

/* -----------------------------------------------------------------------
 * spi_tx_task - periodic GPIO toggle + SPI transfer
 *
 * Each cycle:
 *   - Drive DC + RST high, sleep
 *   - Drive DC + RST low,  sleep
 *   - Transmit 1 byte over SPI (CS handled by HW)
 * -----------------------------------------------------------------------*/
Void spi_tx_task(UArg arg0, UArg arg1)
{
    SPI_Transaction transaction;
    uint32_t count = 0;

    transaction.count = SPI_DATA_COUNT;
    transaction.txBuf = gTxBuffer;
    transaction.rxBuf = gRxBuffer;

    UART_printf("\n=== SPI TX DEMO ===\n");
    UART_printf("CLK=P9_22 MOSI=P9_18 CS=P9_17(HW_SPI) DC=P8_26 RST=P8_19\n\n");

    while (1)
    {
        GPIO_write(LCD_DC,  GPIO_PIN_VAL_HIGH);
        GPIO_write(LCD_RST, GPIO_PIN_VAL_HIGH);
        Task_sleep(BLINK_DELAY_MS);

        GPIO_write(LCD_DC,  GPIO_PIN_VAL_LOW);
        GPIO_write(LCD_RST, GPIO_PIN_VAL_LOW);
        Task_sleep(BLINK_DELAY_MS);

         /* SPI0 CS is hardware-controlled, stays asserted
         * across the transfer and deasserts at SPI_close() */
        SPI_transfer(gSpiHandle, &transaction);
        count++;
        if (count % 100 == 0)
        {
            UART_printf("SPI TX=%u RX=%02x\n", count, gRxBuffer[0]);
        }
    }
}
