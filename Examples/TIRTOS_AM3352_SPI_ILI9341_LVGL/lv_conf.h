/**
 * @file lv_conf.h
 * LVGL v9.2.1 configuration for TIRTOS_AM3352_SPI_ILI9341_LVGL
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_USE_SYSMON            1
#define LV_USE_PERF_MONITOR      1

#define LV_MEM_SIZE              (256U * 1024U)

#define LV_USE_DEMO_WIDGETS      0
#define LV_USE_DEMO_BENCHMARK    0
#define LV_USE_DEMO_STRESS       0

#define LV_USE_DEMO_MUSIC        1
#if LV_USE_DEMO_MUSIC
# define LV_DEMO_MUSIC_SQUARE       1
# define LV_DEMO_MUSIC_LANDSCAPE    0
# define LV_DEMO_MUSIC_ROUND        0
# define LV_DEMO_MUSIC_LARGE        0
# define LV_DEMO_MUSIC_AUTO_PLAY    1
#endif

#define LV_COLOR_DEPTH           16
#define LV_COLOR_16_SWAP         1

#define LV_FONT_MONTSERRAT_12    1
#define LV_FONT_MONTSERRAT_16    1

#endif /* LV_CONF_H */
