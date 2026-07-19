/**
 *  \file   helper.c
 *
 *  \brief  Helper implementations for SDMMC SDCARD example.
 */

#include "helper.h"

void fill_pattern(uint8_t *buf, uint32_t len, uint32_t start_val)
{
    uint32_t *buf32 = (uint32_t *)buf;
    for (uint32_t i = 0; i < len / sizeof(uint32_t); i++)
        buf32[i] = start_val + i;
}

int compare_buf(uint8_t *a, uint8_t *b, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        if (a[i] != b[i]) return i + 1;
    return 0;
}
