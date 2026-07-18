# TIRTOS AM3352 GPIO LED Blink

SYSBIOS (TI-RTOS) project for **AM3352** (Antminer L3+ Control Board), similar to BeagleBone Black (AM3358) — LED blink demo using Task.

## Architecture

- **Task `gpio_blink_task`** — main task, toggles on-board LED D2 (GPIO1\_23) every 500ms via `Task_sleep()`
- **`Board_initGPIO()`** — initializes pinmux & GPIO clock via Board driver
- **`GPIO_init()`** — initializes TI-RTOS GPIO driver
- **SYSBIOS** — real-time kernel (`BIOS_start()`) with Clock, Hwi, Task, Cache & MMU configuration for AM335x

## Files

| File | Description |
|------|-------------|
| `main_led_blink.c` | Entry point & blink task |
| `GPIO_board.h` | GPIO LED definitions |
| `GPIO_soc.c` | GPIO hardware attributes (base address, IRQ) |
| `GPIO_bbbAM335x_board.c` | Pin config (1 output pin) |
| `GPIO_soc.c` | GPIO hardware attributes |
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
3. Deploy & run — LED blinks every 500ms
