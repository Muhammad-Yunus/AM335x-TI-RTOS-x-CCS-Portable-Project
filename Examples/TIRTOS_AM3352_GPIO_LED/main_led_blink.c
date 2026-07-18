/**
 *  \file   main_led_blink.c
 *
 *  \brief  AM3352 + SYSBIOS (TI-RTOS) demo - Task blinks LED.
 *
 */

/*
 * Copyright (C) 2014 - 2019 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drv/gpio/GPIO.h>

#include "GPIO_board.h"

#include <ti/board/board.h>

/**********************************************************************
 ************************** Macros ************************************
 **********************************************************************/
#define BLINK_DELAY_MS        (500)

/**********************************************************************
 ************************** Internal functions ************************
 **********************************************************************/

Void gpio_blink_task(UArg arg0, UArg arg1);

/*
 *  ======== Board_initGPIO ========
 */
static void Board_initGPIO(void)
{
    Board_initCfg boardCfg;

    boardCfg = BOARD_INIT_PINMUX_CONFIG |
               BOARD_INIT_MODULE_CLOCK;

    Board_init(boardCfg);
}

/**********************************************************************
 ************************** Global Variables **************************
 **********************************************************************/

/*
 *  ======== main ========
 */
int main(void)
{
    Task_Params taskParams;
    Error_Block eb;

    Board_initGPIO();
    GPIO_init();

    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.priority = 1;
    taskParams.stackSize = 0x400;
    Task_create((Task_FuncPtr)gpio_blink_task, &taskParams, &eb);

    BIOS_start();
    return 0;
}

/*
 *  ======== gpio_blink_task ========
 */
Void gpio_blink_task(UArg arg0, UArg arg1)
{
    while (1)
    {
        GPIO_toggle(USER_LED0);
        Task_sleep(BLINK_DELAY_MS);
    }
}
