/**
 * @file    main.c
 * @brief   SPI Master Loopback Test with DMA Transfer
 *
 * @details
 *  This file implements a loopback test on MCSPI (Multi-Channel Serial
 *  Peripheral Interface) using Master mode. The transmitted data is
 *  2 bytes with value 0xAF, which is looped back to the RX pin and read back
 *  using DMA for transfer efficiency.
 *
 *  Features:
 *  - SPI Master Mode with DMA transfer (EDMA3)
 *  - Loopback TX → RX (MISO pin connected to MOSI hardware-wise)
 *  - Callback-based transfer with semaphore synchronization
 *  - Continuous loop with 50ms delay between transfers
 *  - Cache management (Write Back / Invalidate) for data integrity
 *
 *  Hardware: AM335x (BeagleBone Black / AM3352 EVM)
 *  Board:     BOARD_MCSPI_MASTER_INSTANCE = 1 (SPI1)
 *  Pin:       MISO loopback to MOSI hardware-wise
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

/* ============================================================================
 * Constants & Macros
 * ============================================================================ */

#define SPI_TIMEOUT_VALUE     5000          /* Transfer timeout in ms      */
#define SPI_MSG_LENGTH        2             /* Bytes per transfer          */
#define MCSPI_TEST_CHN        0             /* SPI channel used            */

/* ============================================================================
 * Global Variables
 * ============================================================================ */

static EDMA3_RM_Handle gEdmaHandle = NULL;   /* EDMA3 Resource Manager Handle */

/* RX buffer - 128-byte aligned for cache line requirement */
unsigned char masterRxBuffer[128] __attribute__ ((aligned (128)));

/* TX buffer - 128-byte aligned for cache line requirement */
unsigned char masterTxBuffer[128] __attribute__ ((aligned (128)));

/* SPI Transaction structure */
SPI_Transaction   transaction;

/* Semaphore for callback synchronization */
SemaphoreP_Params cbSemParams;
SemaphoreP_Handle cbSem = NULL;

/* ============================================================================
 * Function Prototypes
 * ============================================================================ */

/**
 * @brief   Initialize EDMA3 driver
 * @return  EDMA3_RM_Handle, NULL on failure
 *
 * This function initializes EDMA3 (Enhanced Direct Memory Access) which
 * is used by the SPI driver for data transfer without CPU intervention.
 */
static EDMA3_RM_Handle MCSPIApp_edmaInit(void);

/**
 * @brief   SPI Transfer callback function
 * @param   handle      SPI handle
 * @param   transaction Pointer to SPI_Transaction
 *
 * Called by the SPI driver when the transfer is complete. The semaphore is
 * posted to signal that the DMA transfer has finished.
 */
void SPI_callback(SPI_Handle handle, SPI_Transaction *transaction);

/**
 * @brief   Configure SPI hardware attributes
 * @param   instance    SPI instance (0-based index)
 *
 * Configures SPI settings including:
 * - EDMA mode enabled
 * - Chip select timing (TCS)
 * - Data line communication mode
 * - TX/RX mode (simultaneous)
 * - FIFO trigger levels
 */
static void SPI_initConfig(uint32_t instance);

/**
 * @brief   Perform SPI master-slave transfer via DMA
 * @param   spi         SPI handle
 * @param   xferLen     Length of data to transfer (bytes)
 * @return  true if transfer succeeds, false on failure/timeout
 *
 * This function:
 * 1. Prepares TX/RX buffers
 * 2. Cache management (WB/WB-Inv)
 * 3. Starts SPI transfer via DMA
 * 4. Waits for callback via semaphore
 * 5. Invalidates RX buffer cache
 * 6. Prints RX result to UART
 */
static bool SPI_test_mst_slv_xfer(SPI_Handle spi, uint32_t xferLen);

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

static bool SPI_test_mst_slv_xfer(SPI_Handle spi, uint32_t xferLen)
{
    bool            transferOK;
    uint32_t        xferBytes;
    bool            ret = false;

    /* Clear RX buffer */
    memset(masterRxBuffer, 0, sizeof(masterRxBuffer));

    /* Cache Write Back: synchronize TX buffer before DMA */
    CacheP_wb((void *)masterTxBuffer, (int32_t)xferLen);
    /* Cache Write Back + Invalidate: prepare RX buffer for DMA write */
    CacheP_wbInv((void *)masterRxBuffer, (int32_t)sizeof(masterRxBuffer));

    /* Setup SPI transaction */
    transaction.count = xferLen;
    transaction.arg = NULL;
    transaction.txBuf = (void *)masterTxBuffer;
    transaction.rxBuf = (void *)masterRxBuffer;

    /* Start SPI transfer (non-blocking, DMA works in background) */
    transferOK = SPI_transfer(spi, &transaction);

    if(transferOK) {
        /* Wait for callback from SPI driver (blocking) */
        if (SPI_osalPendLock(cbSem, SPI_TIMEOUT_VALUE) != SemaphoreP_OK) {
            SPI_log("Transfer timeout!\n");
            goto Err;
        }

        xferBytes = transaction.count;

        /* Invalidate RX buffer cache so new DMA data can be read */
        CacheP_Inv((void *)masterRxBuffer, (int32_t)xferBytes);

        /* Print RX result to UART */
        SPI_log("RX: ");
        for (uint32_t i = 0; i < xferBytes; i++) {
            SPI_log("0x%02X ", masterRxBuffer[i]);
        }
        SPI_log("\n");
    } else {
        SPI_log("Unsuccessful SPI transfer\n");
        goto Err;
    }

    ret = true;

Err:
    return (ret);
}

/**
 * @brief   Main task function for SPI loopback test
 *
 * Flow:
 * 1. Print banner and description
 * 2. Initialize SPI and EDMA
 * 3. Open SPI device
 * 4. Setup TX buffer with 0xAF (2 bytes)
 * 5. Infinite loop: SPI transfer + 50ms delay
 */
Void masterTaskFxn (UArg arg0, UArg arg1)
{
    SPI_Handle        spi;
    SPI_Params        spiParams;
    uint32_t          instance;

    /* Print banner */
    SPI_log("\r\n ================================ \r\n");
    SPI_log("   SPI DMA Loopback Test v1.0     \r\n");
    SPI_log(" ================================ \r\n");
    SPI_log("TX: 0xAF 0xAF\r\n");

    /* Initialize SPI SOC layer */
    SPI_init();

    /* Get SPI instance and configure */
    instance = (uint32_t)BOARD_MCSPI_MASTER_INSTANCE - 1;
    SPI_initConfig(instance);

    /* Setup semaphore for callback synchronization */
    SPI_osalSemParamsInit(&cbSemParams);
    cbSemParams.mode = SemaphoreP_Mode_BINARY;
    cbSem = SPI_osalCreateBlockingLock(0, &cbSemParams);

    /* Setup SPI parameters */
    SPI_Params_init(&spiParams);
    spiParams.transferMode = SPI_MODE_CALLBACK;       /* Callback mode */
    spiParams.transferCallbackFxn = SPI_callback;     /* Set callback */
    spiParams.transferTimeout = SPI_TIMEOUT_VALUE;    /* Timeout 5000ms */

    /* Open SPI device */
    spi = SPI_open(instance, &spiParams);

    if (spi == NULL) {
        SPI_log("Error initializing SPI\n");
        while (true) {
            Task_sleep(50000);
        }
    } else {
        SPI_log("SPI initialized\n");
    }

    /* Fill TX buffer with 0xAF (2 bytes) */
    for (int i = 0; i < SPI_MSG_LENGTH; i++) {
        masterTxBuffer[i] = 0xAF;
    }

    /* Continuous loop: transfer + delay */
    while (true) {
        SPI_test_mst_slv_xfer(spi, SPI_MSG_LENGTH);
        Task_sleep(50); /* 50ms delay */
    }
}

/**
 * @brief   Main entry point
 *
 * Flow:
 * 1. Setup Task parameters
 * 2. Create masterTaskFxn task
 * 3. Initialize board (pinmux, clock, UART)
 * 4. Start BIOS scheduler
 */
int main(void)
{
    Task_Handle task;
    Error_Block eb;
    Task_Params taskParams;

    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.priority = 2;
    taskParams.stackSize = 0x4000;

    task = Task_create(masterTaskFxn, &taskParams, &eb);

    if (task == NULL) {
        System_printf("Task_create() failed!\n");
        BIOS_exit(0);
    }

    /* Initialize board: pinmux, module clock, UART */
    Board_initCfg boardCfg = BOARD_INIT_PINMUX_CONFIG |
                             BOARD_INIT_MODULE_CLOCK  |
                             BOARD_INIT_UART_STDIO;

    if (Board_init(boardCfg) != BOARD_SOK) {
        return (0);
    }

    /* Start BIOS - scheduler will run tasks */
    BIOS_start();

    return (0);
}
