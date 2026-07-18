# TIRTOS AM3352 I2C SSD1306 OLED LCD Test

SYSBIOS (TI-RTOS) project for **AM3352** (Antminer L3+ Control Board) — drives **SSD1306 128×32 OLED LCD** on **I2C1** via **PDK I2C Driver**.

## Hardware Setup

| Item | Value |
|------|-------|
| SoC | AM3352 (Cortex-A8) |
| Bus | I2C1 — 100 kHz standard mode |
| SCL | P9_17 (`SPI0_D1`, MUXMODE=2) |
| SDA | P9_18 (`SPI0_CS0`, MUXMODE=2) |
| OLED | SSD1306 128×32, address 0x3C |
| Pull-up | **10 kΩ SCL→3.3 V + 10 kΩ SDA→3.3 V** — mandatory |
| UART | UART0 @ 115200 8N1 (P9_11 TX, P9_13 RX) |

## Architecture

- **Task `i2c_ssd1306_lcd_test`** — opens I2C1 via PDK `I2C_open(1, ...)`, init SSD1306, runs demo loop (splash → shapes → filled shapes → counter)
- **I2C transport** — PDK `I2C_transfer()` in blocking mode (polling), no interrupts
- **`Board_initI2C1()`** — enables I2C1 module clock (CM_PER) and configures pinmux via direct register access
- **SSD1306 driver** — ported from STM32 reference; pixel/font drawing identical, only the bottom-half I2C transport is replaced
- **SYSBIOS** — real-time kernel (`BIOS_start()`) with Task, Clock, Cache & MMU

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point & `i2c_ssd1306_lcd_test` task |
| `ssd1306/ssd1306.c` / `.h` | SSD1306 driver (I2C transport via PDK `I2C_transfer`) |
| `ssd1306/ssd1306_defines.h` | Panel geometry & address (128×32, 0x3C) |
| `ssd1306/fonts.c` / `.h` | Fonts 7×10, 11×18, 16×26 |
| `UART_soc.c` | UART hardware attributes (from PDK) |
| `am335x_app_bbbam335x.cfg` | SYSBIOS configuration |

## Example Output (UART)

```
+---------------------------------------------+
|  AM3352  SSD1306  128x32  OLED  LCD  Test    |
|  I2C1  SCL=P9_17  SDA=P9_18  addr=0x3C      |
+---------------------------------------------+
ssd1306_Init OK
0
1
2
...
```

## Toolchain

- **Processor SDK RTOS AM335x** — v06.03.00.106
- **Code Composer Studio** — v12.8.1
- **Compiler** — GCC ARM 7.3.1 (`gcc-arm-none-eabi-7-2018-q2-update`)
- **XDCtools** — v3.55.2.22_core
- **SYSBIOS** — v6.76.3.01

## Build & Run

1. Build via CCS — Configuration **Debug**
2. Debug using **JLink** with target configuration:
   - Device: `AM3352` / `AM3359` (Cortex-A8)
   - GEL file: `C:\ti\ccs1281\ccs\ccs_base\emulation\boards\beaglebone\gel\beagleboneblack.gel`
3. Open serial terminal (e.g. PuTTY, TeraTerm) — 115200 8N1
4. Connect **10 kΩ pull-up resistors** from P9_17 (SCL) and P9_18 (SDA) to 3.3 V
5. Connect SSD1306 OLED to I2C1 (P9_17 SCL, P9_18 SDA, VCC 3.3 V, GND)
6. Launch target — SSD1306 will show demo sequence
