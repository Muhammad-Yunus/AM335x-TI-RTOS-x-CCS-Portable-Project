# TIRTOS AM3352 MMCSD SD Card FATFS

SYSBIOS (TI-RTOS) project for **AM3352** (Board mirip Beaglebone Black, e.g. Antminer L3+) — SD Card file access dengan interactive shell console via UART.

## Architecture

- **Task `FATFFS_Driver`** (Priority 2) — Monitor SD Card status via GPIO interrupt (Card Detect), mount/unmount FATFS filesystem
- **Task `FShellUtilsProcess`** (Priority 1) — Interactive file shell, menerima command dari UART console
- **`Board_init()`** — Initializes pinmux, clock, and UART0 stdio
- **`GPIO_init()`** — Initializes TI-RTOS GPIO driver for card detect interrupt
- **`FATFS_init()`** — Registers MMCSD driver as FATFS disk I/O layer
- **SYSBIOS** — Real-time kernel (`BIOS_start()`) with Task, Semaphore, Clock, Hwi, Cache & MMU configuration for AM335x

## Features

- **Auto card detection** — SD Card insert/remove detected via GPIO interrupt
- **Dynamic mount/unmount** — FATFS filesystem mounted on card insert, unmounted on remove
- **Interactive file shell** — Full file operations via UART console:

| Command | Description | Example |
|---------|-------------|---------|
| `ls` | List files and directories | `ls` |
| `cd` | Change directory | `cd home` |
| `pwd` | Print working directory | `pwd` |
| `mkdir` | Create directory | `mkdir myapp` |
| `rm` | Delete file or empty directory | `rm oldfile.txt` |
| `cat` | Read file or copy file | `cat myfile.txt` \| `cat src.txt > dst.txt` |
| `help` | Show available commands | `help` |

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point, task creation, GPIO callback, FATFS mount/unmount |
| `fs_shell_app_utils.c` | Interactive file shell implementation (state machine + command handlers) |
| `fs_shell_app_utils.h` | Shell utility header (structures, enums, prototypes) |
| `GPIO_soc.c` | GPIO hardware attributes (base address, IRQ per bank) |
| `UART_soc.c` | UART hardware attributes (EDMA channels, trigger levels) |
| `MMCSD_log.h` | Log utility header (UART_printf alias) |
| `mmcsd_bbbam335x.cfg` | SYS/BIOS configuration file |

## Hardware Connection

| Component | Connection |
|-----------|------------|
| SD Card Slot | MMCSD Controller (Slot 0) |
| SD_CD (Card Detect) | GPIO pin with interrupt (both edges) |
| UART Console | UART0 (Baud: 115200, 8N1) |

## Toolchain

- **Processor SDK RTOS AM335x** — [v06.03.00.106](https://www.ti.com/tool/download/PROCESSOR-SDK-RTOS-AM335X/06.03.00.106)
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
3. Deploy & run — Open serial terminal (115200 baud)
4. Insert SD Card (FAT16/FAT32) — Shell prompt appears: `0:> `
5. Type commands to interact with SD Card files

## Shell Session Example

```
Please insert card.

Card inserted.
All tests have PASSED
0:>ls
D---- 2024/01/15 10:30         0  System Volume Information
----A 2024/01/15 10:30     1024  readme.txt

   1 File(s),      1024 bytes total
   1 Dir(s),   15140800K bytes free
0:>cd home
0:/home>ls
----A 2024/01/15 11:00      4096  data.bin

   1 File(s),      4096 bytes total
0:/home>cat data.bin
[File contents displayed...]
0:/home>mkdir backup
0:/home>ls
D---- 2024/01/15 11:05         0  backup
----A 2024/01/15 11:00      4096  data.bin
0:>pwd
0:/home
0:>
```

## Notes

- MMCSD runs in **PIO mode** (no DMA)
- UART TX/RX runs with **DMA enabled**
- Stack size: **8 KB** per task
- Heap size: **1 KB**
- SD Card must be formatted as **FAT16** or **FAT32**
- `rm` command only deletes files or **empty** directories
