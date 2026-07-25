/**
 * @file    main.c
 * @brief   SPI Master Loopback Test + ILI9341 LCD Demo with DMA Transfer
 *
 * @details
 *  This file implements:
 *  1. SPI1 loopback test using DMA (TX + RX with EDMA3)
 *  2. ILI9341 LCD demo on SPI1 (TX only, blocking via DMA callback)
 *
 *  Hardware: AM3352 (Antminer L3+ board)
 *  Board:     BOARD_MCSPI_MASTER_INSTANCE = 1 (SPI1)
 *  SPI1 Pins: SCK=P9_31, MOSI=P9_30, CS=P9_28
 *  GPIO Pins: DC=P8_26, RST=P8_19
 */

#include <stdio.h>
#include <string.h>

#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/BIOS.h>
#include <xdc/runtime/Error.h>

#include <ti/osal/osal.h>
#include <ti/drv/spi/soc/SPI_soc.h>
#include <ti/drv/spi/src/SPI_osal.h>
#include <ti/drv/spi/SPI.h>
#include "SPI_log.h"
#include <ti/board/board.h>

#include <ti/osal/CacheP.h>
#include <ti/sdo/edma3/drv/edma3_drv.h>
#include <ti/sdo/edma3/rm/edma3_rm.h>
#include <ti/sdo/edma3/rm/sample/bios6_edma3_rm_sample.h>

/* Pinmux registers */
#include <ti/csl/hw_types.h>
#include <ti/starterware/include/hw/soc_am335x.h>
#include <ti/starterware/include/hw/hw_control_am335x.h>

/* GPIO driver */
#include <ti/drv/gpio/GPIO.h>
#include "GPIO_board.h"

/* ILI9341 LCD driver */
#include "ili9341/ili9341.h"
#include "ili9341/delay.h"
#include "ili9341/fonts.h"

/* Local modules */
#include "utils.h"
#include "simple_gfx.h"

/* ============================================================================
 * Constants & Macros
 * ============================================================================ */

#define SPI_TIMEOUT_VALUE     5000          /* Transfer timeout in ms      */
#define SPI_MSG_LENGTH        2             /* Bytes per transfer          */
#define MCSPI_TEST_CHN        0             /* SPI channel used            */

#define LCD_TASK_PRIORITY     (1)
#define LCD_TASK_STACK_SIZE   (0x2000)

#define SPI_LCD_CHUNK         (65535)

/* Max bytes to batch SPI byte sends for LCD init commands */
#define SPI_LCD_BATCH_SIZE    32

/* ============================================================================
 * Global Variables
 * ============================================================================ */

static EDMA3_RM_Handle gEdmaHandle = NULL;

unsigned char masterRxBuffer[128] __attribute__ ((aligned (128)));
unsigned char masterTxBuffer[128] __attribute__ ((aligned (128)));

SPI_Transaction   transaction;

SemaphoreP_Params cbSemParams;
SemaphoreP_Handle cbSem = NULL;

/* LCD task semaphore */
SemaphoreP_Handle lcdSem = NULL;
bool lcdTransferDone = false;

/* SPI handle for LCD (shared with DMA loopback) */
SPI_Handle gSpiLcdHandle = NULL;

/* ============================================================================
 * Function Prototypes
 * ============================================================================ */

static EDMA3_RM_Handle MCSPIApp_edmaInit(void);
void SPI_callback(SPI_Handle handle, SPI_Transaction *transaction);
static void SPI_initConfig(uint32_t instance);

Void lcd_demo_task(UArg arg0, UArg arg1);

/* ============================================================================
 * Function Definitions
 * ============================================================================ */

static EDMA3_RM_Handle MCSPIApp_edmaInit(void)
{
    EDMA3_DRV_Result edmaResult = EDMA3_DRV_E_INVALID_PARAM;
    uint32_t         edma3Id = 0;

    if (gEdmaHandle != NULL) {
        return (gEdmaHandle);
    }

    gEdmaHandle = (EDMA3_RM_Handle)edma3init(edma3Id, &edmaResult);

    if (edmaResult != EDMA3_DRV_SOK) {
        System_printf("\nEDMA driver initialization FAIL\n");
    } else {
        System_printf("\nEDMA driver initialization PASS.\n");
    }
    return(gEdmaHandle);
}

void SPI_callback(SPI_Handle handle, SPI_Transaction *transaction)
{
    if (cbSem != NULL) {
        SPI_osalPostLock(cbSem);
    }
    if (lcdSem != NULL && !lcdTransferDone) {
        lcdTransferDone = true;
        SPI_osalPostLock(lcdSem);
    }
}

static void SPI_initConfig(uint32_t instance)
{
    SPI_HWAttrs spi_cfg;

    SPI_socGetInitCfg(instance, &spi_cfg);

    spi_cfg.enableIntr = false;
    spi_cfg.edmaHandle = MCSPIApp_edmaInit();
    spi_cfg.dmaMode    = TRUE;

    spi_cfg.chnCfg[MCSPI_TEST_CHN].tcs              = MCSPI_CS_TCS_0PNT5_CLK;
    spi_cfg.chnCfg[MCSPI_TEST_CHN].dataLineCommMode = MCSPI_DATA_LINE_COMM_MODE_6;
    spi_cfg.chnCfg[MCSPI_TEST_CHN].trMode           = MCSPI_TX_RX_MODE;
    spi_cfg.initDelay                    = MCSPI_INITDLY_0;
    spi_cfg.rxTrigLvl                    = MCSPI_RX_TX_FIFO_SIZE / 2;
    spi_cfg.txTrigLvl                    = MCSPI_RX_TX_FIFO_SIZE / 2;

    SPI_socSetInitCfg(instance, &spi_cfg);
}

Void lcd_demo_task(UArg arg0, UArg arg1)
{
    SPI_Handle        spi;
    SPI_Params        spiParams;
    uint32_t          instance;

    SPI_log("\r\n=== ILI9341 LCD DEMO ===\r\n");
    SPI_log("SPI1 DMA, DC=P8_26, RST=P8_19\r\n\r\n");

    HWREG(SOC_CONTROL_REGS + 0x87C) = 0x00000007;
    HWREG(SOC_CONTROL_REGS + 0x820) = 0x00000007;

    Gpio0ClockEnable();
    GPIO_init();

    SPI_init();

    instance = (uint32_t)BOARD_MCSPI_MASTER_INSTANCE - 1;
    SPI_initConfig(instance);

    SPI_osalSemParamsInit(&cbSemParams);
    cbSemParams.mode = SemaphoreP_Mode_BINARY;
    cbSem = SPI_osalCreateBlockingLock(0, &cbSemParams);

    lcdSem = SemaphoreP_create(0, NULL);

    SPI_Params_init(&spiParams);
    spiParams.transferMode = SPI_MODE_CALLBACK;
    spiParams.transferCallbackFxn = SPI_callback;
    spiParams.transferTimeout = SPI_TIMEOUT_VALUE;
    spiParams.bitRate = 24000000;

    spi = SPI_open(instance, &spiParams);
    gSpiLcdHandle = spi;

    if (spi == NULL) {
        SPI_log("Error initializing SPI for LCD\r\n");
        while (true) {
            Task_sleep(50000);
        }
    } else {
        SPI_log("SPI initialized for LCD\r\n");
    }

    Task_sleep(100);

    LcdRstHigh();
    Task_sleep(10);
    LcdRstLow();
    Task_sleep(10);
    LcdRstHigh();
    Task_sleep(120);

    ILI9341_Init();
    SPI_log("ILI9341_Init() done\r\n");

    ILI9341_FillScreen(ILI9341_COLOR_RED);
    Task_sleep(500);

    while (1) {
        simple_gfx_demo_color_bands();
        simple_gfx_demo_shapes();
        simple_gfx_demo_text();
        simple_gfx_demo_pixel_grid();
    }
}

int main(void)
{
    Task_Handle lcdTask;
    Error_Block eb;
    Task_Params lcdTaskParams;

    Error_init(&eb);
    Task_Params_init(&lcdTaskParams);
    lcdTaskParams.priority = LCD_TASK_PRIORITY;
    lcdTaskParams.stackSize = LCD_TASK_STACK_SIZE;

    lcdTask = Task_create((Task_FuncPtr)lcd_demo_task, &lcdTaskParams, &eb);

    if (lcdTask == NULL) {
        System_printf("LCD Task_create() failed!\n");
        BIOS_exit(0);
    }

    Board_initCfg boardCfg = BOARD_INIT_PINMUX_CONFIG |
                              BOARD_INIT_MODULE_CLOCK  |
                              BOARD_INIT_UART_STDIO;

    if (Board_init(boardCfg) != BOARD_SOK) {
        return (0);
    }

    BIOS_start();

    return (0);
}
