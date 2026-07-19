/**
 * =============================================================================
 * FILE: fs_shell_app_utils.h
 * =============================================================================
 * Deskripsi: Header untuk interactive file shell utilities.
 *             Menyediakan definisi struktur, enum, macro, dan function prototype
 *             yang digunakan untuk operasi filesystem via UART console.
 *
 * Author:    Texas Instruments (TI PDK)
 * Modified:  Add comprehensive comments for understanding
 * =============================================================================
 */

#ifndef APP_UTILS_FS_SHELL_H_
#define APP_UTILS_FS_SHELL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* MACROS & CONSTANTS                                                         */
/* ========================================================================== */

/* Cache line size untuk alignment data (AM335x L1D cache = 32KB, line = 32 byte) */
#define SOC_CACHELINE_SIZE                             (64U)

/* Max karakter input user (termasuk null terminator) */
#define UI_APP_UTILS_MAX_INPUT_SIZE                      (80U)

/* Pesan navigasi: '0'=Home, Enter=Back */
#define UI_APP_UTILS_PAGE_HELP       (" 0 - Home; (carriage return) - Back")

/* Max jumlah arguments dalam satu command (misal: "ls /dir/file.txt" = 2 args) */
#define FSSHELLAPPUTILS_CMDLINE_MAX_ARGS               (8U)

/* Return value: command tidak ditemukan */
#define FSSHELLAPPUTILS_CMDLINE_BAD_CMD                (-1)

/* Return value: terlalu banyak arguments */
#define FSSHELLAPPUTILS_CMDLINE_TOO_MANY_ARGS          (-2)

/* Cache line size khusus untuk shell app */
#define FS_SHELL_APP_UTILS_CACHELINE_SIZE 32

/* ========================================================================== */
/* ENUM: Status eksekusi                                                      */
/* ========================================================================== */

/**
 * Enum untuk status eksekusi command.
 * Nilai: MIN=0, COMPLETE=0, INPROGRESS=1, ERROR=2, MAX=2
 */
typedef enum fsShellAppUtilsStatus
{
    FS_SHELL_APP_UTILS_STATUS_MIN = 0U,
    FS_SHELL_APP_UTILS_STATUS_COMPLETE = FS_SHELL_APP_UTILS_STATUS_MIN,
    FS_SHELL_APP_UTILS_STATUS_INPROGRESS = FS_SHELL_APP_UTILS_STATUS_COMPLETE + 1U,
    FS_SHELL_APP_UTILS_STATUS_ERROR = FS_SHELL_APP_UTILS_STATUS_INPROGRESS + 1U,
    FS_SHELL_APP_UTILS_STATUS_MAX = FS_SHELL_APP_UTILS_STATUS_ERROR
} fsShellAppUtilsStatus_t;

/* ========================================================================== */
/* ENUM: State Machine                                                        */
/* ========================================================================== */

/**
 * Enum untuk state machine file shell.
 *
 * State flow:
 *   HELP -> READ_COMMAND -> EXECUTE_COMMAND -> HELP -> ...
 */
typedef enum fsShellAppUtilsState
{
    FS_SHELL_APP_UTILS_STATE_HELP,
    /**< Menampilkan prompt dan siap menerima command */
    FS_SHELL_APP_UTILS_STATE_READ_COMMAND,
    /**< Membaca input karakter dari UART */
    FS_SHELL_APP_UTILS_STATE_EXECUTE_COMMAND,
    /**< Mem-parse dan mengeksekusi command */
    FS_SHELL_APP_UTILS_STATE_MAX
    /**< State idle/max, menunggu card insert */
} fsShellAppUtilsState_t;

/* ========================================================================== */
/* ENUM: Status Command                                                       */
/* ========================================================================== */

/**
 * Enum untuk status hasil eksekusi command.
 */
typedef enum fsShellAppUtilsCmdStatus
{
    FS_SHELL_APP_UTILS_CMD_STATUS_PASS,
    /**< Command berhasil dieksekusi */
    FS_SHELL_APP_UTILS_CMD_STATUS_TOO_MANY_ARG,
    /**< Terlalu banyak arguments */
    FS_SHELL_APP_UTILS_CMD_STATUS_FAIL,
    /**< Command gagal dieksekusi */
    FS_SHELL_APP_UTILS_CMD_STATUS_INVALID
    /**< Command tidak valid/dikenali */
} fsShellAppUtilsCmdStatus_t;

/* ========================================================================== */
/* STRUCT: Command Entry                                                      */
/* ========================================================================== */

/**
 * Struktur untuk mendefinisikan satu entry command dalam command table.
 *
 * Format:
 *   { "nama_cmd", &fungsi_handler, "deskripsi_help" }
 *
 * Contoh:
 *   { "ls", &FSShellAppUtilsCmdLs, "   : Display list of files" }
 */
typedef struct fsShellAppUtilsCmdEntry
{
    const char *pCmd;
    /**< Nama command (string, dipakai untuk matching) */
    int32_t (*pfnCmd) (int32_t argc, char *argv[]);
    /**< Pointer ke fungsi handler. Menerima argc/argv seperti main() */
    const char *pHelp;
    /**< Teks bantuan yang ditampilkan saat command 'help' dipanggil */
} fsShellAppUtilsCmdEntry_t;

/* ========================================================================== */
/* STRUCT: Console Interface (tidak digunakan saat ini)                       */
/* ========================================================================== */

/**
 * Struktur interface console.
 * Saat ini tidak digunakan karena UART handled langsung oleh UART driver.
 */
typedef struct fsShellAppUtilsConsoleInterface
{
    uint32_t opMode;
    /**< Operating mode: blocking atau non-blocking */
    uint8_t* pRxConsoleBuf;
    /**< Buffer untuk menyimpan input user */
    uint32_t readStatus;
    /**< Status pembacaan buffer */
} fsShellAppUtilsConsoleInterface_t;

/* ========================================================================== */
/* FUNCTION DECLARATIONS                                                      */
/* ========================================================================== */

/**
 * FSShellAppUtilsInit()
 *
 * Inisialisasi shell utilities.
 * Saat ini belum diimplementasikan (return INVALID).
 *
 * \return Status inisialisasi
 */
int32_t FSShellAppUtilsInit(void);

/**
 * FSShellAppUtilsProcess()
 *
 * Main loop state machine untuk file shell.
 * Fungsi ini memanggil diri sendiri secara rekursif (via spinProcess loop)
 * untuk menangani state machine secara non-blocking.
 *
 * \return Status eksekusi
 *   - CSL_SOK: berhasil
 *   - CSL_ESYS_FAIL: gagal (misal: card tidak terpasang)
 */
int32_t FSShellAppUtilsProcess(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef APP_UTILS_FS_SHELL_H_ */
