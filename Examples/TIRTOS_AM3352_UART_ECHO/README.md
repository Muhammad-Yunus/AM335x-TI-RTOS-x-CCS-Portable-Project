# TIRTOS AM3352 UART Echo

SYSBIOS (TI-RTOS) project for **AM3352** (Antminer L3+ Control Board), similar to BeagleBone Black (AM3358) — UART echo demo.

## Architecture

- **Task `uart_echo_task`** — main task, reads characters via UART and echoes back with `\necho> ` prefix on each line break
- **`Board_initUART()`** — initializes pinmux, module clock & UART stdio via Board driver
- **SYSBIOS** — real-time kernel (`BIOS_start()`) with Clock, Hwi, Task, Cache & MMU configuration for AM335x

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point & UART echo task |
| `UART_soc.c` | UART hardware attributes (6 instances) |
| `am335x_app_bbbam335x.cfg` | SYSBIOS configuration |
| `AM335x.lds` | Linker script |

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
3. Open serial terminal (e.g. PuTTY, TeraTerm) — 115200 baud
4. Type any text and press Enter — you will see:
   ```
   ==== TI-RTOS - UART Echo Example ====
   hello
   echo> hello
   ```
