/**
 * @file    utils.c
 * @brief   Utility helpers for LCD GPIO control and delay
 */

#include "utils.h"
#include <ti/drv/gpio/GPIO.h>
#include "GPIO_board.h"
#include <ti/sysbios/knl/Task.h>

void delay(uint32_t ms)
{
    Task_sleep(ms);
}

void LcdDcLow(void)
{
    GPIO_write(LCD_DC, GPIO_PIN_VAL_LOW);
}

void LcdDcHigh(void)
{
    GPIO_write(LCD_DC, GPIO_PIN_VAL_HIGH);
}

void LcdRstLow(void)
{
    GPIO_write(LCD_RST, GPIO_PIN_VAL_LOW);
}

void LcdRstHigh(void)
{
    GPIO_write(LCD_RST, GPIO_PIN_VAL_HIGH);
}
