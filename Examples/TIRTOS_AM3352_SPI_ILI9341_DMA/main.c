/**
 * ============================================================================
 * @file    main.c
 * @brief   AM3352 SPI1 ILI9341 LCD Demo with DMA Transfer (TIRTOS)
 * ============================================================================
 *
 *  Hardware Platform: AM3352 (Antminer L3+ board)
 *  Display:           ILI9341 2.8" TFT (240x320), SPI1 mode
 *  SPI1 Pins:         SCK=P9_31, MOSI=P9_30, CS=P9_28
 *  GPIO Pins:         DC=P8_26 (GPIO0[22]), RST=P8_19 (GPIO1[29])
 *  Clock Speed:       24 MHz
 *  DMA:               EDMA3 used for SPI TX transfers
 *
 *  Architecture:
 *  ┌─────────────────────────────────────────────────────────────┐
 *  │                      main() - BIOS Start                     │
 *  │   1. Create LCD Demo Task (priority 1, stack 0x2000)         │
 *  │   2. Initialize Board (pinmux, clock, UART)                  │
 *  │   3. Start BIOS (runs lcd_demo_task)                         │
 *  └───────��─────────────────��───────────────────────────────────┘
 *                              │
 *                              ▼
 *  ┌─────────────────────────────────────────────────────────────┐
 *  │              lcd_demo_task() — Main Application              │
 *  │   1. Pinmux override for SPI1 + GPIO0 clock enable          │
 *  │   2. Init SPI + EDMA3                                       │
 *  │   3. Create semaphores (cbSem for callback, lcdSem for LCD)  │
 *  │   4. Open SPI1 at 24 MHz, callback mode                     │
 *  │   5. LCD Reset sequence (DC/RST GPIO toggle)                │
 *  │   6. ILI9341_Init() — send init commands                    │
 *  │   7. Loop: run graphics demos forever                       │
 *  └─────────────────────────────────────────────────────────────┘
 *                              │
 *                              ▼
 *  ┌─────────────────────────────────────────────────────────────┐
 *  │              SPI_callback() — DMA Completion ISR             │
 *  │   Called by SPI driver when TX transfer finishes via EDMA3  │
 *  │   Posts cbSem and/or lcdSem to unblock waiting caller        │
 *  └─────────────────────────────────────────────────────────────┘
 *                              │
 *                              ▼
 *  ┌─────────────────────────────────────────────────────────────┐
 *  │         Spi1TxByte() / Spi1TxBuffer() — SPI Helpers          │
 *  │   Blocking SPI transfer wrappers using DMA + semaphore wait  │
 *  │   Handles cache maintenance (wb/Inv) for DMA coherence       │
 *  └─────────────────────────────────────────────────────────────┘
 *                              │
 *                              ▼
 *  ┌───────────────────────────���─────────────────────────────────┐
 *  │              simple_gfx.* — Graphics Demos                   │
 *  │   - Color bands (8 colored stripes)                         │
 *  │   - Shapes (rect, circle, line)                             │
 *  │   - Text (uppercase, lowercase, digits)                     │
 *  │   - Pixel grid (checkerboard pattern)                       │
 *  └─────────────────────────────────────────────────────────────┘
 *
 *  File Organization:
 *    main.c      — SPI init, DMA callbacks, task entry points
 *    ili9341.c   — LCD driver (commands, pixel drawing primitives)
 *    simple_gfx.c— Higher-level demo scenes (shapes, text, patterns)
 *    utils.c     — Low-level helpers (delay, DC/RST GPIO control)
 *
 * ============================================================================
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
#include "ili9341.h"

/* Simple graphics demos */
#include "simple_gfx.h"

/* Utility helpers (GPIO, delay) */
#include "utils.h"

/* ============================================================================
 * Constants & Macros
 * ============================================================================ */

#define SPI_TIMEOUT_VALUE     5000          /* Transfer timeout in ms      */
#define SPI_MSG_LENGTH        2             /* Bytes per transfer          */
#define MCSPI_TEST_CHN        0             /* SPI channel used            */

#define LCD_TASK_PRIORITY     (1)           /* Task priority (lower = lower) */
#define LCD_TASK_STACK_SIZE   (0x2000)      /* Task stack size (8 KB)       */

#define SPI_LCD_CHUNK         (65535)       /* Max chunk for SPI transfer   */

/* ============================================================================
 * Global Variables
 * ============================================================================ */

/* EDMA3 handle — used by SPI driver for DMA transfers */
static EDMA3_RM_Handle gEdmaHandle = NULL;

/* RX/TX buffers for SPI loopback test (128 bytes, cache-aligned) */
unsigned char masterRxBuffer[128] __attribute__ ((aligned (128)));
unsigned char masterTxBuffer[128] __attribute__ ((aligned (128)));

/* SPI transaction object (reused across transfers) */
SPI_Transaction   transaction;

/* Semaphore params for callback-based blocking lock */
SemaphoreP_Params cbSemParams;
SemaphoreP_Handle cbSem = NULL;

/* LCD-specific synchronization */
static SemaphoreP_Handle lcdSem = NULL;       /* Signals LCD transfer done */
static bool lcdTransferDone = false;          /* Flag to avoid double-post */

/* RX buffer for LCD SPI transfers (mirrors TX for DMA coherence) */
static uint8_t lcdRxBuf[SPI_LCD_CHUNK];

/* SPI handle — shared between LCD and potential loopback tests */
SPI_Handle gSpiLcdHandle = NULL;

/* ============================================================================
 * Function Prototypes
 * ============================================================================ */

/* EDMA3 initialization */
static EDMA3_RM_Handle MCSPIApp_edmaInit(void);

/* SPI DMA transfer callback — posted semaphore on completion */
void SPI_callback(SPI_Handle handle, SPI_Transaction *transaction);

/* SPI peripheral configuration (FIFO, DMA, channel settings) */
static void SPI_initConfig(uint32_t instance);

/* LCD demo task — entry point for the main application */
Void lcd_demo_task(UArg arg0, UArg arg1);

/* SPI write helpers — blocking via DMA callback semaphore */
void Spi1TxByte(uint8_t b);
void Spi1TxBuffer(const uint8_t *buf, uint32_t len);

/* ============================================================================
 * Function Definitions
 * ============================================================================ */

/**
 * MCSPIApp_edmaInit — Initialize EDMA3 driver for DMA transfers
 *
 * Returns cached EDMA3 handle if already initialized, otherwise
 * creates a new handle via edma3init().
 */
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

/**
 * SPI_callback — DMA transfer completion interrupt handler
 *
 * Posted by SPI driver when EDMA3 finishes a transfer.
 * Unblocks the caller waiting on cbSem (general) or lcdSem (LCD).
 */
void SPI_callback(SPI_Handle handle, SPI_Transaction *transaction)
{
    /* Signal general callback semaphore */
    if (cbSem != NULL) {
        SPI_osalPostLock(cbSem);
    }
    /* Signal LCD semaphore only once per transfer */
    if (lcdSem != NULL && !lcdTransferDone) {
        lcdTransferDone = true;
        SPI_osalPostLock(lcdSem);
    }
}

/**
 * SPI_initConfig — Configure SPI peripheral hardware attributes
 *
 * Sets up:
 *   - EDMA3 for DMA mode
 *   - FIFO trigger levels (half-depth)
 *   - Channel 0: half-clock, 6-line mode, TX+RX
 */
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

/**
 * Spi1TxByte — Send a single byte via SPI1 (DMA + blocking)
 *
 * 1. Invalidate cache for TX byte and RX buffer
 * 2. Start SPI transfer
 * 3. Wait for DMA callback via lcdSem
 * 4. Invalidate RX buffer after transfer complete
 */
void Spi1TxByte(uint8_t b)
{
    SPI_Transaction tx;
    bool ok;

    lcdTransferDone = false;
    CacheP_wb((void *)&b, 1);
    CacheP_wbInv((void *)lcdRxBuf, 1);

    tx.count = 1;
    tx.arg = NULL;
    tx.txBuf = (void *)&b;
    tx.rxBuf = (void *)lcdRxBuf;

    ok = SPI_transfer(gSpiLcdHandle, &tx);
    if (!ok) {
        while (1) {}
    }

    SPI_osalPendLock(lcdSem, SPI_TIMEOUT_VALUE);
    CacheP_Inv((void *)lcdRxBuf, 1);
}

/**
 * Spi1TxBuffer — Send a buffer via SPI1 in chunks (DMA + blocking)
 *
 * Splits large buffers into SPI_LCD_CHUNK-sized pieces to stay
 * within DMA transfer limits. Each chunk waits for completion.
 */
void Spi1TxBuffer(const uint8_t *buf, uint32_t len)
{
    while (len > 0) {
        uint32_t chunk = (len > SPI_LCD_CHUNK) ? SPI_LCD_CHUNK : len;
        SPI_Transaction tx;
        bool ok;

        lcdTransferDone = false;
        CacheP_wb((void *)buf, (int32_t)chunk);
        CacheP_wbInv((void *)lcdRxBuf, SPI_LCD_CHUNK);

        tx.count = chunk;
        tx.arg = NULL;
        tx.txBuf = (void *)buf;
        tx.rxBuf = (void *)lcdRxBuf;

        ok = SPI_transfer(gSpiLcdHandle, &tx);
        if (!ok) {
            while (1) {}
        }

        SPI_osalPendLock(lcdSem, SPI_TIMEOUT_VALUE);
        CacheP_Inv((void *)lcdRxBuf, (int32_t)chunk);

        buf += chunk;
        len -= chunk;
    }
}

/**
 * lcd_demo_task — Main application task
 *
 * Flow:
 *   1. Print banner to UART
 *   2. Override pinmux for SPI1 + enable GPIO0 clock
 *   3. Initialize SPI + EDMA3
 *   4. Create semaphores for DMA callback synchronization
 *   5. Open SPI1 at 24 MHz in callback mode
 *   6. Perform LCD hardware reset (RST + DC timing)
 *   7. Send ILI9341 init command sequence
 *   8. Flash red screen briefly as visual confirmation
 *   9. Enter infinite loop cycling through graphics demos
 */
Void lcd_demo_task(UArg arg0, UArg arg1)
{
    SPI_Handle        spi;
    SPI_Params        spiParams;
    uint32_t          instance;

    /* --- Banner --- */
    SPI_log("\r\n=== ILI9341 LCD DEMO ===\r\n");
    SPI_log("SPI1 DMA, DC=P8_26, RST=P8_19\r\n\r\n");

    /* --- Pinmux override for SPI1 + GPIO0 --- */
    HWREG(SOC_CONTROL_REGS + 0x87C) = 0x00000007;
    HWREG(SOC_CONTROL_REGS + 0x820) = 0x00000007;

    Gpio0ClockEnable();
    GPIO_init();

    /* --- SPI subsystem --- */
    SPI_init();

    instance = (uint32_t)BOARD_MCSPI_MASTER_INSTANCE - 1;
    SPI_initConfig(instance);

    SPI_osalSemParamsInit(&cbSemParams);
    cbSemParams.mode = SemaphoreP_Mode_BINARY;
    cbSem = SPI_osalCreateBlockingLock(0, &cbSemParams);

    lcdSem = SemaphoreP_create(0, NULL);

    /* --- SPI parameters: 24 MHz, callback mode --- */
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

    /* --- LCD Hardware Reset Sequence --- */
    /* Hold RST high, then low for 10ms, then high again */
    LcdRstHigh();
    Task_sleep(10);
    LcdRstLow();
    Task_sleep(10);
    LcdRstHigh();
    /* Wait for LCD controller to wake up */
    Task_sleep(120);

    /* --- ILI9341 Initialization --- */
    ILI9341_Init();
    SPI_log("ILI9341_Init() done\r\n");

    /* Visual confirmation: flash red screen */
    ILI9341_FillScreen(ILI9341_COLOR_RED);
    Task_sleep(500);

    /* --- Main Demo Loop --- */
    while (1) {
        simple_gfx_demo_color_bands();
        simple_gfx_demo_shapes();
        simple_gfx_demo_text();
        simple_gfx_demo_pixel_grid();
    }
}

/**
 * main — Application entry point
 *
 * 1. Create LCD demo task
 * 2. Initialize board (pinmux, clocks, UART console)
 * 3. Start TIRTOS BIOS scheduler
 */
int main(void)
{
    Task_Handle lcdTask;
    Error_Block eb;
    Task_Params lcdTaskParams;

    Error_init(&eb);
    Task_Params_init(&lcdTaskParams);
    lcdTaskParams.priority = LCD_TASK_PRIORITY;
    lcdTaskParams.stackSize = LCD_TASK_STACK_SIZE;

    lcdSem = SemaphoreP_create(0, NULL);

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
