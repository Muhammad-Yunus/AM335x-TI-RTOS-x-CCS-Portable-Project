/* ==========================================================================
 * GPIO_board.h - AM335x BBB GPIO definitions for SPI LCD display
 *
 * Defines the GPIO pin mapping and helper macros shared between
 * main.c and GPIO_bbbAM335x_board.c.
 * ========================================================================*/

#ifndef _GPIO_BOARD_H
#define _GPIO_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Board driver
 * -----------------------------------------------------------------------*/
#include <ti/board/board.h>

/* -----------------------------------------------------------------------
 * GPIO0 clock enable (declared, defined in GPIO_bbbAM335x_board.c)
 * -----------------------------------------------------------------------*/
void Gpio0ClockEnable(void);

/* -----------------------------------------------------------------------
 * LCD GPIO pin indices - used as 0-based index into gpioPinConfigs[]
 * -----------------------------------------------------------------------*/
typedef enum GPIO_LCD {
    LCD_DC  = 0,    /* P8_26  = GPIO1_29  (DC / Data-Command)  */
    LCD_RST = 1,    /* P8_19  = GPIO0_22  (Reset)              */
} GPIO_LCD;

/* -----------------------------------------------------------------------
 * GPIO level aliases
 * -----------------------------------------------------------------------*/
#define GPIO_PIN_VAL_LOW     (0U)
#define GPIO_PIN_VAL_HIGH    (1U)

#ifdef __cplusplus
}
#endif

#endif /* _GPIO_BOARD_H */
