/**
 * @file    utils.h
 * @brief   Utility helpers for LCD GPIO control and delay
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>

/* Delay in milliseconds (uses Task_sleep) */
void delay(uint32_t ms);

/* GPIO control for LCD DC and RST pins */
void LcdDcLow(void);
void LcdDcHigh(void);
void LcdRstLow(void);
void LcdRstHigh(void);

#endif /* _UTILS_H_ */
