# TIRTOS AM3352 ILI9341 LVGL Music Demo

SYS/BIOS (TI-RTOS) project for **AM3352** (BeagleBone Black) — drives a 2.8" TFT LCD with ILI9341 controller over SPI0, running **LVGL v9.2.1** Graphics Library with the **Music Player Demo**.

## Hardware Setup

| Item | Value |
|------|-------|
| SoC | AM3352 / AM3359 (Cortex-A8) |
| SPI | SPI0 — 24 MHz, mode 0 (POL0/PHA0), TX_ONLY, polling |
| SCLK | P9_22 |
| MOSI | P9_18 (SPI0_D1 — 4-pin mode) |
| CS   | P9_17 (hardware-controlled, SPI0_CS0) |
| DC   | P8_26 (GPIO1_29) |
| RST  | P8_19 (GPIO0_22) |
| UART | UART0 @ 115200 8N1 (P9_11 TX, P9_13 RX) |

> CS is handled by the MCSPI peripheral in 4-pin mode (hardware-controlled). Each `SPI_transfer()` call asserts CS at the start and deasserts at the end. For LVGL flush bursts, `Spi0TxBufferFast()` uses direct register writes to keep CS asserted for the entire pixel data transfer.

## Architecture

```
main()
  |-- Board_initGPIO()            — board pinmux + module clocks + UART stdio
  |-- Gpio0ClockEnable()          — enable GPIO0 module clock (CM_WKUP)
  |-- SpiPinMuxSetup()            — configure all 5 pads (DC, RST, SCLK, MOSI, CS)
  |-- GPIO_init()                 — TI-RTOS GPIO driver init
  |-- Spi0ClockEnable()           — enable SPI0 module clock (CM_PER)
  |-- SpiInit()                   — open SPI0 @ 24 MHz, mode 0, 4-pin HW-CS, TX_ONLY, polling
  |-- Task_create(lvgl_demo_task)
  |-- BIOS_start()
        |
        +-- lvgl_demo_task
              |-- ILI9341_Init()
              |-- lv_init()
              |-- lv_port_disp_init()
              |-- Centered button demo (3 seconds)
              |-- lv_demo_music()  — LVGL Music Player Demo
              |-- loop: lv_tick_inc(5), lv_timer_handler(), Task_sleep(5ms)
```

## LVGL Integration

LVGL v9.2.1 is located in `Middlewares/lvgl/`. The following files integrate LVGL with the display:

| File | Description |
|------|-------------|
| `lv_conf.h` | LVGL configuration (RGB565 16-bit, 256KB heap, Music Demo enabled, auto-play) |
| `lv_port_disp.h` | Display port header — declares `lv_port_disp_init()` |
| `lv_port_disp.c` | Display port implementation — 2 full-frame buffers (320x240x2 = 307KB), flush callback using `Spi0TxBufferFast()` |

### SPI Burst Mode for LVGL

`Spi0TxBufferFast()` in `main.c` writes pixel data directly to the MCSPI TX FIFO using register-level access, keeping CS asserted for the entire burst. This is required by the ILI9341 GRAM write protocol where CS must stay low throughout the pixel stream.

## Demos

The task runs two stages sequentially:

1. **Centered Button** — Simple LVGL button with "Hello LVGL!" label centered on screen (3 seconds)
2. **Music Player Demo** — Full LVGL Music Demo with album art, track list, play controls, auto-play enabled

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point, init sequence, `lvgl_demo_task`, SPI/GPIO hooks, `Spi0TxBufferFast` |
| `SPI_board.h` | SPI constants (`SPI_INSTANCE`, `SPI_BIT_RATE`=24 MHz, `SPI_LCD_CHUNK`=65535) and function declarations |
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
| `lv_conf.h` | LVGL configuration header — color depth, heap size, demo enablers |
| `lv_port_disp.h` / `lv_port_disp.c` | LVGL display port — frame buffers, flush callback |
| `Middlewares/lvgl/` | LVGL v9.2.1 source tree — core library, widgets, demos (music) |

## Build Notes

- **Include path** `${PROJECT_ROOT}/Middlewares/lvgl` must be set in the CCS project compiler settings (already added to `.cproject`).
- All LVGL `.c` source files under `Middlewares/lvgl/src/` and `Middlewares/lvgl/demos/` must be included in the build (CCS managed make discovers them automatically from the project tree).
- LVGL uses its own built-in memory allocator (`LV_MEM_SIZE = 256KB`), separate from the SYS/BIOS heap.

## Important Notes

- **C89 compliance**: TI CGT ARM defaults to C89; all variable declarations are at the top of blocks. GCC ARM accepts C99 inline declarations.
- **GPIO driver uses 1-based port numbers**: The TI GPIO v1 driver checks `portNum > 0U` and indexes `hwAttrs[portNum - 1]`. GPIO0 -> port 1, GPIO1 -> port 2.
- **SOC_AM335x define**: The SPI SoC headers guard behind `SOC_AM335x` (lowercase x), but the project defines `SOC_AM335X` (uppercase). `SPI_bbbAM335x_board.c` adds `#define SOC_AM335x` before including the SPI driver headers.
- **SPI0 GPIO pins (SCLK, MOSI, CS)**: Pinmux value `0x20` = mode 0 (peripheral function) + RX enable (bit 5).
- **LCD GPIO pins (DC, RST)**: Pinmux value `0x07` = mode 7 (GPIO), no RX.
- **ILI9341 orientation**: Landscape mode (0x28 = MV=0, MX=0, MY=1, BGR=1).
- **LVGL color format**: `LV_COLOR_16_SWAP = 1` for RGB565 big-endian (MSB-first) to match ILI9341 pixel format.
- **LVGL requires 5ms periodic tick**: Called from `lvgl_demo_task` via `lv_tick_inc(5)` followed by `lv_timer_handler()` and `Task_sleep(5)` — ensures tick advances only when the task is active, avoiding SYS/BIOS Clock module dependency.
- **Frame buffers**: Two full-size 320x240x2 = 153,600 byte buffers in `.bss` (307 KB total). LVGL renders in `LV_DISPLAY_RENDER_MODE_FULL`.

## Example Output (UART0)

```
Board_init done
GPIO init done
SPI init OK

=== LVGL DEMO ===
SPI0 @ 24 MHz, HW CS, DC=P8_26, RST=P8_19
ILI9341_Init()...
ILI9341_Init() returned
lv_init() + lv_port_disp_init()...
LVGL initialized
Creating centered button...
Starting Music Demo...
Music Demo running
```

## Toolchain

- **Processor SDK RTOS AM335x** — v06.03.00.106
- **Code Composer Studio** — v12.8.1
- **Compiler** — GCC ARM 7.3.1 (`gcc-arm-none-eabi-7-2018-q2-update`)
- **XDCtools** — v3.55.2.22_core
- **SYS/BIOS** — v6.76.3.01

## Build & Run

1. In CCS, right-click project -> **Add Files** to include all `.c` files from `Middlewares/lvgl/src/` and `Middlewares/lvgl/demos/` into the build tree.
2. Build via CCS — Configuration **Debug**
3. Debug using **JLink** with target configuration:
   - Device: `AM3352` / `AM3359` (Cortex-A8)
   - GEL file: `C:\ti\ccs1281\ccs\ccs_base\emulation\boards\beaglebone\gel\beagleboneblack.gel`
4. Open serial terminal (e.g. PuTTY, TeraTerm) — 115200 8N1
5. Connect LCD display: DC -> P8_26, RST -> P8_19, SCLK -> P9_22, MOSI -> P9_18, CS -> P9_17, ground, 3.3V
6. Launch target — LVGL initializes, shows button, then Music Demo
