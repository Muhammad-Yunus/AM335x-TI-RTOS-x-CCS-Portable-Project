# TIRTOS AM3352 MMCSD SDCARD

SYSBIOS (TI-RTOS) project for **AM3352** (BeagleBone Black class) — SDMMC + SDCARD raw read/write example.

## Architecture

- **Task `mmcsd_task`** — main task, performs basic SD card operations:
  1. Open SDMMC driver
  2. Read card parameters (block size, count, total size)
  3. Write pattern to sectors
  4. Read back and verify data integrity
  5. Overwrite (update) same sectors with new pattern
  6. Read back and verify updated data
- **`Board_init()`** — initializes pinmux, module clock & UART stdio via Board driver
- **`MMCSD` driver** — TI PDK MMCSD driver with DMA enabled (ADMA2)
- **SYSBIOS** — real-time kernel (`BIOS_start()`) with Cache & MMU configuration for AM335x

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point & SDMMC task |
| `helper.c` / `helper.h` | Reusable helpers (MMCSD_CHECK, VERIFY_DATA, fill_pattern, compare_buf) |
| `MMCSD_soc.c` | MMCSD hardware attributes (3 instances) |
| `GPIO_soc.c` | GPIO hardware attributes (4 ports) |
| `UART_soc.c` | UART hardware attributes (6 instances) |
| `profiling.c` | Performance profiling & benchmarking |
| `MMCSD_log.h` | UART-based logging macros |
| `mmcsd_bbbam335x.cfg` | SYSBIOS configuration |

## Toolchain

- **Processor SDK RTOS AM335x (PDK)** — [v1.0.17](https://www.ti.com/tool/download/PDKAM335X/1.0.17)
- **Code Composer Studio** — [v12.8.1](https://www.ti.com/tool/download/CCSTUDIO/12.8.1)
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
4. Expected output:
   ```
   MMCSD_Open() completed successfully
   Getting SD Card parameters
   SD Card: BlockSize = 512,  BlockCount = 0x01cea000, CardSize = 0x39d400000 bytes

   Writing 2048 bytes to sector 0x300000 ...
   Write OK
   Reading back 2048 bytes from sector 0x300000 ...
   Read OK
   Data verified: write -> read matched

   Updating (overwriting) same sectors with new pattern ...
   Update write OK
   Reading back updated data ...
   Read after update OK
   Data verified: update -> read matched

   All SDMMC SDCARD tests PASSED
   ```

## Key APIs

| API | Purpose |
|-----|---------|
| `MMCSD_init()` | Initialize MMCSD driver |
| `MMCSD_open(instance, mode, handle)` | Open MMCSD instance |
| `MMCSD_control(handle, cmd, params)` | Control commands (GETMEDIAPARAMS) |
| `MMCSD_write(handle, buf, sector, count)` | Write data to card |
| `MMCSD_read(handle, buf, sector, count)` | Read data from card |
| `MMCSD_close(handle)` | Close MMCSD instance |

## Notes

- Test writes to sector offset `0x300000` (1.5 GB) to avoid FAT partition area
- DMA is enabled by default (ADMA2)
- All data buffers are 256-byte aligned for DMA transfer
