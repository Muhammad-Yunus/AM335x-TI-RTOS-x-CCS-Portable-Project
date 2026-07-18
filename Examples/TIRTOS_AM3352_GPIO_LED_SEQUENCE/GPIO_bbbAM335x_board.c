/**
 *  \file   GPIO_bbbAM335x_board.c
 *
 *  \brief  AM335x BBB EVM board specific GPIO parameters.
 *
 */

/*
 * Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/
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
#include <ti/drv/gpio/GPIO.h>
#include <ti/drv/gpio/soc/GPIO_soc.h>

#define GPIO_LED_PORT_NUM        (0x01)

#define GPIO_LED_D2_PIN_NUM      (0x15)  /* D2 - GPIO1[21] */
#define GPIO_LED_D3_PIN_NUM      (0x16)  /* D3 - GPIO1[22] */
#define GPIO_LED_D4_PIN_NUM      (0x17)  /* D4 - GPIO1[23] */
#define GPIO_LED_D5_PIN_NUM      (0x18)  /* D5 - GPIO1[24] */

GPIO_PinConfig gpioPinConfigs[] = {
    GPIO_DEVICE_CONFIG((GPIO_LED_PORT_NUM + 1), GPIO_LED_D2_PIN_NUM) | GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG((GPIO_LED_PORT_NUM + 1), GPIO_LED_D3_PIN_NUM) | GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG((GPIO_LED_PORT_NUM + 1), GPIO_LED_D4_PIN_NUM) | GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG((GPIO_LED_PORT_NUM + 1), GPIO_LED_D5_PIN_NUM) | GPIO_CFG_OUTPUT
};

GPIO_CallbackFxn gpioCallbackFunctions[] = {
    NULL, NULL, NULL, NULL
};

GPIO_v1_Config GPIO_v1_config = {
    gpioPinConfigs,
    gpioCallbackFunctions,
    4,
    4,
    0x0U,
};
