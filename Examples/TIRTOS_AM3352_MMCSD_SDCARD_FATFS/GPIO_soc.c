/**
 * =============================================================================
 * FILE: GPIO_soc.c
 * =============================================================================
 * Deskripsi: SOC-specific GPIO configuration untuk AM335x.
 *             File ini mendefinisikan hardware attributes untuk 4 bank GPIO
 * *bank) pada AM335x dan menghubungkan GPIO driver dengan tabel fungsi.
 *
 * AM335x GPIO Overview:
 *   - 4 bank GPIO (GPIO0-GPIO3)
 *   - Masing-masing bank memiliki 32 pin (total 128 pin)
 *   - Bank 0: pin 0-96  (register base: SOC_GPIO_0_REGS)
 *   - Bank 1: pin 97-99
 *   - Bank 2: pin 32-33
 *   - Bank 3: pin 62-63
 *
 * Pin yang digunakan untuk project ini:
 *   - GPIO_MMC_SDCD: Card Detect pin (interrupt enabled)
 * =============================================================================
 */

#include <ti/csl/csl_utils.h>
#include <ti/drv/gpio/GPIO.h>
#include <ti/starterware/include/hw/soc_am335x.h>
#include <ti/csl/csl_types.h>
#include <ti/drv/gpio/soc/GPIO_soc.h>

/* Jumlah bank GPIO pada AM335x = 4 */
#define CSL_GPIO_PER_CNT    4U

/**
 * GPIO_v1_hwAttrs: Hardware attributes untuk masing-masing bank GPIO.
 *
 * Format per entry:
 *   { register_base, firstPin, lastPin, reserved1, reserved2 }
 *
 * Bank 0 (SOC_GPIO_0_REGS):  pin 0-96   -> Termasuk pin SD_CD
 * Bank 1 (SOC_GPIO_1_REGS):  pin 97-99
 * Bank 2 (SOC_GPIO_2_REGS):  pin 32-33
 * Bank 3 (SOC_GPIO_3_REGS):  pin 62-63
 *
 * Entry 4-7 diisi NULL (karena hanya 4 bank)
 */
GPIO_v1_hwAttrs_list GPIO_v1_hwAttrs = {
    { SOC_GPIO_0_REGS, 96, 97, 0, 0 },
    { SOC_GPIO_1_REGS, 98, 99, 0, 0 },
    { SOC_GPIO_2_REGS, 32, 33, 0, 0 },
    { SOC_GPIO_3_REGS, 62, 63, 0, 0 },
    { 0,0,0,0,0 }, { 0,0,0,0,0 }, { 0,0,0,0,0 }, { 0,0,0,0,0 },
};

/**
 * GPIO_config: Tabel konfigurasi GPIO driver.
 *
 * Struktur:
 *   { function_table, hwAttrs_list, init_func }
 *
 * Saat ini hanya menggunakan default function table (GPIO_FxnTable_v1).
 */
CSL_PUBLIC_CONST GPIOConfigList GPIO_config = {
    { &GPIO_FxnTable_v1, NULL, NULL },
    { NULL, NULL, NULL },
    { NULL, NULL, NULL }
};
