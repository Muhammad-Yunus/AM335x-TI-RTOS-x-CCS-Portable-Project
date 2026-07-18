#include <xdc/std.h>
#include <xdc/runtime/Error.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <ti/board/board.h>

#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>

#include "i2c1_scanner.h"

static Void i2c_scanner_task(UArg arg0, UArg arg1)
{
    unsigned int foundCount = 0;
    unsigned char foundMap[16] = {0};
    uint32_t addr;
    unsigned int row, col;

    SetupI2C1Master();

    UART_printf("\n+---------------------------------------------+\n");
    UART_printf("|   AM3352  I2C1  Bus  Scanner                |\n");
    UART_printf("|   SCL = P9_17   SDA = P9_18   100 kHz       |\n");
    UART_printf("+---------------------------------------------+\n");

    for (addr = 0x03; addr <= 0x77; addr++)
    {
        if (I2C1ProbeAddress((unsigned char)addr))
        {
            foundMap[addr >> 3] |= (unsigned char)(1u << (addr & 0x7));
            foundCount++;
        }
    }

    UART_printf("\n      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    for (row = 0; row < 8; row++)
    {
        UART_printf("%02x: ", row << 4);
        for (col = 0; col < 16; col++)
        {
            unsigned char a = (unsigned char)((row << 4) | col);
            if (a > 0x77) break;
            if (a < 0x03)
                UART_printf("   ");
            else if (foundMap[a >> 3] & (unsigned char)(1u << (a & 0x7)))
                UART_printf(" %02x", a);
            else
                UART_printf(" --");
        }
        UART_printf("\n");
    }

    UART_printf("\n%u device(s) detected on I2C1.\n\n", foundCount);

    while (1)
    {
        Task_sleep(1000);
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

    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.priority = 1;
    taskParams.stackSize = 0x1000;
    Task_create((Task_FuncPtr)i2c_scanner_task, &taskParams, &eb);

    BIOS_start();
    return 0;
}
