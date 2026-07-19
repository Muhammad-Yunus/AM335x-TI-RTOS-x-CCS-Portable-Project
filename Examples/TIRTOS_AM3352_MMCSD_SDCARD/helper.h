/**
 *  \file   helper.h
 *
 *  \brief  Reusable helpers for SDMMC SDCARD example.
 *
 *  Provides:
 *    - MMCSD_CHECK  : macro for MMCSD API error handling
 *    - VERIFY_DATA  : macro for buffer comparison with error exit
 *    - fill_pattern : fill buffer with 32-bit incrementing pattern
 *    - compare_buf  : byte-by-byte buffer comparison
 */

#ifndef HELPER_H
#define HELPER_H

#include <stdint.h>
#include <ti/drv/mmcsd/MMCSD.h>
#include "MMCSD_log.h"

/* ---------------------------------------------------------------------------
 *  Macros
 * ------------------------------------------------------------------------- */

#define MMCSD_CHECK(ret, handle, msg) \
    do { \
        if ((ret) != MMCSD_OK) { \
            MMCSD_log((msg)); \
            MMCSD_close(handle); \
            return; \
        } \
    } while (0)

#define VERIFY_DATA(handle, a, b, len, msg) \
    do { \
        if (compare_buf((a), (b), (len)) != 0) { \
            MMCSD_log((msg)); \
            MMCSD_close(handle); \
            return; \
        } \
    } while (0)

/* ---------------------------------------------------------------------------
 *  Function prototypes
 * ------------------------------------------------------------------------- */

void fill_pattern(uint8_t *buf, uint32_t len, uint32_t start_val);
int  compare_buf(uint8_t *a, uint8_t *b, uint32_t len);

#endif /* HELPER_H */
