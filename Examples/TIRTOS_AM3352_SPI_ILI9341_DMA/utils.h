/**
 * @file    utils.h
 * @brief   Utility functions for SPI DMA transfer and LCD GPIO control
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>

/* Delay in task ticks */
void delay(uint32_t ms);

/* SPI DMA transfer helpers */
void Spi1TxByte(uint8_t b);
void Spi1TxBuffer(const uint8_t *buf, uint32_t len);

/* LCD GPIO control */
void LcdDcLow(void);
void LcdDcHigh(void);
void LcdRstLow(void);
void LcdRstHigh(void);

#endif /* UTILS_H_ */
