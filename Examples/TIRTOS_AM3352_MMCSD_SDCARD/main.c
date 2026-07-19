/**
 *  \file   main.c
 *
 *  \brief  SDMMC + SDCARD example for AM335x (TI-RTOS / SYS/BIOS)
 *
 *  Demonstrates basic SD card operations:
 *    1. Open SDMMC driver
 *    2. Read card parameters (block size, count, total size)
 *    3. Write pattern to sectors
 *    4. Read back and verify
 *    5. Overwrite (update) same sectors with new pattern
 *    6. Read back and verify updated data
 *
 *  Hardware   : AM3352 (BeagleBone Black class)
 *  Toolchain  : GCC ARM Embedded 7.3.1
 *  PDK        : pdk_am335x_1_0_17
 *  SYS/BIOS   : 6.76.03.01
 */

#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <stdio.h>
#include <string.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/Error.h>
#include <ti/sysbios/BIOS.h>
#include <ti/board/board.h>

#include <ti/drv/mmcsd/MMCSD.h>
#include <ti/drv/mmcsd/soc/MMCSD_soc.h>
#include "MMCSD_log.h"
#include "helper.h"

/* ---------------------------------------------------------------------------
 *  Constants
 * ------------------------------------------------------------------------- */
#define SECTORSIZE         512        /* SD card sector / block size         */
#define TEST_SECTOR_START  0x300000   /* 1.5 GB offset — safe for most cards */
#define NUM_TEST_SECTORS   4          /* number of 512-byte sectors to test  */
#define DATA_BUF_ALIGN     256        /* DMA alignment requirement           */
#define TEST_BUF_SIZE      (SECTORSIZE * NUM_TEST_SECTORS)

/* ---------------------------------------------------------------------------
 *  Buffers  (placed in .benchmark_buffer section via linker .cfg)
 * ------------------------------------------------------------------------- */
uint8_t write_buf[TEST_BUF_SIZE] __attribute__((aligned(DATA_BUF_ALIGN))) __attribute__((section(".benchmark_buffer")));
uint8_t read_buf[TEST_BUF_SIZE]  __attribute__((aligned(DATA_BUF_ALIGN))) __attribute__((section(".benchmark_buffer")));
uint8_t update_buf[TEST_BUF_SIZE] __attribute__((aligned(DATA_BUF_ALIGN))) __attribute__((section(".benchmark_buffer")));

/* ---------------------------------------------------------------------------
 *  SDMMC Task  -  runs after BIOS starts
 * ------------------------------------------------------------------------- */
void mmcsd_task(UArg arg0, UArg arg1)
{
    MMCSD_Handle handle = NULL;
    MMCSD_mediaParams mediaParams;
    MMCSD_Error ret;

    /* ---- Step 1 : Init driver ---- */
    MMCSD_init();

    ret = MMCSD_open(0, NULL, &handle);
    MMCSD_CHECK(ret, handle, "MMCSD_Open() failed\n");
    MMCSD_log("MMCSD_Open() completed successfully\n");



    /* ---- Step 2 : Query SD card parameters ---- */
    MMCSD_log("Getting SD Card parameters\n");
    ret = MMCSD_control(handle, MMCSD_CMD_GETMEDIAPARAMS, (void *)&mediaParams);
    MMCSD_CHECK(ret, handle, "Get media params failed!\n");
    MMCSD_log("SD Card: BlockSize = %d,  BlockCount = 0x%x%x, CardSize = 0x%x%x bytes\n",
              mediaParams.blockSize,
              (unsigned int)(mediaParams.blockCount >> 32),
              (unsigned int)(mediaParams.blockCount & 0xFFFFFFFF),
              (unsigned int)(mediaParams.size >> 32),
              (unsigned int)(mediaParams.size & 0xFFFFFFFF));



    /* ---- Prepare test data ---- */
    fill_pattern(write_buf,  TEST_BUF_SIZE, 0xAA000000);
    fill_pattern(update_buf, TEST_BUF_SIZE, 0x55000000);

    /* ---- Step 3 : Write pattern to card ---- */
    MMCSD_log("\nWriting %d bytes to sector 0x%x ...\n", TEST_BUF_SIZE, TEST_SECTOR_START);
    ret = MMCSD_write(handle, write_buf, TEST_SECTOR_START, NUM_TEST_SECTORS);
    MMCSD_CHECK(ret, handle, "Write failed!\n");
    MMCSD_log("Write OK\n");



    /* ---- Step 4 : Read back and verify ---- */
    MMCSD_log("Reading back %d bytes from sector 0x%x ...\n", TEST_BUF_SIZE, TEST_SECTOR_START);
    memset(read_buf, 0, TEST_BUF_SIZE);
    ret = MMCSD_read(handle, read_buf, TEST_SECTOR_START, NUM_TEST_SECTORS);
    MMCSD_CHECK(ret, handle, "Read failed!\n");
    MMCSD_log("Read OK\n");

    VERIFY_DATA(handle, write_buf, read_buf, TEST_BUF_SIZE, "Data MISMATCH after write+read!\n");
    MMCSD_log("Data verified: write -> read matched\n");



    /* ---- Step 5 : Overwrite (update) with new pattern ---- */
    MMCSD_log("\nUpdating (overwriting) same sectors with new pattern ...\n");
    ret = MMCSD_write(handle, update_buf, TEST_SECTOR_START, NUM_TEST_SECTORS);
    MMCSD_CHECK(ret, handle, "Update write failed!\n");
    MMCSD_log("Update write OK\n");



    /* ---- Step 6 : Read updated data and verify ---- */
    MMCSD_log("Reading back updated data ...\n");
    memset(read_buf, 0, TEST_BUF_SIZE);
    ret = MMCSD_read(handle, read_buf, TEST_SECTOR_START, NUM_TEST_SECTORS);
    MMCSD_CHECK(ret, handle, "Read after update failed!\n");
    MMCSD_log("Read after update OK\n");

    VERIFY_DATA(handle, update_buf, read_buf, TEST_BUF_SIZE, "Data MISMATCH after update!\n");
    MMCSD_log("Data verified: update -> read matched\n");



    MMCSD_log("\nAll SDMMC SDCARD tests PASSED\n");
    MMCSD_close(handle);
}

/* ---------------------------------------------------------------------------
 *  main  -  board init, create task, start SYS/BIOS
 * ------------------------------------------------------------------------- */
int main(void)
{
    Board_initCfg boardCfg = BOARD_INIT_PINMUX_CONFIG | BOARD_INIT_UART_STDIO | BOARD_INIT_MODULE_CLOCK;

    if (Board_init(boardCfg) != BOARD_SOK) {
        System_printf("Board_init failed!\n");
        while (1);
    }

    Error_Block eb;
    Error_init(&eb);
    Task_Handle task = Task_create(mmcsd_task, NULL, &eb);
    if (task == NULL) {
        System_printf("Task_create() failed!\n");
        BIOS_exit(0);
    }

    BIOS_start();
    return 0;
}
