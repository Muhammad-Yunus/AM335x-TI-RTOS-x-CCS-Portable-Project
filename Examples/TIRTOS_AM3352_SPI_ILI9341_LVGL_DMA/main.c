
#include <stdio.h>
#include <string.h>

#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
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


/* Utility helpers (GPIO, delay) */
#include "utils.h"

/* LVGL headers */
#include "lv_conf.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "demos/music/lv_demo_music.h"

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

/* SPI write helpers — blocking via DMA callback semaphore */
void Spi1TxByte(uint8_t b);
void Spi1TxBuffer(const uint8_t *buf, uint32_t len);

/* LVGL tick clock callback */
Void lvgl_tick(UArg arg0);

/* LVGL demo task */
Void lvgl_demo_task(UArg arg0, UArg arg1);

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
 * lvgl_tick — SYS/BIOS Clock callback for LVGL internal timekeeping
 */
Void lvgl_tick(UArg arg0)
{
    lv_tick_inc(5);
}

/**
 * lvgl_demo_task — ILI9341 + LVGL Music Demo
 */
Void lvgl_demo_task(UArg arg0, UArg arg1)
{
    SPI_Handle        spi;
    SPI_Params        spiParams;
    uint32_t          instance;

    /* --- Pinmux override for SPI1 + GPIO0 --- */
    HWREG(SOC_CONTROL_REGS + 0x87C) = 0x00000007;
    HWREG(SOC_CONTROL_REGS + 0x820) = 0x00000007;

    Gpio0ClockEnable();
    GPIO_init();

    /* --- SPI subsystem --- */
    SPI_init();

    instance = (uint32_t)BOARD_MCSPI_MASTER_INSTANCE - 1;
    SPI_initConfig(instance);

    lcdSem = SemaphoreP_create(0, NULL);

    /* --- SPI parameters: 40 MHz, callback mode --- */
    SPI_Params_init(&spiParams);
    spiParams.transferMode = SPI_MODE_CALLBACK;
    spiParams.transferCallbackFxn = SPI_callback;
    spiParams.transferTimeout = SPI_TIMEOUT_VALUE;
    spiParams.bitRate = 40000000;

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
    LcdRstHigh();
    Task_sleep(10);
    LcdRstLow();
    Task_sleep(10);
    LcdRstHigh();
    Task_sleep(120);

    /* --- LVGL Init --- */
    SPI_log("\r\n=== LVGL MUSIC DEMO ===\r\n");

    ILI9341_Init();
    lv_init();
    lv_port_disp_init();

    SPI_log("Starting Music Demo...\r\n");
    lv_demo_music();
    SPI_log("Music Demo running\r\n");

    while (1) {
        lv_timer_handler();
        Task_sleep(5);
    }
}

/**
 * main — Application entry point
 *
 * 1. Initialize board (pinmux, clocks, UART console)
 * 2. Create LVGL 5-ms tick clock
 * 3. Create LVGL demo task
 * 4. Start TIRTOS BIOS scheduler
 */
int main(void)
{
    Task_Handle lvglTask;
    Error_Block eb;
    Task_Params lvglTaskParams;
    Clock_Params clockParams;

    Error_init(&eb);
    Task_Params_init(&lvglTaskParams);
    lvglTaskParams.priority = LCD_TASK_PRIORITY;
    lvglTaskParams.stackSize = LCD_TASK_STACK_SIZE;

    /* Create LVGL 5-ms tick clock */
    Clock_Params_init(&clockParams);
    clockParams.period = 5;
    clockParams.startFlag = TRUE;
    Clock_create((Clock_FuncPtr)lvgl_tick, 5, &clockParams, &eb);

    lvglTask = Task_create((Task_FuncPtr)lvgl_demo_task, &lvglTaskParams, &eb);

    if (lvglTask == NULL) {
        System_printf("LVGL Task_create() failed!\n");
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
