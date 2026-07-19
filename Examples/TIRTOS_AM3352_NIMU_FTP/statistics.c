/*
 * Copyright (C) 2018 Texas Instruments Incorporated
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met.
 */

/**
 *  @file statistics.c
 *  @brief Ethernet statistics task
 *
 *  Background task that periodically retrieves and prints EMAC/RX/TX
 *  frame statistics via UART. Runs at priority 9 with a 10-second
 *  interval between updates.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ti/ndk/inc/stkmain.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <ti/drv/emac/emac_drv.h>
#include <ti/drv/emac/src/v4/emac_drv_v4.h>
#include <ti/board/board.h>

#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>

/* Statistics update interval in ticks (10 seconds) */
#define STATS_INTERVAL_TICKS    10000

/**
 *  @brief Ethernet statistics task entry point
 *
 *  Continuously retrieves EMAC statistics and prints RX/TX counters
 *  including good frames, broadcast, multicast, octets, CRC errors,
 *  collisions, and more via UART_printf.
 *
 *  @param  arg0  Task argument (unused)
 *  @param  arg1  Task argument (unused)
 */
void app_stats(UArg arg0, UArg arg1)
{
    EMAC_STATISTICS_T stats;
    EMAC_DRV_ERR_E ret = EMAC_DRV_RESULT_OK;

    while (TRUE) {
        memset(&stats, 0, sizeof(EMAC_STATISTICS_T));

        /* Retrieve EMAC statistics from the driver */
        ret = emac_get_statistics(0, &stats);

        if (ret != EMAC_DRV_RESULT_OK) {
            UART_printf("Failed to retrieve EMAC stats: %d\n", ret);
            continue;
        }

        /* Print RX statistics */
        UART_printf("--------------------------------------------------------------------------------------\n");
        UART_printf(" RX |     Good:%8d |   Bcast:%8d |    Mcast:%8d |    Oct:%12d |\n",
                    stats.RxGoodFrames,
                    stats.RxBCastFrames,
                    stats.RxMCastFrames,
                    stats.RxOctets);
        UART_printf(" RX |    Pause:%8d |     CRC:%8d | AlignErr:%8d | Oversz:%12d |\n",
                    stats.RxPauseFrames,
                    stats.RxCRCErrors,
                    stats.RxAlignCodeErrors,
                    stats.RxOversized);
        UART_printf(" RX |   Jabber:%8d | Undersz:%8d |     Frag:%8d |   Filt:%12d |\n",
                    stats.RxJabber,
                    stats.RxUndersized,
                    stats.RxFragments,
                    stats.RxFiltered);
        UART_printf(" RX |      QoS:%8d |  SOFOvr:%8d |   MOFOvr:%8d | DMAOvr:%12d |\n",
                    stats.RxQOSFiltered,
                    stats.RxSOFOverruns,
                    stats.RxMOFOverruns,
                    stats.RxDMAOverruns);

        /* Print TX statistics */
        UART_printf(" TX |     Good:%8d |   Bcast:%8d |    Mcast:%8d |    Oct:%12d |\n",
                    stats.TxGoodFrames,
                    stats.TxBCastFrames,
                    stats.TxMCastFrames,
                    stats.TxOctets);
        UART_printf(" TX |    Pause:%8d | Deferred:%7d |     Coll:%8d |   Udrn:%12d |\n",
                    stats.TxPauseFrames,
                    stats.TxDeferred,
                    stats.TxCollision,
                    stats.TxUnderrun);
        UART_printf("--------------------------------------------------------------------------------------\n");

        /* Wait 10 seconds before next update */
        Task_sleep(STATS_INTERVAL_TICKS);
    }
}
