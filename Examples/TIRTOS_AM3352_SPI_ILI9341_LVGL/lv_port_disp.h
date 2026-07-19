#ifndef LV_PORT_DISP_H_
#define LV_PORT_DISP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "lvgl.h"

extern void LcdDcLow(void);
extern void LcdDcHigh(void);
extern void Spi0TxByte(uint8_t b);

void lv_port_disp_init(void);

#ifdef __cplusplus
}
#endif

#endif /* LV_PORT_DISP_H_ */
