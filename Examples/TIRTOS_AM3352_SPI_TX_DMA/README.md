# TIRTOS AM3352 SPI TX DMA

SYSBIOS (TI-RTOS) project for **AM3352** — SPI Master continuous loopback test with **DMA transfer** (EDMA3).

## Architecture

- **Task `masterTaskFxn`** — main task, continuously transmits `0xAF 0xAF` via SPI using DMA and receives loopback data
- **SPI Callback** — non-blocking transfer with semaphore synchronization
- **EDMA3** — hardware DMA engine for efficient data transfer without CPU intervention
- **Cache Management** — Write Back / Invalidate for ARM Cortex-A8 cache coherence
- **`Board_init()`** — initializes pinmux, module clock & UART stdio via Board driver
- **SYSBIOS** — real-time kernel (`BIOS_start()`) with Task scheduler

## Pin Mapping (Beaglebone Black P9 Header)

| Function | P9 Pin | Package Pin | SPI Signal |
|----------|--------|-------------|------------|
| SCLK | P9-31 | A13 | SPI1_SCLK |
| MOSI | P9-29 | B13 | SPI1_D0 |
| MISO | P9-30 | D12 | SPI1_D1 |
| CS0 | P9-28 | C12 | SPI1_CS0 |

### Loopback Connection

Connect **P9-29 (MOSI)** to **P9-30 (MISO)** with a jumper wire.

```
P9-29 (MOSI/D0) ─────�� P9-30 (MISO/D1)
```

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point, SPI task & DMA transfer logic |
| `SPI_log.c` | UART logging helper |
| `spi_arm_dma_bbbAM335x_slavemode.cfg` | SYSBIOS configuration |
| `TIRTOS_AM3352_SPI_TX_DMA.cmd` | Linker script |

## Toolchain

- **Processor SDK RTOS AM335x** — [v1.0.17](https://www.ti.com/tool/download/PROCESSOR-SDK-RTOS-AM335X)
- **Code Composer Studio** — v12.8.1
- **Compiler** — GCC ARM 7.3.1 (`gcc-arm-none-eabi-7-2018-q2-update`)
- **XDCtools** — v3.55.2.22_core
- **SYSBIOS** — v6.76.3.01

## Build & Run

1. Build via CCS — Configuration **Debug**
2. Debug using **JLink** with target configuration:
   - Device: `AM3352` / `AM3359` (Cortex-A8)
   - CCXML: use target configuration with JLink
   - GEL file: `C:\ti\ccs1281\ccs\ccs_base\emulation\boards\beaglebone\gel\beagleboneblack.gel`
3. Open serial terminal (e.g. PuTTY, TeraTerm) — 115200 baud
4. Observe output:
   ```
    ================================ 
      SPI DMA Loopback Test v1.0     
    ================================ 
   EDMA driver initialization PASS.
   SPI initialized
   TX: 0xAF 0xAF
   RX: 0xAF 0xAF 
   RX: 0xAF 0xAF 
   RX: 0xAF 0.AF 
   ...
   ```

## Comparison with TIRTOS_AM3352_SPI_TX_V2

| | **SPI_TX_V2 (poling/callback tanpa DMA)** | **SPI_TX_DMA (baru dengan EDMA3)** |
|--|--|-|
| **Transfer Mode** | Callback-based, CPU-driven | DMA-driven (EDMA3), minimal CPU overhead |
| **EDMA3** | Tidak digunakan | Digunakan untuk TX/RX otomatis |
| **Cache Management** | Tidak ada | CacheP_wb / CacheP_Inv untuk data integrity |
| **Semaphore Sync** | Ada | Ada (sama) |
| **Performance** | Lebih banyak CPU usage | Lebih efisien, CPU bisa lakukan tugas lain |
| **Complexity** | Simpel | Lebih kompleks (EDMA init, cache ops) |

### Kesimpulan

Project **SPI_TX_DMA** menambahkan **EDMA3 hardware DMA** untuk transfer SPI, sehingga CPU tidak perlu mengurusi setiap byte yang ditransfer. Ini lebih efisien untuk transfer data besar dan memungkinkan CPU melakukan tugas lain selama DMA bekerja di background.
