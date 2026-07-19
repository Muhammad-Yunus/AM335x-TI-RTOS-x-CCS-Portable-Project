/*
 * Copyright (C) 2015 - 2018 Texas Instruments Incorporated
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

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

/* Forward declarations for external symbols */
extern char *LocalIPAddr;
extern void app_stats(UArg arg0, UArg arg1);
extern int32_t ftpserver_init(void);

/* ========================================================================== */
/*                             Macros                                         */
/* ========================================================================== */

/* Logging via UART stdio */
#define NIMU_log                UART_printf

/* PHY address for CPSW port 0 and port 1 */
#define EMAC_CPSW_PORT0_PHY_ADDR_EVM 	0
#define EMAC_CPSW_PORT1_PHY_ADDR_EVM 	1

/* Maximum number of NIMU devices in the device table */
#define MAX_TABLE_ENTRIES   3

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* Task handle for the main application task */
Task_Handle main_task;

/* Index for NIMU device table */
static int nimu_device_index = 0U;

/* NIMU device table — holds pointers to device init functions */
NIMU_DEVICE_TABLE_ENTRY NIMUDeviceTable[MAX_TABLE_ENTRIES];

/* Forward declaration for main application task */
void TaskFxn(UArg a0, UArg a1);

/* Forward declaration for CPSW EMAC initialization */
extern int CpswEmacInit (STKEVENT_Handle hEvent);

/* ========================================================================== */
/*                             Main Function                                  */
/* ========================================================================== */

/**
 *  \brief Main entry point
 *
 *  Initializes the board (pinmux, clocks, UART), configures CPSW EMAC
 *  hardware attributes, creates the main application task and statistics
 *  task, registers the NIMU device, and starts SYS/BIOS.
 */
int main()
{
    Board_initCfg boardCfg;
    Task_Params taskParams;
    EMAC_HwAttrs_V4 cfg;

    /* Step 1: Initialize board — pinmux, module clocks, UART stdio */
    boardCfg = BOARD_INIT_PINMUX_CONFIG |
        BOARD_INIT_MODULE_CLOCK | BOARD_INIT_UART_STDIO;
    Board_init(boardCfg);

    /* Step 2: Configure CPSW port MAC interface type (MII mode) */
    SOCCtrlCpswPortMacModeSelect(1, ETHERNET_MAC_TYPE_MII);
    SOCCtrlCpswPortMacModeSelect(2, ETHERNET_MAC_TYPE_MII);

    /* Step 3: Configure EMAC hardware attributes — PHY addr, duplex mode */
    EMAC_socGetInitCfg(0, &cfg);
    cfg.port[0].phy_addr = EMAC_CPSW_PORT0_PHY_ADDR_EVM;
    cfg.port[1].phy_addr = EMAC_CPSW_PORT0_PHY_ADDR_EVM;
    cfg.macModeFlags = EMAC_CPSW_CONFIG_MODEFLG_FULLDUPLEX;
    EMAC_socSetInitCfg(0, &cfg);

    /* Step 4: Create main application task (priority 1, stack 0x1400) */
    Task_Params_init(&taskParams);
    taskParams.priority = 1;
    taskParams.stackSize = 0x1400;
    main_task = Task_create (TaskFxn, &taskParams, NULL);

    /* Step 5: Create Ethernet statistics task (priority 9, name "EmacStats") */
    Task_Params_init(&taskParams);
    taskParams.priority = 9;
    taskParams.instance->name = "EmacStats";
    Task_create(app_stats, &taskParams, NULL);

    /* Step 6: Register NIMU device init callback */
    NIMUDeviceTable[nimu_device_index++].init =  &CpswEmacInit ;
    NIMUDeviceTable[nimu_device_index].init =  NULL ;

    /* Step 7: Start SYS/BIOS scheduler (never returns) */
    BIOS_start();

    return -1;
}

/* ========================================================================== */
/*                        Application Task                                    */
/* ========================================================================== */

/**
 *  \brief Main application task
 *
 *  Prints the board IP address banner and initializes the FTP server.
 *  Runs at priority 1.
 */
void TaskFxn(UArg a0, UArg a1)
{
    /* Print banner with IP address */
    NIMU_log("\n\r========================================\n\r");
    NIMU_log("\n\r  SYS/BIOS Ethernet/IP (CPSW) Sample\n\r");
    NIMU_log("\n\r  Board: AM3352 (BeagleBone Black class)\n\r");
    NIMU_log("\n\r  Interface: CPSW (MII mode)\n\r");
    NIMU_log("\n\r  IP Address: %s\n\r", LocalIPAddr);
    NIMU_log("\n\r  FTP Server: Port 21\n\r");
    NIMU_log("\n\r  Credentials: user / password\n\r");
    NIMU_log("\n\r========================================\n\r");

    /* Initialize FTP server — spawns server task internally */
    ftpserver_init();
}
