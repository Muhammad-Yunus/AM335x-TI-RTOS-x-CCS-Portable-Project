/**
 * =============================================================================
 * FILE: UART_soc.c
 * =============================================================================
 * Deskripsi: SOC-specific UART configuration untuk AM335x.
 *             File ini mendefinisikan konfigurasi hardware untuk 6 UART peripheral
 *             pada AM335x, termasuk register base, interrupt, EDMA channels,
 *             dan trigger levels.
 *
 * UART pada AM335x:
 *   - 6 UART peripheral (UART0-UART5)
 *   - UART0 digunakan untuk console stdio (printf, scanf)
 *   - Semua UART mendukung DMA (EDMA) untuk TX dan RX
 *
 * UART0 (Console):
 *   - Baud rate: 115200 (default Board_init)
 *   - Interrupt: 72
 *   - EDMA RX: channel 27, EDMA TX: channel 26
 *   - RX Trigger Level: 8 byte
 *   - TX Trigger Level: 56 byte
 * =============================================================================
 */

#include <ti/csl/csl_utils.h>
#include <ti/drv/uart/UART.h>
#include <ti/starterware/include/types.h>
#include <ti/starterware/include/hw/soc_am335x.h>
#include <ti/drv/uart/soc/UART_soc.h>

/* Jumlah UART peripheral pada AM335x = 6 */
#define CSL_UART_PER_CNT  (6U)

/* ============================================================================
 * EDMA CHANNEL MAPPING
 * ============================================================================
 * EDMA (Enhanced Direct Memory Access) digunakan untuk transfer data UART
 * tanpa melibatkan CPU. Setiap UART memiliki 2 channel EDMA:
 *   - RX: menerima data dari UART ke memory
 *   - TX: mengirim data dari memory ke UART
 * ============================================================================ */

#define CSL_EDMA3_CHA_UART0_RX    (27U)   /* EDMA channel untuk UART0 RX */
#define CSL_EDMA3_CHA_UART0_TX    (26U)   /* EDMA channel untuk UART0 TX */
#define CSL_EDMA3_CHA_UART1_RX    (29U)   /* EDMA channel untuk UART1 RX */
#define CSL_EDMA3_CHA_UART1_TX    (28U)   /* EDMA channel untuk UART1 TX */
#define CSL_EDMA3_CHA_UART2_RX    (31U)   /* EDMA channel untuk UART2 RX */
#define CSL_EDMA3_CHA_UART2_TX    (30U)   /* EDMA channel untuk UART2 TX */
#define CSL_EDMA3_CHA_UART3_RX    (8U)    /* EDMA channel untuk UART3 RX */
#define CSL_EDMA3_CHA_UART3_TX    (7U)    /* EDMA channel untuk UART3 TX */
#define CSL_EDMA3_CHA_UART4_RX    (10U)   /* EDMA channel untuk UART4 RX */
#define CSL_EDMA3_CHA_UART4_TX    (9U)    /* EDMA channel untuk UART4 TX */
#define CSL_EDMA3_CHA_UART5_RX    (12U)   /* EDMA channel untuk UART5 RX */
#define CSL_EDMA3_CHA_UART5_TX    (11U)   /* EDMA channel untuk UART5 TX */

/* ============================================================================
 * UART HARDWARE ATTRIBUTES
 * ============================================================================
 * Format per entry:
 *   { regBase, irqNum, rxTrigger, clkFreq, edmaRxCh, edmaTxCh,
 *     rxFifoThresh, txFifoThresh, enableInt, enableRxDma, enableTxDma,
 *     flowCtrl, rxTrigLvl, txTrigLvl, isClkSrcExt, isIrqShared,
 *     operMode }
 * ============================================================================ */

UART_HwAttrs uartInitCfg[CSL_UART_PER_CNT] = {
    /* UART0 - Console (non-16x mode, default baud) */
    { SOC_UART_0_REGS, 72, 0, 48000000U, CSL_EDMA3_CHA_UART0_RX, CSL_EDMA3_CHA_UART0_TX, 0, 0, 0, 0, 0, NULL, UART_RXTRIGLVL_8, UART_TXTRIGLVL_56, TRUE, FALSE, TRUE },
    /* UART1-UART5 - 16x clock mode */
    { SOC_UART_1_REGS, 73, 0, 48000000U, CSL_EDMA3_CHA_UART1_RX, CSL_EDMA3_CHA_UART1_TX, 0, 0, 0, 0, 0, NULL, UART_RXTRIGLVL_8, UART_TXTRIGLVL_56, TRUE, FALSE, TRUE, UART16x_OPER_MODE },
    { SOC_UART_2_REGS, 74, 69, 48000000U, CSL_EDMA3_CHA_UART2_RX, CSL_EDMA3_CHA_UART2_TX, 0, 0, 0, 0, 0, NULL, UART_RXTRIGLVL_8, UART_TXTRIGLVL_56, TRUE, FALSE, TRUE, UART16x_OPER_MODE },
    { SOC_UART_3_REGS, 44, 0, 48000000U, CSL_EDMA3_CHA_UART3_RX, CSL_EDMA3_CHA_UART3_TX, 0, 0, 0, 0, 0, NULL, UART_RXTRIGLVL_8, UART_TXTRIGLVL_56, TRUE, FALSE, TRUE, UART16x_OPER_MODE },
    { SOC_UART_4_REGS, 45, 0, 48000000U, CSL_EDMA3_CHA_UART4_RX, CSL_EDMA3_CHA_UART4_TX, 0, 0, 0, 0, 0, NULL, UART_RXTRIGLVL_8, UART_TXTRIGLVL_56, TRUE, FALSE, TRUE, UART16x_OPER_MODE },
    { SOC_UART_5_REGS, 46, 0, 48000000U, CSL_EDMA3_CHA_UART5_RX, CSL_EDMA3_CHA_UART5_TX, 0, 0, 0, 0, 0, NULL, UART_RXTRIGLVL_8, UART_TXTRIGLVL_56, TRUE, FALSE, TRUE, UART16x_OPER_MODE },
};

/* UART Objects: satu per UART peripheral (untuk driver internal state) */
UART_V1_Object UartObjects[CSL_UART_PER_CNT];

/**
 * UART_config: Tabel konfigurasi untuk semua UART.
 * Menghubungkan function table, object, dan hardware attrs.
 */
UART_config_list UART_config = {
    { &UART_FxnTable_v1, &UartObjects[0], &uartInitCfg[0] },
    { &UART_FxnTable_v1, &UartObjects[1], &uartInitCfg[1] },
    { &UART_FxnTable_v1, &UartObjects[2], &uartInitCfg[2] },
    { &UART_FxnTable_v1, &UartObjects[3], &uartInitCfg[3] },
    { &UART_FxnTable_v1, &UartObjects[4], &uartInitCfg[4] },
    { &UART_FxnTable_v1, &UartObjects[5], &uartInitCfg[5] },
    {NULL, NULL, NULL}, {NULL, NULL, NULL}, {NULL, NULL, NULL}, {NULL, NULL, NULL},
    {NULL, NULL, NULL}, {NULL, NULL, NULL}, {NULL, NULL, NULL}, {NULL, NULL, NULL}
};

/* ============================================================================
 * SOC-SPECIFIC FUNCTIONS
 * ============================================================================ */

/**
 * UART_socGetInitCfg()
 *
 * Mengambil konfigurasi hardware untuk UART tertentu.
 * Dipanggil oleh UART driver saat inisialisasi.
 *
 * \param index  Index UART (0-5)
 * \param cfg    Pointer ke struktur hardware attrs yang diisi
 * \return 0 jika berhasil, -1 jika index invalid
 */
int32_t UART_socGetInitCfg(uint32_t index, UART_HwAttrs *cfg)
{
    int32_t ret = 0;
    if (index < CSL_UART_PER_CNT)
        *cfg = uartInitCfg[index];
    else
        ret = -1;
    return ret;
}

/**
 * UART_socSetInitCfg()
 *
 * Mengubah konfigurasi hardware untuk UART tertentu.
 * Dipanggil untuk override default configuration.
 *
 * \param index  Index UART (0-5)
 * \param cfg    Pointer ke struktur hardware attrs baru
 * \return 0 jika berhasil, -1 jika index invalid
 */
int32_t UART_socSetInitCfg(uint32_t index, const UART_HwAttrs *cfg)
{
    int32_t ret = 0;
    if (index < CSL_UART_PER_CNT)
        uartInitCfg[index] = *cfg;
    else
        ret = -1;
    return ret;
}
