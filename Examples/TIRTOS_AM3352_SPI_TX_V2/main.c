#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drv/spi/soc/SPI_soc.h>
#include <ti/drv/spi/src/SPI_osal.h>
#include <ti/drv/spi/SPI.h>
#include "SPI_log.h"
#include <ti/board/board.h>
#include <string.h>

#define SPI_MSG_LENGTH    2

unsigned char masterTxBuffer[SPI_MSG_LENGTH];
unsigned char masterRxBuffer[SPI_MSG_LENGTH];

Void spi_task(UArg arg0, UArg arg1);

int main(void)
{
    Task_Params taskParams;
    Error_Block eb;

    Board_initCfg boardCfg;
    boardCfg = BOARD_INIT_PINMUX_CONFIG | BOARD_INIT_MODULE_CLOCK | BOARD_INIT_UART_STDIO;
    Board_init(boardCfg);

    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.priority = 1;
    taskParams.stackSize = 0x800;
    Task_create((Task_FuncPtr)spi_task, &taskParams, &eb);

    BIOS_start();
    return 0;
}

Void spi_task(UArg arg0, UArg arg1)
{
    uint32_t      instance;
    SPI_Handle    spi;
    SPI_Params    spiParams;
    SPI_Transaction transaction;

    instance = (uint32_t)BOARD_MCSPI_MASTER_INSTANCE - 1;

    SPI_Params_init(&spiParams);
    spiParams.mode = SPI_MASTER;
    spiParams.transferTimeout = 1000;
    spi = SPI_open(instance, &spiParams);
    if (spi == NULL)
    {
        SPI_log("Error initializing SPI\n");
        while (1) {}
    }
    SPI_log("SPI initialized\n");

    for (uint32_t i = 0; i < SPI_MSG_LENGTH; i++)
    {
        masterTxBuffer[i] = 0xAF;
    }
    memset(masterRxBuffer, 0, sizeof(masterRxBuffer));

    transaction.count = SPI_MSG_LENGTH;
    transaction.txBuf = (void *)masterTxBuffer;
    transaction.rxBuf = (void *)masterRxBuffer;

    while (1)
    {
        SPI_transfer(spi, &transaction);
        for (uint32_t i = 0; i < SPI_MSG_LENGTH; i++)
        {
            SPI_log("RX[%d]=0x%02X TX[%d]=0x%02X\n", i, masterRxBuffer[i], i, masterTxBuffer[i]);
        }
        Task_sleep(10 * 1000 / 100);
    }
}
