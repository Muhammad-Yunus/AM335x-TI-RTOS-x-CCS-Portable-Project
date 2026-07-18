# TIRTOS AM3352 GPIO LED Sequence

SYSBIOS (TI-RTOS) project for **AM3352** (Antminer L3+ Control Board), similar to BeagleBone Black (AM3358) — 4-LED sweeping sequence using RTOS Task.

## Architecture

- **Task `led_sequence_task`** — sweeps 4 onboard LEDs (D2–D5) one by one, ping-pong pattern (D2→D3→D4→D5→D4→D3→D2→...), 500ms per step via `Task_sleep()`
- **LED mapping** — all 4 LEDs on GPIO1 port (pins 21–24):

  | LED | GPIO Pin | Register |
  |-----|----------|----------|
  | D2  | GPIO1[21] | GPMC_A5 |
  | D3  | GPIO1[22] | GPMC_A6 |
  | D4  | GPIO1[23] | GPMC_A7 |
  | D5  | GPIO1[24] | GPMC_A8 |

- **`Board_initGPIO()`** — initializes pinmux & GPIO clock via Board driver
- **`GPIO_init()`** — initializes TI-RTOS GPIO driver (4 outputs)
- **SYSBIOS** — real-time kernel (`BIOS_start()`) with Clock, Hwi, Task, Cache & MMU configuration for AM335x

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point & LED sequence task |
| `GPIO_board.h` | GPIO LED definitions |
| `GPIO_bbbAM335x_board.c` | Pin config (4 output pins) |
| `GPIO_soc.c` | GPIO hardware attributes (base address, IRQ) |
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
3. Deploy & run — LEDs sweep in sequence
