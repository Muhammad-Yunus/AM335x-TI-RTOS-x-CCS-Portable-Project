#include <xdc/std.h>
#include <xdc/runtime/Error.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <ti/board/board.h>

#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>

#include <ti/drv/i2c/I2C.h>

#include <ti/csl/hw_types.h>
#include <ti/starterware/include/hw/soc_am335x.h>
#include <ti/starterware/include/am335x/hw_cm_per.h>
#include <ti/starterware/include/hw/hw_control_am335x.h>

#include <stdio.h>

#include "ssd1306.h"

#define I2C1_SCL_PIN_VAL  (0x3A)
#define I2C1_SDA_PIN_VAL  (0x3A)

I2C_Handle g_i2cHandle = NULL;

static void Board_initI2C1(void)
{
    HWREG(SOC_CM_PER_REGS + CM_PER_I2C1_CLKCTRL) =
        (HWREG(SOC_CM_PER_REGS + CM_PER_I2C1_CLKCTRL) & ~CM_PER_I2C1_CLKCTRL_MODULEMODE) |
        CM_PER_I2C1_CLKCTRL_MODULEMODE_ENABLE;

    while ((HWREG(SOC_CM_PER_REGS + CM_PER_I2C1_CLKCTRL) & CM_PER_I2C1_CLKCTRL_IDLEST) !=
           CM_PER_I2C1_CLKCTRL_IDLEST_FUNC);

    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_SPI0_D1) = I2C1_SCL_PIN_VAL;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_SPI0_CS0) = I2C1_SDA_PIN_VAL;
}

static Void i2c_ssd1306_lcd_test(UArg arg0, UArg arg1)
{
    I2C_Params i2cParams;
    int n;
    char buf[16];

    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_100kHz;
    i2cParams.transferMode = I2C_MODE_BLOCKING;

    g_i2cHandle = I2C_open(1, &i2cParams);
    if (g_i2cHandle == NULL)
    {
        UART_printf("Error: I2C1 open failed\n");
        while (1) { Task_sleep(1000); }
    }

    UART_printf("\n+---------------------------------------------+\n");
    UART_printf("|  AM3352  SSD1306  128x32  OLED  LCD  Test    |\n");
    UART_printf("|  I2C1  SCL=P9_17  SDA=P9_18  addr=0x3C      |\n");
    UART_printf("+---------------------------------------------+\n");

    if (ssd1306_Init() == 0)
    {
        UART_printf("ssd1306_Init FAILED\n");
        while (1) { Task_sleep(1000); }
    }
    UART_printf("ssd1306_Init OK\n");

    while (1)
    {
        ssd1306_Clear();
        ssd1306_SetCursor(2, 0);
        ssd1306_SetColor(White);
        ssd1306_WriteString("AM3352 + SSD1306", Font_7x10);
        ssd1306_SetCursor(2, 16);
        ssd1306_WriteString("I2C1 @ 0x3C  128x32", Font_7x10);
        ssd1306_UpdateScreen();
        Task_sleep(3000);

        ssd1306_Clear();
        ssd1306_SetColor(White);
        ssd1306_DrawRect(0, 0, 128, 32);
        ssd1306_DrawRect(4, 4, 56, 24);
        ssd1306_DrawCircle(110, 16, 12);
        ssd1306_DrawLine(64, 4, 96, 28);
        ssd1306_DrawLine(96, 4, 64, 28);
        ssd1306_UpdateScreen();
        Task_sleep(3000);

        ssd1306_Clear();
        ssd1306_SetColor(White);
        ssd1306_FillRect(0, 0, 60, 32);
        ssd1306_FillCircle(110, 16, 14);
        ssd1306_UpdateScreen();
        Task_sleep(3000);

        for (n = 0; n < 100; n++)
        {
            ssd1306_Clear();
            ssd1306_SetCursor(0, 0);
            ssd1306_SetColor(White);
            ssd1306_WriteString("AM3352", Font_7x10);
            ssd1306_SetCursor(0, 16);
            UART_printf("%d\n", n);
            snprintf(buf, sizeof(buf), "n=%d", n);
            ssd1306_WriteString(buf, Font_11x18);
            ssd1306_UpdateScreen();
            Task_sleep(200);
        }
    }
}

int main(void)
{
    Board_initCfg boardCfg;
    Error_Block eb;
    Task_Params taskParams;

    boardCfg = BOARD_INIT_PINMUX_CONFIG |
               BOARD_INIT_MODULE_CLOCK |
               BOARD_INIT_UART_STDIO;
    Board_init(boardCfg);

    Board_initI2C1();

    I2C_init();

    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.priority = 1;
    taskParams.stackSize = 0x1000;
    Task_create((Task_FuncPtr)i2c_ssd1306_lcd_test, &taskParams, &eb);

    BIOS_start();
    return 0;
}
