/**
 * =============================================================================
 * FILE: fs_shell_app_utils.c
 * =============================================================================
 * Deskripsi: Implementasi interactive file shell untuk operasi FATFS pada SD Card.
 *             File ini berisi state machine dan command processor yang memungkinkan
 *             user berinteraksi dengan filesystem melalui UART console.
 *
 * State Machine:
 *   [MAX] -> [HELP] -> [READ_COMMAND] -> [EXECUTE_COMMAND] -> [HELP] -> ...
 *
 *   MAX      : Idle state, menunggu card insert & directory valid
 *   HELP     : Menampilkan prompt dan memverifikasi directory accessible
 *   READ_CMD : Membaca input karakter dari UART (dengan backspace support)
 *   EXEC_CMD : Parse command line dan eksekusi command yang sesuai
 * =============================================================================
 */

#include <ti/csl/tistdtypes.h>
#include "string.h"
#include "stdio.h"
#include <ti/csl/csl_error.h>
#include <ti/fs/fatfs/ff.h>
#include "MMCSD_log.h"
#include "fs_shell_app_utils.h"

/* Buffer sizes */
#define FS_SHELL_APP_UTILS_PATH_BUF_SIZE   512   /* Max path length */
#define FS_SHELL_APP_UTILS_DATA_BUF_SIZE   512   /* Max data/temp buffer length */
#define FS_SHELL_APP_UTILS_CMD_CMD_BUF_SIZE    512 /* Max command line length */

/* Help info strings untuk setiap command */
#define FS_SHELL_APP_UTILS_CMD_INFO_CAT                                            \
        "  : Show contents of a text file : cat <FILENAME> \n"                 \
                   "       Write to a file : cat <INPUTFILE> > <OUTPUTFILE> \n" \
                    "       Read from UART : cat dev.UART \n"                   \
                   "       Write from UART : cat dev.UART > <OUTPUTFILE>"
#define FS_SHELL_APP_UTILS_CMD_INFO_HELP  " : Display list of commands"
#define FS_SHELL_APP_UTILS_CMD_INFO_LS  "   : Display list of files"
#define FS_SHELL_APP_UTILS_CMD_INFO_CD     ": Change directory"
#define FS_SHELL_APP_UTILS_CMD_INFO_MKDIR  ": Create directory"
#define FS_SHELL_APP_UTILS_CMD_INFO_PWD  "  : Show current working directory"
#define FS_SHELL_APP_UTILS_CMD_INFO_RM  "   : Delete a file or an empty directory"

/* Function prototypes */
int32_t FSShellAppUtilsCmdExecute(uint8_t *pCmdLine, fsShellAppUtilsCmdEntry_t *pCmdList);
int32_t FSShellAppUtilsCmdLs(int32_t argc, char *argv[]);
int32_t FSShellAppUtilsCmdCd(int32_t argc, char *argv[]);
int32_t FSShellAppUtilsCmdPwd(int32_t argc, char *argv[]);
int32_t FSShellAppUtilsCmdMkDir(int32_t argc, char *argv[]);
int32_t FSShellAppUtilsCmdRm(int32_t argc, char *argv[]);
int32_t FSShellAppUtilsCmdCat(int32_t argc, char *argv[]);
int32_t FSShellAppUtilsCmdHelp(int32_t argc, char *argv[]);

/* ============================================================================
 * GLOBAL VARIABLES - STATE MACHINE & BUFFERS
 * ============================================================================ */

static DIR gFsShellAppUtilsDirObj;                /* Directory object untuk f_opendir/f_readdir */
static FILINFO gFsShellAppUtilsFileInfo;           /* File info object untuk f_readdir */
volatile uint32_t gFsShellAppUtilsCurState = FS_SHELL_APP_UTILS_STATE_MAX; /* Current FSM state */
static FIL gFsShellAppUtilsReadFileObj  __attribute__ ((aligned (SOC_CACHELINE_SIZE)));  /* Read file handle */
static FIL gFsShellAppUtilsWriteFileObj  __attribute__ ((aligned (SOC_CACHELINE_SIZE))); /* Write file handle */
static char gFsShellAppUtilsTempPath[FS_SHELL_APP_UTILS_PATH_BUF_SIZE] __attribute__ ((aligned (SOC_CACHELINE_SIZE))); /* Temp path buffer */
static char gFsShellAppUtilsCwd[FS_SHELL_APP_UTILS_DATA_BUF_SIZE] __attribute__ ((aligned (SOC_CACHELINE_SIZE))); /* Current working directory */
static char gFsShellAppUtilsDataBuf[FS_SHELL_APP_UTILS_DATA_BUF_SIZE] __attribute__ ((aligned (SOC_CACHELINE_SIZE))); /* Data read/write buffer */

/* ============================================================================
 * COMMAND TABLE
 * ============================================================================
 * Daftar command yang didukung. Urutan tidak penting karena dicari linear.
 * Entry terakhir harus {0,0,0} sebagai terminator.
 * ============================================================================ */
fsShellAppUtilsCmdEntry_t gFsShellAppUtilsCmdTable[] = {
    { "help",     &FSShellAppUtilsCmdHelp,  FS_SHELL_APP_UTILS_CMD_INFO_HELP},
    { "ls",       &FSShellAppUtilsCmdLs,    FS_SHELL_APP_UTILS_CMD_INFO_LS},
    { "cd",       &FSShellAppUtilsCmdCd,    FS_SHELL_APP_UTILS_CMD_INFO_CD},
    { "mkdir",    &FSShellAppUtilsCmdMkDir, FS_SHELL_APP_UTILS_CMD_INFO_MKDIR},
    { "rm",       &FSShellAppUtilsCmdRm,    FS_SHELL_APP_UTILS_CMD_INFO_RM},
    { "pwd",      &FSShellAppUtilsCmdPwd,   FS_SHELL_APP_UTILS_CMD_INFO_PWD},
    { "cat",      &FSShellAppUtilsCmdCat,   FS_SHELL_APP_UTILS_CMD_INFO_CAT},
    { 0, 0, 0 }
};

/* Input buffer untuk command line (max 80 karakter) */
uint8_t gFsShellAppUtilsRxBuf[80U];
/* Flag dari main.c: 1=card inserted & shell active, 0=card removed */
extern Uint32 fatfsShellProcessFlag;

/* ============================================================================
 * FUNCTION: FSShellAppUtilsInit
 * ============================================================================
 * Deskripsi: Placeholder untuk inisialisasi shell. Saat ini belum digunakan.
 * ============================================================================ */
int32_t FSShellAppUtilsInit(void)
{
    return(FS_SHELL_APP_UTILS_CMD_STATUS_INVALID);
}

/* ============================================================================
 * FUNCTION: FSShellAppUtilsProcess
 * ============================================================================
 * Deskripsi: Main loop state machine untuk file shell.
 *
 * Alur:
 *   1. STATE_MAX  -> Cek card insert, set CWD ke "0:", masuk STATE_HELP
 *   2. STATE_HELP -> Verifikasi directory accessible, tampilkan prompt
 *   3. STATE_READ -> Baca karakter dari UART sampai Enter ditekan
 *   4. STATE_EXEC -> Parse command line, cari di command table, eksekusi
 *   5. Kembali ke STATE_HELP untuk prompt berikutnya
 * ============================================================================ */
int32_t FSShellAppUtilsProcess(void)
{
    uint32_t spinProcess = TRUE;
    int32_t retStat = CSL_ESYS_FAIL;
    uint8_t inputChar = 0U;
    uint8_t inputCharIdx = 0U;

    do
    {
        switch(gFsShellAppUtilsCurState)
        {
            /* ---- STATE: HELP (prompt ready) ---- */
            case FS_SHELL_APP_UTILS_STATE_HELP:
                if(FR_OK == f_opendir(&gFsShellAppUtilsDirObj, gFsShellAppUtilsCwd))
                {
                    UART_printStatus("All tests have PASSED\n");
                    MMCSD_log("%s>", gFsShellAppUtilsCwd);
                    gFsShellAppUtilsCurState = FS_SHELL_APP_UTILS_STATE_READ_COMMAND;
                    spinProcess = TRUE;
                    retStat = CSL_SOK;
                }
                else
                {
                    MMCSD_log("%s>", "UNKNOWN");
                    gFsShellAppUtilsCurState = FS_SHELL_APP_UTILS_STATE_MAX;
                    spinProcess = TRUE;
                    retStat = CSL_ESYS_FAIL;
                }
                break;

            /* ---- STATE: READ_COMMAND (baca input dari UART) ---- */
            case FS_SHELL_APP_UTILS_STATE_READ_COMMAND:
                if(FR_OK == f_opendir(&gFsShellAppUtilsDirObj, gFsShellAppUtilsCwd))
                {
                    inputCharIdx = 0;
                    do
                    {
                        inputChar = UART_getc();

                        /* Backspace: hapus karakter terakhir */
                        if((inputChar == '\b') && (inputCharIdx != 0))
                        {
                            UART_printf("\b \b");
                            inputCharIdx--;
                            gFsShellAppUtilsRxBuf[inputCharIdx] = '\0';
                        }
                        /* Enter/Newline: selesaikan input */
                        else if((inputChar == '\r') || (inputChar == '\n'))
                        {
                            UART_printf("\n");
                            break;
                        }
                        /* Karakter printable: tambahkan ke buffer */
                        else if((inputChar >= ' ') && (inputChar <= '~') && (inputCharIdx < (sizeof(gFsShellAppUtilsRxBuf) - 1)))
                        {
                            gFsShellAppUtilsRxBuf[inputCharIdx++] = inputChar;
                            gFsShellAppUtilsRxBuf[inputCharIdx] = '\0';
                            UART_printf("%c", inputChar);
                        }
                    } while (1);

                    /* Hanya proses jika card masih terpasang */
                    if(fatfsShellProcessFlag != 0)
                    {
                        gFsShellAppUtilsCurState = FS_SHELL_APP_UTILS_STATE_EXECUTE_COMMAND;
                        spinProcess = TRUE;
                        retStat = CSL_SOK;
                    }
                    else
                    {
                        gFsShellAppUtilsCurState = FS_SHELL_APP_UTILS_STATE_MAX;
                        spinProcess = TRUE;
                    }
                }
                else
                {
                    MMCSD_log("%s>", "UNKNOWN");
                    gFsShellAppUtilsCurState = FS_SHELL_APP_UTILS_STATE_MAX;
                    spinProcess = TRUE;
                    retStat = CSL_ESYS_FAIL;
                }
                break;

            /* ---- STATE: EXECUTE_COMMAND (parse & jalankan command) ---- */
            case FS_SHELL_APP_UTILS_STATE_EXECUTE_COMMAND:
                if(FR_OK == f_opendir(&gFsShellAppUtilsDirObj, gFsShellAppUtilsCwd))
                {
                    FSShellAppUtilsCmdExecute(gFsShellAppUtilsRxBuf, gFsShellAppUtilsCmdTable);
                    gFsShellAppUtilsCurState = FS_SHELL_APP_UTILS_STATE_HELP;
                    spinProcess = TRUE;
                    retStat = CSL_SOK;
                }
                else
                {
                    MMCSD_log("%s>", "UNKNOWN");
                    gFsShellAppUtilsCurState = FS_SHELL_APP_UTILS_STATE_MAX;
                    spinProcess = TRUE;
                    retStat = CSL_ESYS_FAIL;
                }
                break;

            /* ---- STATE: MAX (idle/reset) ---- */
            case FS_SHELL_APP_UTILS_STATE_MAX:
            default:
                if(fatfsShellProcessFlag != 0)
                {
                    strcpy(gFsShellAppUtilsCwd, "0:");
                    if(FR_OK == f_opendir(&gFsShellAppUtilsDirObj, gFsShellAppUtilsCwd))
                    {
                        gFsShellAppUtilsCurState = FS_SHELL_APP_UTILS_STATE_HELP;
                        spinProcess = TRUE;
                        retStat = CSL_SOK;
                    }
                    else
                    {
                        spinProcess = TRUE;
                        gFsShellAppUtilsCurState = FS_SHELL_APP_UTILS_STATE_MAX;
                        retStat = CSL_ESYS_FAIL;
                    }
                }
                break;
        }
    } while(TRUE == spinProcess);

    return retStat;
}

/* ============================================================================
 * FUNCTION: FSShellAppUtilsCmdExecute
 * ============================================================================
 * Deskripsi: Command parser dan dispatcher.
 *
 * Cara kerja:
 *   1. Split command line menjadi argc/argv (pisah berdasarkan spasi)
 *   2. Cari argv[0] di command table
 *   3. Jika ketemu, panggil fungsi handler-nya
 *
 * Contoh input: "ls /mydir"
 *   -> argv[0]="ls", argv[1]="/mydir", argc=2
 *   -> Cari "ls" di table -> panggil FSShellAppUtilsCmdLs(2, argv)
 * ============================================================================ */
int32_t FSShellAppUtilsCmdExecute(uint8_t *pCmdLine, fsShellAppUtilsCmdEntry_t *pCmdList)
{
    int32_t retStatus = CSL_ESYS_FAIL;
    static uint8_t *argv[FSSHELLAPPUTILS_CMDLINE_MAX_ARGS + 1U];
    uint8_t *pChar;
    int32_t argc;
    uint32_t findArg = TRUE;

    argc = 0U;
    pChar = pCmdLine;

    /* Tokenize: ganti spasi dengan null terminator, simpan pointer ke argv */
    while(*pChar)
    {
        if(*pChar == ' ')
        {
            *pChar = 0;
            findArg = TRUE;
        }
        else
        {
            if(TRUE == findArg)
            {
                if(argc < FSSHELLAPPUTILS_CMDLINE_MAX_ARGS)
                {
                    argv[argc] = pChar;
                    argc++;
                    findArg = FALSE;
                    retStatus = CSL_SOK;
                }
                else
                {
                    MMCSD_log("Too many arguments for command processor!\n");
                    retStatus = CSL_ESYS_FAIL;
                    break;
                }
            }
        }
        pChar++;
    }

    /* Eksekusi command jika ditemukan di table */
    if((CSL_SOK == retStatus) && (0U != argc))
    {
        while(pCmdList->pCmd)
        {
            if(!strcmp((const char *)argv[0], (const char *)pCmdList->pCmd))
            {
                retStatus = pCmdList->pfnCmd(argc, (char **)argv);
            }
            pCmdList++;
        }
    }
    return retStatus;
}

/* ============================================================================
 * FUNCTION: FSShellAppUtilsFrmtPath
 * ============================================================================
 * Deskripsi: Formatter path — menggabungkan CWD dengan path input untuk
 *            menghasilkan absolute path lengkap.
 *
 * Kasus:
 *   - Input sudah ada drive (misal "1:/dir") -> pakai langsung
 *   - Input dimulai "/" (absolute) -> gabung drive dari CWD + path
 *   - Input relatif (misal "subdir") -> gabung CWD + "/" + input
 *
 * Return: CSL_SOK jika berhasil, CSL_ESYS_FAIL jika gagal
 * ============================================================================ */
static int32_t FSShellAppUtilsFrmtPath(char* inputPath, char* outputPath)
{
    int32_t retStat = CSL_ESYS_FAIL;
    uint32_t drvLen = 0U;

    strcpy(outputPath, "");
    drvLen = strcspn(inputPath, ":");

    /* Input sudah punya drive letter (misal "1:/dir") */
    if (drvLen < strlen(inputPath))
    {
        strcpy(outputPath, inputPath);
        retStat = CSL_SOK;
    }
    else
    {
        drvLen = strcspn(gFsShellAppUtilsCwd, ":");

        if (drvLen < strlen(gFsShellAppUtilsCwd))
        {
            /* Input absolute path (dimulai /) */
            if('/' == *inputPath)
            {
                if((strlen(inputPath) + drvLen + 1U + 1U) <= sizeof(gFsShellAppUtilsCwd))
                {
                    if(0U != strlen(gFsShellAppUtilsCwd))
                    {
                        strncpy(outputPath, gFsShellAppUtilsCwd, drvLen + 1U);
                        outputPath[drvLen + 1U] = '\0';
                        if(strlen(inputPath) > 1U)
                        {
                            strcat(outputPath, inputPath);
                        }
                        retStat = CSL_SOK;
                    }
                }
            }
            /* Input relative path */
            else
            {
                if((strlen(gFsShellAppUtilsCwd) + strlen(inputPath) + 1U + 1U) <= sizeof(gFsShellAppUtilsCwd))
                {
                    strcat(outputPath, gFsShellAppUtilsCwd);
                    strcat(outputPath, "/");
                    strcat(outputPath, inputPath);
                    retStat = CSL_SOK;
                }
            }
        }
    }
    return retStat;
}

/* ============================================================================
 * COMMAND: ls
 * ============================================================================
 * Deskripsi: List semua file dan direktori di current directory.
 * Output: Attribute, tanggal, waktu, ukuran, nama file
 *         Total file, total bytes, total direktori, space tersisa
 * ============================================================================ */
int32_t FSShellAppUtilsCmdLs(int32_t argc, char *argv[])
{
    uint32_t totalSize = 0U;
    uint32_t fileCount = 0U;
    uint32_t dirCount = 0U;
    FRESULT fresult;
    FATFS *pFatFs;

    fresult = f_opendir(&gFsShellAppUtilsDirObj, gFsShellAppUtilsCwd);

    while(FR_OK == fresult)
    {
        fresult = f_readdir(&gFsShellAppUtilsDirObj, &gFsShellAppUtilsFileInfo);

        if(FR_OK == fresult)
        {
            if('\0' == gFsShellAppUtilsFileInfo.fname[0])
                break;

            if(AM_DIR == (gFsShellAppUtilsFileInfo.fattrib & AM_DIR))
                dirCount++;
            else
            {
                fileCount++;
                totalSize += gFsShellAppUtilsFileInfo.fsize;
            }

            MMCSD_log("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s\n",
                (gFsShellAppUtilsFileInfo.fattrib & AM_DIR) ? 'D' : '-',
                (gFsShellAppUtilsFileInfo.fattrib & AM_RDO) ? 'R' : '-',
                (gFsShellAppUtilsFileInfo.fattrib & AM_HID) ? 'H' : '-',
                (gFsShellAppUtilsFileInfo.fattrib & AM_SYS) ? 'S' : '-',
                (gFsShellAppUtilsFileInfo.fattrib & AM_ARC) ? 'A' : '-',
                (gFsShellAppUtilsFileInfo.fdate >> 9) + 1980,
                (gFsShellAppUtilsFileInfo.fdate >> 5) & 15,
                gFsShellAppUtilsFileInfo.fdate & 31,
                (gFsShellAppUtilsFileInfo.ftime >> 11),
                (gFsShellAppUtilsFileInfo.ftime >> 5) & 63,
                gFsShellAppUtilsFileInfo.fsize,
                gFsShellAppUtilsFileInfo.fname);
        }
    }

    if(FR_OK == fresult)
    {
        MMCSD_log("\n%4u File(s),%10u bytes total\n%4u Dir(s)", fileCount, totalSize, dirCount);
        fresult = f_getfree("/", (DWORD *)&totalSize, &pFatFs);
        if(FR_OK == fresult)
        {
            MMCSD_log(", %10uK bytes free\n", totalSize * pFatFs->csize / 2);
        }
    }
    return fresult;
}

/* ============================================================================
 * COMMAND: rm <path>
 * ============================================================================
 * Deskripsi: Hapus file atau direktori kosong.
 * Argumen: argv[1] = path file/direktori yang dihapus
 * ============================================================================ */
int32_t FSShellAppUtilsCmdRm(int32_t argc, char *argv[])
{
    int32_t retStat = CSL_ESYS_FAIL;
    FRESULT fresult;
    retStat = FSShellAppUtilsFrmtPath(argv[1U], gFsShellAppUtilsTempPath);
    if (CSL_SOK == retStat)
    {
        fresult = f_unlink(gFsShellAppUtilsTempPath);
        if(fresult != FR_OK)
            retStat = CSL_ESYS_FAIL;
    }
    return retStat;
}

/* ============================================================================
 * COMMAND: mkdir <path>
 * ============================================================================
 * Deskripsi: Buat direktori baru.
 * Argumen: argv[1] = path direktori yang dibuat
 * ============================================================================ */
int32_t FSShellAppUtilsCmdMkDir(int32_t argc, char *argv[])
{
    int32_t retStat = CSL_ESYS_FAIL;
    FRESULT fresult;
    retStat = FSShellAppUtilsFrmtPath(argv[1U], gFsShellAppUtilsTempPath);
    if (CSL_SOK == retStat)
    {
        fresult = f_mkdir(gFsShellAppUtilsTempPath);
        if(FR_OK != fresult)
        {
            MMCSD_log("mkdir: %s\n", gFsShellAppUtilsTempPath);
            retStat = CSL_ESYS_FAIL;
        }
    }
    return retStat;
}

/* ============================================================================
 * COMMAND: cd <path>
 * ============================================================================
 * Deskripsi: Ubah current working directory.
 * Argumen: argv[1] = path direktori tujuan
 * ============================================================================ */
int32_t FSShellAppUtilsCmdCd(int32_t argc, char *argv[])
{
    int32_t retStat = CSL_ESYS_FAIL;
    FRESULT fresult;
    retStat = FSShellAppUtilsFrmtPath(argv[1U], gFsShellAppUtilsTempPath);
    if (CSL_SOK == retStat)
    {
        fresult = f_opendir(&gFsShellAppUtilsDirObj, gFsShellAppUtilsTempPath);
        if(FR_OK != fresult)
        {
            MMCSD_log("cd: %s\n", gFsShellAppUtilsTempPath);
            retStat = CSL_ESYS_FAIL;
        }
        else
        {
            strncpy(gFsShellAppUtilsCwd, gFsShellAppUtilsTempPath, sizeof(gFsShellAppUtilsCwd));
        }
    }
    return retStat;
}

/* ============================================================================
 * COMMAND: pwd
 * ============================================================================
 * Deskripsi: Tampilkan current working directory saat ini.
 * ============================================================================ */
int32_t FSShellAppUtilsCmdPwd(int32_t argc, char *argv[])
{
    MMCSD_log("%s\n", gFsShellAppUtilsCwd);
    return CSL_SOK;
}

/* ============================================================================
 * COMMAND: cat <src> [> <dst>]
 * ============================================================================
 * Deskripsi: Baca isi file dan tampilkan ke console.
 *            Jika ada argumen "> <dst>", maka copy file ke tujuan.
 *
 * Mode 1 (read):  cat myfile.txt      -> tampil isi file ke UART
 * Mode 2 (copy):  cat src.txt > dst.txt -> copy src.txt ke dst.txt
 * ============================================================================ */
int32_t FSShellAppUtilsCmdCat(int32_t argc, char *argv[])
{
    FRESULT fresultRead = FR_NOT_READY;
    FRESULT fresultWrite = FR_NOT_READY;
    uint32_t bytesWrite = 0;
    uint32_t flagWrite = FALSE;
    uint32_t usBytesRead = 0;
    uint32_t flagRead = FALSE;
    int32_t retStat = CSL_ESYS_FAIL;

    strcpy(gFsShellAppUtilsTempPath, "");
    retStat = FSShellAppUtilsFrmtPath(argv[1U], gFsShellAppUtilsTempPath);

    /* Buka file sumber untuk dibaca */
    if (CSL_SOK == retStat)
    {
        fresultRead = f_open(&gFsShellAppUtilsReadFileObj, gFsShellAppUtilsTempPath, FA_READ);
        if(fresultRead != FR_OK)
        {
            MMCSD_log("Fail to open file for read !!!!\n");
            retStat = CSL_ESYS_FAIL;
        }
        else
        {
            flagRead = TRUE;
        }
    }

    /* Cek apakah ada redirect (>) untuk copy file */
    if (CSL_SOK == retStat)
    {
        if(argc >= 4)
        {
            if(0U == strcmp(argv[2], ">"))
            {
                strcpy(gFsShellAppUtilsTempPath, "");
                retStat = FSShellAppUtilsFrmtPath(argv[3U], gFsShellAppUtilsTempPath);
                if (CSL_SOK == retStat)
                {
                    fresultWrite = f_open(&gFsShellAppUtilsWriteFileObj, gFsShellAppUtilsTempPath, FA_WRITE|FA_OPEN_ALWAYS);
                    if(fresultWrite != FR_OK)
                    {
                        MMCSD_log("Fail to open file for write !!!!\n");
                        retStat = CSL_ESYS_FAIL;
                    }
                    else
                    {
                        flagWrite = TRUE;
                    }
                }
            }
        }
    }

    /* Baca file dan tampilkan/copy */
    if (CSL_SOK == retStat)
    {
        do
        {
            fresultRead = f_read(&gFsShellAppUtilsReadFileObj, gFsShellAppUtilsDataBuf, sizeof(gFsShellAppUtilsDataBuf) - 1, &usBytesRead);
            if(fresultRead != FR_OK)
            {
                MMCSD_log("Fail to read from file !!!!\n");
                retStat = CSL_ESYS_FAIL;
            }
            if(TRUE == flagWrite)
            {
                fresultWrite = f_write(&gFsShellAppUtilsWriteFileObj, gFsShellAppUtilsDataBuf, usBytesRead, &bytesWrite);
                if(fresultWrite != FR_OK)
                {
                    MMCSD_log("Fail to write into file !!!!\n");
                    retStat = CSL_ESYS_FAIL;
                }
            }
            else
            {
                gFsShellAppUtilsDataBuf[usBytesRead] = 0;
                MMCSD_log("%s", gFsShellAppUtilsDataBuf);
            }
        } while(usBytesRead == (sizeof(gFsShellAppUtilsDataBuf) - 1));
    }

    /* Tutup file baca */
    if(TRUE == flagRead)
    {
        fresultRead = f_close(&gFsShellAppUtilsReadFileObj);
        if(fresultRead != FR_OK)
        {
            MMCSD_log("Fail to close read file !!!!\n");
            retStat = CSL_ESYS_FAIL;
        }
    }
    /* Tutup file tulis */
    if(TRUE == flagWrite)
    {
        fresultWrite = f_close(&gFsShellAppUtilsWriteFileObj);
        if(fresultWrite != FR_OK)
        {
            MMCSD_log("Fail to close write file !!!!\n");
            retStat = CSL_ESYS_FAIL;
        }
    }
    return retStat;
}

/* ============================================================================
 * COMMAND: help / h / ?
 * ============================================================================
 * Deskripsi: Tampilkan daftar semua command yang didukung beserta info singkat.
 * ============================================================================ */
int32_t FSShellAppUtilsCmdHelp(int32_t argc, char *argv[])
{
    fsShellAppUtilsCmdEntry_t *pEntry;
    MMCSD_log("\nAvailable commands\n");
    MMCSD_log("------------------\n");
    pEntry = &gFsShellAppUtilsCmdTable[0];
    while(pEntry->pCmd)
    {
        MMCSD_log("%s%s\n", pEntry->pCmd, pEntry->pHelp);
        pEntry++;
    }
    return(0);
}
