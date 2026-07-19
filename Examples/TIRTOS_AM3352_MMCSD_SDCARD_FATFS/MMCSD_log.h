/**
 * =============================================================================
 * FILE: MMCSD_log.h
 * =============================================================================
 * Deskripsi: Header untuk logging/print functions.
 *            Macro MMCSD_log di-map ke UART_printf (default) atau printf
 *            (jika IO_CONSOLE di-enable).
 *
 * Penggunaan:
 *   MMCSD_log("Card inserted\r\n");  -> output ke UART console
 *   MMCSD_log("%d files found\n", count); -> formatted output
 * =============================================================================
 */

#ifndef _MMCSD_LOG_H
#define _MMCSD_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/* UART driver headers untuk ConsoleUtilsInit dan UART_printf */
#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */
extern void UART_printf(const char *pcString, ...);
extern void ConsoleUtilsInit(void);

/* ============================================================================
 * MACROS
 * ============================================================================
 * MMCSD_log: Macro alias untuk print ke console.
 *   - Default: UART_printf (output ke UART serial)
 *   - Jika IO_CONSOLE di-uncomment: printf (output ke stdout)
 * ============================================================================ */

/* Uncomment baris berikut untuk menggunakan printf alih-alih UART */
//#define IO_CONSOLE

#ifndef IO_CONSOLE
#define MMCSD_log                UART_printf
#else
#define MMCSD_log                printf
#endif

#ifdef __cplusplus
}
#endif

#endif /* _MMCSD_LOG_H */
