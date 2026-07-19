#ifndef _DELAY_H_
#define _DELAY_H_

#include <stdint.h>

/*
 * Busy-wait / RTOS-aware delay in milliseconds.
 *
 * In this SYS/BIOS project, delay() is implemented using Task_sleep()
 * in main.c so it yields the task without burning CPU.
 */
void delay(uint32_t ms);

#endif /* _DELAY_H_ */
