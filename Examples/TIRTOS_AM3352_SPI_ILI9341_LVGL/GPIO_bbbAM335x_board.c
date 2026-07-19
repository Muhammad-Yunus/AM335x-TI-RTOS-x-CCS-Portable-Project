/* ==========================================================================
 * GPIO_bbbAM335x_board.c - AM335x BBB GPIO board configuration
 *
 * Provides GPIO0 clock enable and the TI GPIO driver configuration
 * tables (pin configs, callbacks, v1 attrs) for the LCD display pins.
 * ========================================================================*/

/* -----------------------------------------------------------------------
 * Standard & CSL includes
 * -----------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
#include <ti/starterware/include/hw/soc_am335x.h>
#include <ti/csl/hw_types.h>
#include <ti/starterware/include/am335x/hw_cm_wkup.h>

/* -----------------------------------------------------------------------
 * TI GPIO driver
 * -----------------------------------------------------------------------*/
#include <ti/drv/gpio/GPIO.h>
#include <ti/drv/gpio/soc/GPIO_v1.h>

/* -----------------------------------------------------------------------
 * GPIO pin definitions (0-based port numbers)
 *
 * The TI GPIO driver uses 1-based port indices (portNum > 0U check),
 * so +1 is added in GPIO_DEVICE_CONFIG() below.
 *
 *   LCD_DC  = GPIO1_29 -> port 1, pin 29
 *   LCD_RST = GPIO0_22 -> port 0, pin 22
 * -----------------------------------------------------------------------*/
#define LCD_DC_PORT_NUM     (0x01)
#define LCD_DC_PIN_NUM      (0x1D)

#define LCD_RST_PORT_NUM    (0x00)
#define LCD_RST_PIN_NUM     (0x16)

/* -----------------------------------------------------------------------
 * Gpio0ClockEnable - enable GPIO0 module clock (CM_WKUP domain)
 *
 * CM_WKUP_GPIO0_CLKCTRL offset = 0x08
 *   0x02  = MODULEMODE ENABLE
 *   Wait  = bit 1 set + bit 0 clear (module fully enabled)
 * -----------------------------------------------------------------------*/
void Gpio0ClockEnable(void)
{
    HWREG(SOC_CM_WKUP_REGS + 0x08) = 0x00000002;
    while ((HWREG(SOC_CM_WKUP_REGS + 0x08) & 0x03) != 0x02) {}
}

/* -----------------------------------------------------------------------
 * GPIO pin config table
 *   Indices: LCD_DC=0, LCD_RST=1  (see GPIO_board.h)
 * -----------------------------------------------------------------------*/
GPIO_PinConfig gpioPinConfigs[] = {
    GPIO_DEVICE_CONFIG((LCD_DC_PORT_NUM + 1), LCD_DC_PIN_NUM) |
    GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG((LCD_RST_PORT_NUM + 1), LCD_RST_PIN_NUM) |
    GPIO_CFG_OUTPUT,
};

/* -----------------------------------------------------------------------
 * Callback table (unused - all pins are outputs)
 * -----------------------------------------------------------------------*/
GPIO_CallbackFxn gpioCallbackFunctions[] = {
    NULL,
    NULL,
};

/* -----------------------------------------------------------------------
 * GPIO v1 driver config
 *   .pinConfigs      = gpioPinConfigs
 *   .callbacks       = gpioCallbackFunctions
 *   .numPins         = 2
 *   .numCallbacks    = 2
 *   .enableIntr      = 0x3U  (interrupts disabled on both pins)
 * -----------------------------------------------------------------------*/
GPIO_v1_Config GPIO_v1_config = {
    gpioPinConfigs,
    gpioCallbackFunctions,
    2,
    2,
    0x3U,
};
