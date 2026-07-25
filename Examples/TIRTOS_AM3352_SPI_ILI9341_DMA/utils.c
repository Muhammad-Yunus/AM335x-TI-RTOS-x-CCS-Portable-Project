/**
 * @file    utils.c
 * @brief   Utility functions for SPI DMA transfer and LCD GPIO control
 */

#include <ti/sysbios/knl/Task.h>
#include <ti/drv/spi/SPI.h>
#include <ti/osal/CacheP.h>
#include <ti/drv/spi/src/SPI_osal.h>
#include <ti/drv/gpio/GPIO.h>
#include "GPIO_board.h"
#include "ili9341/ili9341.h"
#include "utils.h"

/* ============================================================================
 * Constants
 * ============================================================================ */

#define SPI_TIMEOUT_VALUE     5000          /* Transfer timeout in ms      */
#define SPI_LCD_CHUNK         (65535)

/* ============================================================================
 * Global Variables (shared with lcd_demo.c)
 * ============================================================================ */

extern SemaphoreP_Handle lcdSem;
extern bool              lcdTransferDone;
extern SPI_Handle        gSpiLcdHandle;
static uint8_t           lcdRxBuf[SPI_LCD_CHUNK];

/* ============================================================================
 * Function Definitions
 * ============================================================================ */

void delay(uint32_t ms)
{
    Task_sleep(ms);
}

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

void LcdDcLow(void)   { GPIO_write(LCD_DC,  GPIO_PIN_VAL_LOW);  }
void LcdDcHigh(void)  { GPIO_write(LCD_DC,  GPIO_PIN_VAL_HIGH); }
void LcdRstLow(void)  { GPIO_write(LCD_RST, GPIO_PIN_VAL_LOW);  }
void LcdRstHigh(void) { GPIO_write(LCD_RST, GPIO_PIN_VAL_HIGH); }
