#include <stddef.h>
#include <stdint.h>
#include <ti/starterware/include/hw/soc_am335x.h>
#include <ti/csl/hw_types.h>
#include <ti/starterware/include/am335x/hw_cm_wkup.h>

#include <ti/drv/gpio/GPIO.h>
#include <ti/drv/gpio/soc/GPIO_v1.h>

#include "GPIO_board.h"

#define LCD_DC_PORT_NUM     (0x01)
#define LCD_DC_PIN_NUM      (0x1D)

#define LCD_RST_PORT_NUM    (0x00)
#define LCD_RST_PIN_NUM     (0x16)

void Gpio0ClockEnable(void)
{
    HWREG(SOC_CM_WKUP_REGS + 0x08) = 0x00000002;
    while ((HWREG(SOC_CM_WKUP_REGS + 0x08) & 0x03) != 0x02) {}
}

GPIO_PinConfig gpioPinConfigs[] = {
    GPIO_DEVICE_CONFIG((LCD_DC_PORT_NUM + 1), LCD_DC_PIN_NUM) |
    GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG((LCD_RST_PORT_NUM + 1), LCD_RST_PIN_NUM) |
    GPIO_CFG_OUTPUT,
};

GPIO_CallbackFxn gpioCallbackFunctions[] = {
    NULL,
    NULL,
};

GPIO_v1_Config GPIO_v1_config = {
    gpioPinConfigs,
    gpioCallbackFunctions,
    2,
    2,
    0x3U,
};
