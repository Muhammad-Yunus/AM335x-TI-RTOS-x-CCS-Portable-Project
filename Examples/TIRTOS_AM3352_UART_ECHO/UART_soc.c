/*
 * Copyright (c) 2014-2019, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ti/csl/csl_utils.h>
#include <ti/drv/uart/UART.h>
#include <ti/starterware/include/types.h>
#include <ti/starterware/include/hw/soc_am335x.h>
#include <ti/drv/uart/soc/UART_soc.h>

#define CSL_UART_PER_CNT  (6U)

UART_HwAttrs uartInitCfg[CSL_UART_PER_CNT] =
{
    {
        SOC_UART_0_REGS,
        72,
        0,
        48000000U,
        27U, 26U,
        0, 0, 0, 0,
        0,
        NULL,
        UART_RXTRIGLVL_8,
        UART_TXTRIGLVL_56,
        0,
        0,
        1,
    },
    {
        SOC_UART_1_REGS,
        73,
        0,
        48000000U,
        29U, 28U,
        0, 0, 0, 0,
        0,
        NULL,
        UART_RXTRIGLVL_8,
        UART_TXTRIGLVL_56,
        0,
        0,
        1,
    },
    {
        SOC_UART_2_REGS,
        74,
        69,
        48000000U,
        31U, 30U,
        0, 0, 0, 0,
        0,
        NULL,
        UART_RXTRIGLVL_8,
        UART_TXTRIGLVL_56,
        0,
        0,
        1,
    },
    {
        SOC_UART_3_REGS,
        44,
        0,
        48000000U,
        8U, 7U,
        0, 0, 0, 0,
        0,
        NULL,
        UART_RXTRIGLVL_8,
        UART_TXTRIGLVL_56,
        0,
        0,
        1,
    },
    {
        SOC_UART_4_REGS,
        45,
        0,
        48000000U,
        10U, 9U,
        0, 0, 0, 0,
        0,
        NULL,
        UART_RXTRIGLVL_8,
        UART_TXTRIGLVL_56,
        0,
        0,
        1,
    },
    {
        SOC_UART_5_REGS,
        46,
        0,
        48000000U,
        12U, 11U,
        0, 0, 0, 0,
        0,
        NULL,
        UART_RXTRIGLVL_8,
        UART_TXTRIGLVL_56,
        0,
        0,
        1,
    },
};

UART_V1_Object UartObjects[CSL_UART_PER_CNT];

UART_config_list UART_config = {
    {&UART_FxnTable_v1, &UartObjects[0], &uartInitCfg[0]},
    {&UART_FxnTable_v1, &UartObjects[1], &uartInitCfg[1]},
    {&UART_FxnTable_v1, &UartObjects[2], &uartInitCfg[2]},
    {&UART_FxnTable_v1, &UartObjects[3], &uartInitCfg[3]},
    {&UART_FxnTable_v1, &UartObjects[4], &uartInitCfg[4]},
    {&UART_FxnTable_v1, &UartObjects[5], &uartInitCfg[5]},
};
