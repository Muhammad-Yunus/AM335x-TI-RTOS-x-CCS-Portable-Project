# TIRTOS AM3352 GPIO Interrupt — LED Blink + Push Button

SYSBIOS (TI-RTOS) project for **AM3352** (Antminer L3+ Control Board), similar to BeagleBone Black — LED blink with push button interrupt using GPIO.

## Behaviour

- **LED D2** (`GPIO1_23`) blinks with a configurable delay.
- **Default delay**: 1000 ms (1 second).
- **Push button** on **P9_12** (`GPIO1_28` = GPIO_60) triggers a rising-edge interrupt.
- Each button press **decreases** the blink delay by **100 ms**.
- When delay reaches **100 ms**, it resets back to **1000 ms**.

## Push Button Circuit (Active High)

```
   3V3 (HEADER P9 PIN 3 / P9 PIN 4 = VDD_ADC)
                     │
                     │
                     │
                  ┌─────┐
                  │ BTN │  ← pushbutton normally-open
                  └─────┘
                     │
                     (contact closes on press)
                     │
                     ├──────────── P9_12  (HEADER P9, pin 12)
                     │
                  ┌─────┐
                  │ 10k │  ← R_pulldown
                  └─────┘
                     │
   GND (HEADER P9 GND, e.g. P9 pin 1/2)
```

- **Button released**: P9_12 pulled LOW via 10kΩ pull-down resistor.
- **Button pressed**: +3.3V connected directly to P9_12 → reads HIGH (rising edge trigger).

## Hardware

| Signal | Pin | GPIO |
|--------|-----|------|
| LED D2 | on-board (GPIO1_23) | GPIO_55 |
| Push Button | P9_12 (GPIO1_28) | GPIO_60 |

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point, blink task, button ISR |
| `GPIO_board.h` | GPIO LED & button definitions |
| `GPIO_soc.c` | GPIO hardware attributes (base address, IRQ) |
| `GPIO_bbbAM335x_board.c` | Pin config (LED output + button input interrupt) |
| `am335x_app_bbbam335x.cfg` | SYSBIOS configuration (MMU, Hwi, clocks) |
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
3. Deploy & run — LED blinks at 1 s; press button to speed up.
