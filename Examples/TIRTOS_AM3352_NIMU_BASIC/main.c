/*
 * main.c - AM3352 NIMU Ethernet Application
 *
 * This application demonstrates NIMU (Network Interface Middleware Unit)
 * Ethernet driver on TI AM335x SOC using CPSW (CPSW Ethernet Switch).
 *
 * Hardware: AM3352 (Beaglebone Black-like board)
 * Ethernet: CPSW via MII interface, PHY address 0
 * Stack: SYS/BIOS + TI-RTOS
 *
 * Application flow:
 *   1. Board init (pinmux, clock, UART)
 *   2. Configure CPSW port MAC mode (MII)
 *   3. Configure PHY address and duplex mode
 *   4. Create main task (simple_task) - prints IP info
 *   5. Create stats task (app_stats) - prints Ethernet stats
 *   6. Register NIMU device table
 *   7. Start BIOS (runs tasks in loop)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/family/arm/a8/Mmu.h>

#include <ti/ndk/inc/stkmain.h>

#include <ti/drv/emac/emac_drv.h>
#include <ti/drv/emac/src/v4/emac_drv_v4.h>

#include <ti/starterware/include/types.h>
#include <ti/starterware/include/hw/hw_types.h>
#include <ti/starterware/include/hw/hw_control_am335x.h>
#include <ti/starterware/include/hw/soc_am335x.h>
#include <ti/starterware/include/ethernet.h>
#include <ti/starterware/include/soc_control.h>

#include <ti/board/board.h>

#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>

/* External symbols from NIMU/NDK stack */
extern char *LocalIPAddr;           /* Current IP address string */
extern void app_stats(UArg arg0, UArg arg1);  /* Stats task function */

/* Logging macro: uses UART_printf by default, or printf if IO_CONSOLE defined */
#ifndef IO_CONSOLE
#define NIMU_log                UART_printf
#else
#define NIMU_log                printf
#endif

/* PHY address configuration for CPSW ports */
#define EMAC_CPSW_PORT0_PHY_ADDR_EVM  0   /* PHY on port 0 */
#define EMAC_CPSW_PORT1_PHY_ADDR_EVM  1   /* PHY on port 1 */

/* Maximum number of NIMU devices in the table */
#define MAX_TABLE_ENTRIES   3

/* Global variables */
Task_Handle main_task;                      /* Handle for main application task */
static int nimu_device_index = 0U;          /* Index for NIMU device table */
NIMU_DEVICE_TABLE_ENTRY NIMUDeviceTable[MAX_TABLE_ENTRIES];  /* NIMU device registry */

/* Function declarations */
void simple_task(UArg a0, UArg a1);         /* Main task: prints banner and IP info */
extern int CpswEmacInit(STKEVENT_Handle hEvent);  /* NIMU driver init callback */

/* ========================================================================== */
/*  main - Application Entry Point                                            */
/* ========================================================================== */
int main()
{
    Board_initCfg boardCfg;         /* Board initialization configuration */
    Task_Params taskParams;         /* Task parameter structure */
    EMAC_HwAttrs_V4 cfg;            /* EMAC hardware attributes (V4 driver) */

    /* Step 1: Initialize board - pinmux, module clocks, UART stdout */
    boardCfg = BOARD_INIT_PINMUX_CONFIG | BOARD_INIT_MODULE_CLOCK | BOARD_INIT_UART_STDIO;
    Board_init(boardCfg);

    /* Step 2: Configure CPSW port MAC mode to MII for both ports */
    SOCCtrlCpswPortMacModeSelect(1, ETHERNET_MAC_TYPE_MII);
    SOCCtrlCpswPortMacModeSelect(2, ETHERNET_MAC_TYPE_MII);

    /* Step 3: Configure EMAC/PHY settings */
    EMAC_socGetInitCfg(0, &cfg);                    /* Get current hardware config */
    cfg.port[0].phy_addr = EMAC_CPSW_PORT0_PHY_ADDR_EVM;  /* Set PHY addr for port 0 */
    cfg.port[1].phy_addr = EMAC_CPSW_PORT0_PHY_ADDR_EVM;  /* Set PHY addr for port 1 */
    cfg.macModeFlags = EMAC_CPSW_CONFIG_MODEFLG_FULLDUPLEX;  /* Force full-duplex mode */
    EMAC_socSetInitCfg(0, &cfg);                    /* Apply configuration */

    /* Step 4: Create main task (simple_task) with priority 1 */
    Task_Params_init(&taskParams);
    taskParams.priority = 1;
    taskParams.stackSize = 0x1400;
    main_task = Task_create(simple_task, &taskParams, NULL);

    /* Step 5: Create stats task with higher priority (9) for periodic Ethernet stats */
    Task_Params_init(&taskParams);
    taskParams.priority = 9;
    taskParams.instance->name = "EmacStats";
    Task_create(app_stats, &taskParams, NULL);

    /* Step 6: Register NIMU device init function and terminate table with NULL */
    NIMUDeviceTable[nimu_device_index++].init = &CpswEmacInit;
    NIMUDeviceTable[nimu_device_index].init = NULL;

    /* Step 7: Start BIOS scheduler - tasks will run from here */
    BIOS_start();

    return -1;  /* Should never reach here */
}

/* ========================================================================== */
/*  simple_task - Main Application Task                                       */
/* ========================================================================== */
/* This task runs after Ethernet stack is initialized and prints a banner
 * message along with the assigned IP address. The NIMU/NDK stack handles
 * DHCP/static IP assignment before this task executes.
 */
void simple_task(UArg a0, UArg a1)
{
    /* Print application banner with IP address */
    NIMU_log("========================================\n\r");
    NIMU_log("  AM3352 NIMU Ethernet Application\n\r");
    NIMU_log("  Board: AM3352\n\r");
    NIMU_log("  Interface: CPSW (MII mode)\n\r");
    NIMU_log("  IP Address: %s\n\r", LocalIPAddr);
    NIMU_log("========================================\n\r");
}
