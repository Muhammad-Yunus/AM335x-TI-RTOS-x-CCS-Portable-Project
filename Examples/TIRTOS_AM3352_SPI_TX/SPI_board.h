/* ==========================================================================
 * SPI_board.h - SPI0 board-specific definitions for AM3352 BBB
 *
 * All SPI0 configuration constants and function declarations
 * shared between main.c and SPI_bbbAM335x_board.c.
 * ========================================================================*/

#ifndef SPI_BOARD_H
#define SPI_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * TI SPI driver
 * -----------------------------------------------------------------------*/
#include <ti/drv/spi/SPI.h>

/* -----------------------------------------------------------------------
 * SPI instance & protocol parameters
 * -----------------------------------------------------------------------*/
#define SPI_INSTANCE    (0)         /* SPI controller index (SPI0)        */
#define SPI_BIT_RATE    (100000)    /* 100 kHz SCK                       */
#define SPI_DATA_COUNT  (1)         /* bytes per transaction             */

/* -----------------------------------------------------------------------
 * Global buffers & handle (defined in SPI_bbbAM335x_board.c)
 * -----------------------------------------------------------------------*/
extern SPI_Handle gSpiHandle;
extern uint8_t gTxBuffer[SPI_DATA_COUNT];
extern uint8_t gRxBuffer[SPI_DATA_COUNT];

/* -----------------------------------------------------------------------
 * Public interface
 * -----------------------------------------------------------------------*/
void SpiPinMuxSetup(void);     /* configure all SPI + LCD-GPIO pads       */
void Spi0ClockEnable(void);    /* enable SPI0 module clock (CM_PER)       */
void SpiInit(void);            /* open SPI0, set bitrate / mode / CS     */

#ifdef __cplusplus
}
#endif

#endif /* SPI_BOARD_H */
