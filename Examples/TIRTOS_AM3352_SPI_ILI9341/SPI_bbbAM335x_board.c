/* ==========================================================================
 * SPI_bbbAM335x_board.c — AM335x BBB SPI0 board implementation
 *
 * Provides SPI pinmux, clock enable, and SPI driver init.
 * ========================================================================*/

/* -----------------------------------------------------------------------
 * Standard / CSL includes
 *  - stdint.h must precede hw_types.h (needs uint32_t)
 *  - soc_am335x.h provides SOC_CONTROL_REGS, SOC_CM_PER_REGS
 * -----------------------------------------------------------------------*/
#include <stdint.h>
#include <ti/starterware/include/hw/soc_am335x.h>
#include <ti/csl/hw_types.h>
#include <ti/starterware/include/hw/hw_control_am335x.h>
#include <ti/starterware/include/am335x/hw_cm_per.h>

/* -----------------------------------------------------------------------
 * SPI driver includes
 *  - SPI_soc.h guards behind SOC_AM335x (lowercase x);
 *    define it here since the project provides SOC_AM335X (uppercase).
 * -----------------------------------------------------------------------*/
#ifndef SOC_AM335x
#define SOC_AM335x
#endif

#include <ti/drv/spi/SPI.h>
#include <ti/drv/spi/soc/SPI_soc.h>
#include <ti/drv/spi/soc/SPI_v1.h>
#include <ti/csl/src/ip/mcspi/V0/hw_mcspi.h>

/* -----------------------------------------------------------------------
 * Board header (defines SPI_INSTANCE, SPI_BIT_RATE, …)
 * -----------------------------------------------------------------------*/
#include "SPI_board.h"

/* -----------------------------------------------------------------------
 * Globals — SPI handle + TX/RX buffers
 * -----------------------------------------------------------------------*/
SPI_Handle gSpiHandle;
uint8_t gTxBuffer[SPI_DATA_COUNT];
uint8_t gRxBuffer[SPI_DATA_COUNT];

/* -----------------------------------------------------------------------
 * SpiPinMuxSetup — pinmux for all SPI + LCD-GPIO signals
 *
 * Registers are offset from SOC_CONTROL_REGS (0x44E1_0000).
 * Pinmux mode encoding: bits 0-2 = mode, bit 5 = RX enabled.
 *   0x07 = mode 7 (GPIO), no RX
 *   0x20 = mode 0 (HW peripheral), RX enabled
 *
 * DC & RST are manual GPIO so toggle via GPIO_write() is possible;
 * SCLK, MOSI, CS are handled by the MCSPI peripheral.
 * -----------------------------------------------------------------------*/
void SpiPinMuxSetup(void)
{
    /* DC   (P8_26  GPIO1_29)  -> mode 7 (GPIO)  - offset 0x87C */
    HWREG(SOC_CONTROL_REGS + 0x87C) = 0x00000007;
    /* RST  (P8_19  GPIO0_22)  -> mode 7 (GPIO)  - offset 0x820 */
    HWREG(SOC_CONTROL_REGS + 0x820) = 0x00000007;
    /* SCLK (P9_22  SPI0_SCLK) -> mode 0 (HW)    - offset 0x950 */
    HWREG(SOC_CONTROL_REGS + 0x950) = 0x00000020;
    /* MOSI (P9_18  SPI0_D1)   -> mode 0 (HW)    - offset 0x958 */
    HWREG(SOC_CONTROL_REGS + 0x958) = 0x00000020;
    /* CS   (P9_17  SPI0_CS0)  -> mode 0 (HW)    - offset 0x95C */
    HWREG(SOC_CONTROL_REGS + 0x95C) = 0x00000020;
}

/* -----------------------------------------------------------------------
 * Spi0ClockEnable — enable SPI0 module clock via CM_PER
 *
 * SPI0 clock register: CM_PER_SPI0_CLKCTRL (offset 0x90)
 *   0x02 = MODULEMODE ENABLE (explicit enable)
 *   Wait until bit 1 set + bit 0 clear (idle status = 0).
 * -----------------------------------------------------------------------*/
void Spi0ClockEnable(void)
{
    HWREG(SOC_CM_PER_REGS + CM_PER_SPI0_CLKCTRL) = 0x00000002;
    while ((HWREG(SOC_CM_PER_REGS + CM_PER_SPI0_CLKCTRL) & 0x03) != 0x02) {}
}

/* -----------------------------------------------------------------------
 * SpiInit — configure and open TI-RTOS SPI driver
 *
 * Steps:
 *   1. SPI_init()              — driver init
 *   2. SPI_socGetInitCfg       — read default HW attrs
 *   3. Set dataLineCommMode    — 4-pin (CLK+MOSI+MISO+CS)
 *   4. SPI_socSetInitCfg       — apply modified attrs
 *   5. SPI_open                — SPI_MASTER, 8-bit, POL0/PHA0
 * -----------------------------------------------------------------------*/
void SpiInit(void)
{
    SPI_Params spiParams;
    SPI_v1_HWAttrs spiCfg;

    SPI_init();
    SPI_socGetInitCfg(SPI_INSTANCE, &spiCfg);
    spiCfg.chnCfg[spiCfg.chNum].dataLineCommMode = MCSPI_DATA_LINE_COMM_MODE_1;
    SPI_socSetInitCfg(SPI_INSTANCE, &spiCfg);

    SPI_Params_init(&spiParams);
    spiParams.mode = SPI_MASTER;
    spiParams.dataSize = 8;
    spiParams.frameFormat = SPI_POL0_PHA0;
    spiParams.bitRate = SPI_BIT_RATE;
    spiParams.transferMode = SPI_MODE_BLOCKING;
    spiParams.transferTimeout = UINT32_MAX;

    gSpiHandle = SPI_open(SPI_INSTANCE, &spiParams);
}
