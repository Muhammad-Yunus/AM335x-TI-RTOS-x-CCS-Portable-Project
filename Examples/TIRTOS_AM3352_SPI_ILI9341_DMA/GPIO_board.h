#ifndef _GPIO_BOARD_H
#define _GPIO_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ti/board/board.h>

void Gpio0ClockEnable(void);

typedef enum GPIO_LCD {
    LCD_DC  = 0,
    LCD_RST = 1,
} GPIO_LCD;

#define GPIO_PIN_VAL_LOW     (0U)
#define GPIO_PIN_VAL_HIGH    (1U)

#ifdef __cplusplus
}
#endif

#endif /* _GPIO_BOARD_H */
