# TIRTOS AM3352 ILI9341 LCD Demo

SYS/BIOS (TI-RTOS) project for **AM3352** (BeagleBone Black) — drives a 2.8" TFT LCD with ILI9341 controller over SPI0, showing colour bands, shapes, text, and pixel-art patterns.

## Hardware Setup

| Item | Value |
|------|-------|
| SoC | AM3352 / AM3359 (Cortex-A8) |
| SPI | SPI0 — 10 MHz, mode 0 (POL0/PHA0) |
| SCLK | P9_22 |
| MOSI | P9_18 (SPI0_D1 — 4-pin mode) |
| CS   | P9_17 (hardware-controlled, SPI0_CS0) |
| DC   | P8_26 (GPIO1_29) |
| RST  | P8_19 (GPIO0_22) |
| UART | UART0 @ 115200 8N1 (P9_11 TX, P9_13 RX) |

> CS is handled by the MCSPI peripheral in 4-pin mode. Each `SPI_transfer()` call asserts CS at the start and deasserts at the end, which matches the ILI9341 protocol requirements (CS toggling between command and data phases).

## Architecture

```
main()
  |-- Board_initGPIO()        — board pinmux + module clocks + UART stdio
  |-- Gpio0ClockEnable()      — enable GPIO0 module clock (CM_WKUP)
  |-- SpiPinMuxSetup()        — configure all 5 pads (DC, RST, SCLK, MOSI, CS)
  |-- GPIO_init()             — TI-RTOS GPIO driver init
  |-- Spi0ClockEnable()       — enable SPI0 module clock (CM_PER)
  |-- SpiInit()               — open SPI0 @ 10 MHz, mode 0, 4-pin HW-CS
  |-- Task_create(lcd_demo_task)
  |-- BIOS_start()
        |
        +-- lcd_demo_task     — ILI9341_Init(), then loop: 4 demo scenes
```

- **`Gpio0ClockEnable()`** — direct register write via StarterWare CSL
- **`SpiPinMuxSetup()`** — merged pinmux for all SPI + GPIO pads
- **`SpiInit()`** — sets `dataLineCommMode = MCSPI_DATA_LINE_COMM_MODE_1` (4-pin), then `SPI_open` with `SPI_MASTER`, 8-bit, `SPI_POL0_PHA0`, blocking mode
- **`lcd_demo_task`** — calls `ILI9341_Init()` for full boot sequence, then cycles through 4 demo scenes

### Low-Level Hooks

`ili9341.c` calls into `main.c` through these extern functions:

| Function | Role |
|----------|------|
| `Spi0TxByte(b)` | Send 1 byte via PDK `SPI_transfer()` |
| `Spi0TxBuffer(buf, len)` | Send multiple bytes via `SPI_transfer()` (chunked) |
| `LcdDcLow()` / `LcdDcHigh()` | Toggle DC GPIO (P8_26) |
| `LcdRstLow()` / `LcdRstHigh()` | Toggle RST GPIO (P8_19) |
| `delay(ms)` | RTOS-aware delay using `Task_sleep()` |

## Demo Scenes

Cycled in an infinite loop:

1. **Color Bands** — 8 horizontal bands (red, orange, yellow, green, cyan, blue, magenta, white) with header
2. **Shapes** — hollow/filled rectangles, circles, and cross-diagonal lines
3. **Text** — 5x7 ASCII font: uppercase, lowercase, digits, punctuation against blue background
4. **Pixel Grid** — checkerboard (left half) + solid green panel (right half) with caption

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point, init sequence, `lcd_demo_task`, SPI/GPIO hooks |
| `SPI_board.h` | SPI constants (`SPI_INSTANCE`, `SPI_BIT_RATE`, `SPI_LCD_CHUNK`) and function declarations |
| `SPI_bbbAM335x_board.c` | `SpiPinMuxSetup()`, `Spi0ClockEnable()`, `SpiInit()` |
| `GPIO_board.h` | GPIO pin enum (`LCD_DC`, `LCD_RST`), level aliases |
| `GPIO_bbbAM335x_board.c` | `Gpio0ClockEnable()` implementation, `GPIO_v1_Config` table |
| `GPIO_soc.c` | GPIO SoC hardware attributes (4 ports, from PDK) |
| `UART_soc.c` | UART hardware attributes (6 instances, from PDK) |
| `ili9341.h` | ILI9341 command set, colour macros, drawing API |
| `ili9341.c` | ILI9341 driver: init sequence, pixel/fill/line/circle/text primitives |
| `fonts.h` | 5x7 ASCII bitmap font (95 chars, column-major format) |
| `delay.h` | Millisecond delay declaration (implemented via `Task_sleep` in `main.c`) |
| `am335x_app_bbbam335x.cfg` | SYS/BIOS configuration with UART + SPI + GPIO drivers and MMU sections |

## Important Notes

- **C89 compliance**: TI CGT ARM defaults to C89; all variable declarations are at the top of blocks.
- **GPIO driver uses 1-based port numbers**: The TI GPIO v1 driver checks `portNum > 0U` and indexes `hwAttrs[portNum - 1]`. GPIO0 → port 1, GPIO1 → port 2.
- **SOC_AM335x define**: The SPI SoC headers guard behind `SOC_AM335x` (lowercase x), but the project defines `SOC_AM335X` (uppercase). `SPI_bbbAM335x_board.c` adds `#define SOC_AM335x` before including the SPI driver headers.
- **SPI0 GPIO pins (SCLK, MOSI, CS)**: Pinmux value `0x20` = mode 0 (peripheral function) + RX enable (bit 5).
- **LCD GPIO pins (DC, RST)**: Pinmux value `0x07` = mode 7 (GPIO), no RX.
- **No framebuffer**: Uses row-by-row rendering (single 640-byte row buffer).
- **ILI9341 orientation**: Landscape mode (0x28 = MV=0, MX=0, MY=1, BGR=1).

## Example Output (UART0)

```
Board_init done
GPIO init done
SPI init OK

=== ILI9341 LCD DEMO ===
SPI0 @ 10 MHz, HW CS, DC=P8_26, RST=P8_19

ILI9341 init OK — starting demo cycle
```

## Toolchain

- **Processor SDK RTOS AM335x** — v06.03.00.106
- **Code Composer Studio** — v12.8.1
- **Compiler** — GCC ARM 7.3.1 (`gcc-arm-none-eabi-7-2018-q2-update`)
- **XDCtools** — v3.55.2.22_core
- **SYS/BIOS** — v6.76.3.01

## Build & Run

1. Build via CCS — Configuration **Debug**
2. Debug using **JLink** with target configuration:
   - Device: `AM3352` / `AM3359` (Cortex-A8)
   - GEL file: `C:\ti\ccs1281\ccs\ccs_base\emulation\boards\beaglebone\gel\beagleboneblack.gel`
3. Open serial terminal (e.g. PuTTY, TeraTerm) — 115200 8N1
4. Connect LCD display: DC → P8_26, RST → P8_19, SCLK → P9_22, MOSI → P9_18, CS → P9_17, ground, 3.3V
5. Launch target — LCD initialises and cycles through demo scenes
