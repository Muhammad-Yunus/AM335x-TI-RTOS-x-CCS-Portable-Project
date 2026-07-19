/**
 * =============================================================================
 * PROJECT: TIRTOS_AM3352_MMCSD_SDCARD_FATFS
 * =============================================================================
 * TARGET:   AM3352 (Board mirip Beaglebone Black)
 * CPU:      Cortex-A8 @ 600MHz
 * OS:       TI-RTOS (SYS/BIOS)
 * PDK:      pdk_am335x_1_0_17
 * TOOLCHAIN: GNU ARM GCC 7.3.1
 * =============================================================================
 *
 * DESCRIPTSI:
 *   Aplikasi ini mendemonstrasikan penggunaan MMCSD (MultiMediaCard/Secure
 *   Digital Card) dengan filesystem FATFS pada platform TI AM335x.
 *
 *   Fitur utama:
 *   - Inisialisasi pinmux, clock, dan UART untuk console I/O
 *   - Deteksi card insert/remove via GPIO interrupt (SD_CD pin)
 *   - Mount/unmount filesystem FATFS pada SD Card
 *   - Interactive shell console melalui UART untuk operasi file:
 *       ls    - List file dan direktori
 *       cd    - Change directory
 *       pwd   - Print working directory
 *       mkdir - Create directory
 *       rm    - Delete file/directory
 *       cat   - Read file content / file copy
 *       help  - Tampilkan daftar perintah
 * =============================================================================
 *
 * ARSITEKTUR TASK (TI-RTOS):
 * ============================================================================
 * | Priority | Task Name            | Fungsi                                      |
 * |==========|======================|=============================================|
 * |    2     | FATFFS_Driver        | Monitor SD_CD pin, open/close FATFS         |
 * |    1     | FShellUtilsProcess   | Interactive shell untuk operasi file        |
 * ============================================================================
 *
 * ALUR KERJA:
 *   1. Board Init     -> Pinmux, Clock, UART
 *   2. GPIO Init      -> Setup SD_CD pin sebagai input dengan interrupt
 *   3. FATFS Init     -> Register driver MMCSD ke layer FATFS
 *   4. Task Create    -> Buat 2 task (FATFS driver + Shell utils)
 *   5. BIOS Start     -> Masuk ke event loop
 *   6. Card Insert    -> GPIO interrupt trigger -> FATFS mount -> Shell ready
 *   7. Card Remove    -> GPIO interrupt trigger -> FATFS unmount -> Wait
 * =============================================================================
 *
 * FILE STRUKTUR:
 *   main.c              -> Entry point, task creation, board init
 *   fs_shell_app_utils.c -> Interactive file shell implementation
 *   GPIO_soc.c          -> GPIO SOC-specific configuration (AM335x)
 *   UART_soc.c          -> UART SOC-specific configuration (AM335x)
 *   MMCSD_log.h         -> Log utility header
 *   fs_shell_app_utils.h -> Shell utility header
 *   mmcsd_bbbam335x.cfg -> SYS/BIOS configuration file
 * =============================================================================
 *
 * HARDWARE CONNECTION:
 *   - SD Card Slot: MMCSD Controller (Slot 0)
 *   - SD_CD (Card Detect): GPIO port pin (terhubung ke pin deteksi card)
 *   - UART Console: UART0 untuk interaksi shell
 * =============================================================================
 *
 * BUILD CONFIGURATION:
 *   Define Symbols:
 *     am3359          -> Platform target
 *     USE_BIOS        -> Menggunakan SYS/BIOS
 *     SOC_AM335x      -> SOC family AM335x
 *     bbbAM335x       -> Board variant (BeagleBone Black-like)
 *
 *   Include Paths:
 *     Project root
 *     PDK: C:/ti/pdk_am335x_1_0_17/packages
 *     GCC: C:/ti/gcc-arm-none-eabi-7-2018-q2-update
 * =============================================================================
 *
 * NOTES:
 *   - MMCSD berjalan dalam mode PIO (tanpa DMA)
 *   - UART TX/RX berjalan dengan DMA enabled
 *   - Stack size: 0x2048 (8KB) per task
 *   - Heap size: 0x400 (1KB)
 * =============================================================================
 */

#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <stdio.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>
#include <xdc/runtime/Error.h>
#include <ti/csl/cslr_device.h>
#include "MMCSD_log.h"
#include <ti/fs/fatfs/diskio.h>
#include <ti/fs/fatfs/FATFS.h>
#include <ti/drv/mmcsd/MMCSD.h>
#include <ti/drv/mmcsd/soc/MMCSD_soc.h>
#include <ti/drv/mmcsd/example/fatfs_console/src/fs_shell_app_utils.h>
#include <ti/board/board.h>
#include <ti/drv/gpio/GPIO.h>
#include <ti/drv/gpio/soc/GPIO_soc.h>

/* ============================================================================
 * MACROS & CONSTANTS
 * ============================================================================ */

/* State aktif GPIO SD_CD (Card Detect) - sesuaikan dengan hardware wiring */
#define GPIO_PIN_MMCSD_ACTIVE_STATE 0U

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/* Enum untuk GPIO pin yang digunakan */
typedef enum GPIO_PIN {
    GPIO_PIN_MMC_SDCD = 0U,   /* Pin untuk mendeteksi SD Card Insert/Remove */
    GPIO_PIN_COUNT
} GPIO_PIN;

/* ============================================================================
 * GLOBAL VARIABLES - GPIO CONFIGURATION
 * ============================================================================ */

/* Pin config: Input dengan interrupt pada kedua edge (insert & remove) */
GPIO_PinConfig gpioPinConfigs[] = {
    GPIO_DEVICE_CONFIG(GPIO_MMC_SDCD_PORT_NUM, GPIO_MMC_SDCD_PIN_NUM) |
    GPIO_CFG_IN_INT_BOTH_EDGES | GPIO_CFG_INPUT,
};

/* Callback functions untuk setiap GPIO pin (NULL = tidak ada callback per-pin) */
GPIO_CallbackFxn gpioCallbackFunctions[] = { NULL };

/* GPIO v1 configuration structure untuk AM335x */
GPIO_v1_Config GPIO_v1_config = {
    gpioPinConfigs,
    gpioCallbackFunctions,
    sizeof(gpioPinConfigs) / sizeof(GPIO_PinConfig),
    sizeof(gpioCallbackFunctions) / sizeof(GPIO_CallbackFxn),
    0,
};

/* ============================================================================
 * GLOBAL VARIABLES - FATFS / MMCSD DRIVER CONFIGURATION
 * ============================================================================ */

/* Function table: menghubungkan FATFS layer dengan MMCSD driver */
FATFS_DrvFxnTable FATFS_drvFxnTable = {
    MMCSD_close, MMCSD_control, MMCSD_init, MMCSD_open, MMCSD_write, MMCSD_read
};

/* Hardware attributes: mapping volume ID ke MMCSD instance */
FATFS_HwAttrs FATFS_initCfg[_VOLUMES] = {
    {0U}, {1U}, {2U}, {3U}
};

/* FATFS objects: satu per volume */
FATFS_Object FATFS_objects[_VOLUMES];

/* FATFS configuration array: 4 volume aktif + 1 NULL terminator */
const FATFS_Config FATFS_config[_VOLUMES + 1] = {
    {&FATFS_drvFxnTable, &FATFS_objects[0], &FATFS_initCfg[0]},
    {&FATFS_drvFxnTable, &FATFS_objects[1], &FATFS_initCfg[1]},
    {&FATFS_drvFxnTable, &FATFS_objects[2], &FATFS_initCfg[2]},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL}
};

/* ============================================================================
 * GLOBAL VARIABLES - RUNTIME STATE
 * ============================================================================ */

FATFS_Handle fatfsHandle = NULL;       /* Handle ke filesystem yang sedang mounted */
Semaphore_Handle hSem = NULL;          /* Semaphore untuk sinkronisasi card detect */
Uint32 fatfsShellProcessFlag = 0;      /* Flag: 1=card inserted, 0=card removed */

/* Function prototypes */
void AppGpioCallbackFxn(void);
void board_initGPIO(void);
void mmcsd_fatfs_console(UArg arg0, UArg arg1);
void fatfs_console_taskFn(UArg arg0, UArg arg1);

/* ============================================================================
 * FUNCTION: board_initGPIO
 * ============================================================================
 * Deskripsi: Inisialisasi GPIO board-specific untuk AM335x.
 *             Pada AM335x, fungsi ini kosong karena konfigurasi GPIO sudah
 *             ditangani oleh GPIO_soc.c dan gpioPinConfigs di atas.
 * ============================================================================ */
void board_initGPIO(void)
{
}

/* ============================================================================
 * TASK: mmcsd_fatfs_console (Priority: 2)
 * ============================================================================
 * Deskripsi: Task utama yang memantau status SD Card via GPIO interrupt.
 *             Ketika card terdeteksi insert, task akan:
 *               1. Menginisialisasi GPIO dan FATFS
 *               2. Membuka (mount) filesystem pada volume 0
 *               3. Mengaktifkan flag untuk shell processing
 *
 *             Ketika card terdeteksi remove, task akan:
 *               1. Menonaktifkan flag shell processing
 *               2. Menutup (unmount) filesystem
 *               3. Menunggu card dimasukkan kembali
 *
 * Alur:
 *   [Start] -> Board GPIO Init -> GPIO Init -> FATFS Init
 *            -> Loop: Pend Semaphore (tunggu interrupt)
 *            -> Jika Card Insert -> FATFS Open -> Shell Active
 *            -> Jika Card Remove -> Shell Inactive -> FATFS Close
 * ============================================================================ */
void mmcsd_fatfs_console(UArg arg0, UArg arg1)
{
    /* Inisialisasi GPIO board-specific */
    board_initGPIO();

    /* Init GPIO driver dan setup callback untuk card detect */
    GPIO_init();
    GPIO_setCallback(GPIO_PIN_MMC_SDCD, AppGpioCallbackFxn);
    GPIO_enableInt(GPIO_PIN_MMC_SDCD);

    /* Inisialisasi FATFS dengan driver table MMCSD */
    FATFS_init();

    /* Cek status awal: jika card sudah terpasang saat boot */
    if (GPIO_PIN_MMCSD_ACTIVE_STATE == GPIO_read(GPIO_PIN_MMC_SDCD))
    {
        Semaphore_post(hSem);
    }
    else
    {
        MMCSD_log("\nPlease insert card.\r\n");
    }

    /* Main loop: tunggu interrupt dari GPIO card detect */
    while(1)
    {
        Semaphore_pend(hSem, BIOS_WAIT_FOREVER);

        if (GPIO_PIN_MMCSD_ACTIVE_STATE == GPIO_read(GPIO_PIN_MMC_SDCD))
        {
            /* Card INSERTED */
            FATFS_open(0U, NULL, &fatfsHandle);
            fatfsShellProcessFlag = 1;
            MMCSD_log("\nCard inserted.\r\n");
        }
        else
        {
            /* Card REMOVED */
            fatfsShellProcessFlag = 0;
            MMCSD_log("\nCard Removed.\r\n");
            MMCSD_log("\nPlease insert card.\r\n");
            FATFS_close(fatfsHandle);
        }
    }
}

/* ============================================================================
 * TASK: fatfs_console_taskFn (Priority: 1)
 * ============================================================================
 * Deskripsi: Task untuk interactive file shell.
 *             Menjalankan loop proses shell yang menerima command dari UART
 *             dan mengeksekusi operasi file pada SD Card.
 *
 * Command yang tersedia:
 *   help    - Tampilkan daftar perintah
 *   ls      - List file/direktori
 *   cd      - Change directory
 *   pwd     - Print working directory
 *   mkdir   - Create directory
 *   rm      - Delete file/directory
 *   cat     - Read file atau copy file
 * ============================================================================ */
void fatfs_console_taskFn(UArg arg0, UArg arg1)
{
    FSShellAppUtilsProcess();
}

/* ============================================================================
 * FUNCTION: main
 * ============================================================================
 * Deskripsi: Entry point aplikasi.
 *
 * Langkah-langkah:
 *   1. Board init (pinmux, clock, UART stdio)
 *   2. Buat semaphore untuk sinkronisasi card detect
 *   3. Buat task FATFS Driver (priority 2) - monitor SD card
 *   4. Buat task Shell Utils (priority 1) - proses command user
 *   5. Start BIOS (multitasking dimulai)
 * ============================================================================ */
int main(void)
{
    Task_Handle task;
    Error_Block eb;
    Task_Params taskParams;
    Semaphore_Params semParams;
    Board_initCfg boardCfg;

    /* Step 1: Inisialisasi board - pinmux, module clock, UART */
    boardCfg = BOARD_INIT_PINMUX_CONFIG | BOARD_INIT_MODULE_CLOCK | BOARD_INIT_UART_STDIO;
    Board_init(boardCfg);

    /* Step 2: Setup semaphore dan task parameters */
    Task_Params_init(&taskParams);
    Semaphore_Params_init(&semParams);
    Error_init(&eb);

    /* Create semaphore untuk card detect signaling */
    hSem = Semaphore_create(0, &semParams, &eb);

    /* Step 3: Buat task FATFS Driver (priority 2 - lebih tinggi) */
    taskParams.instance->name = "FATFFS_Driver";
    taskParams.priority = 2;
    taskParams.stackSize = 0x2048;  /* 8 KB stack */
    task = Task_create(mmcsd_fatfs_console, &taskParams, &eb);
    if (task == NULL) {
        System_printf("Task_create() failed!\n");
        BIOS_exit(0);
    }

    /* Step 4: Buat task Shell Utils (priority 1) */
    taskParams.instance->name = "FShellUtilsProcess";
    taskParams.priority = 1;
    taskParams.stackSize = 0x2048;  /* 8 KB stack */
    task = Task_create(fatfs_console_taskFn, &taskParams, &eb);
    if (task == NULL) {
        System_printf("Task_create() failed!\n");
        BIOS_exit(0);
    }

    /* Step 5: Start BIOS - multitasking dimulai */
    BIOS_start();
    return (0);
}

/* ============================================================================
 * CALLBACK: AppGpioCallbackFxn
 * ============================================================================
 * Deskripsi: Dipanggil setiap kali GPIO interrupt terjadi pada pin SD_CD.
 *             Meng-post semaphore untuk membangunkan task mmcsd_fatfs_console
 *             agar memeriksa status card (insert/remove).
 * ============================================================================ */
void AppGpioCallbackFxn(void)
{
    Semaphore_post(hSem);
}
