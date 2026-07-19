# TIRTOS AM3352 SPI TX — LCD Display Driver

SYS/BIOS (TI-RTOS) project for **AM3352** (BeagleBone Black) — drives an SPI LCD display with hardware CS and GPIO data/command + reset toggling.

## Hardware Setup

| Item | Value |
|------|-------|
| SoC | AM3352 / AM3359 (Cortex-A8) |
| SPI | SPI0 — 100 kHz, mode 0 (POL0/PHA0) |
| SCLK | P9_22 |
| MOSI | P9_18 |
| CS   | P9_17 (hardware-controlled, SPI0_CS0) |
| DC   | P8_26 (GPIO1_29) |
| RST  | P8_19 (GPIO0_22) |
| UART | UART0 @ 115200 8N1 (P9_11 TX, P9_13 RX) |

> CS is handled by the MCSPI peripheral in 4-pin mode. It asserts at the start of `SPI_transfer()` and stays asserted until `SPI_close()`. This is the expected behaviour for display protocols that send multi-byte command + data sequences without deasserting CS between bytes.

## Architecture

```
main()
  |-- Board_initGPIO()        — board pinmux + module clocks + UART stdio
  |-- Gpio0ClockEnable()      — enable GPIO0 module clock (CM_WKUP)
  |-- SpiPinMuxSetup()        — configure all 5 pads (DC, RST, SCLK, MOSI, CS)
  |-- GPIO_init()             — TI-RTOS GPIO driver init
  |-- Spi0ClockEnable()       — enable SPI0 module clock (CM_PER)
  |-- SpiInit()               — open SPI0, set bitrate / mode / HW-CS
  |-- Task_create(spi_tx_task)
  |-- BIOS_start()
        |
        +-- spi_tx_task       — loop: toggle DC+RST high, sleep; toggle low, sleep; SPI_transfer
```

- **`Gpio0ClockEnable()`** — direct register write via StarterWare CSL
- **`SpiPinMuxSetup()`** — merged pinmux for all SPI + GPIO pads
- **`SpiInit()`** — sets `dataLineCommMode = MCSPI_DATA_LINE_COMM_MODE_1` (4-pin), then `SPI_open` with `SPI_MASTER`, 8-bit, `SPI_POL0_PHA0`, blocking mode
- **`spi_tx_task`** — periodic toggle of DC / RST, then `SPI_transfer()` with HW CS

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point, init sequence, `spi_tx_task` loop |
| `SPI_board.h` | SPI constants (`SPI_INSTANCE`, `SPI_BIT_RATE`, `SPI_DATA_COUNT`) and function declarations |
| `SPI_bbbAM335x_board.c` | `SpiPinMuxSetup()`, `Spi0ClockEnable()`, `SpiInit()` |
| `GPIO_board.h` | GPIO pin enum (`LCD_DC`, `LCD_RST`), level aliases, `Gpio0ClockEnable()` declaration |
| `GPIO_bbbAM335x_board.c` | `Gpio0ClockEnable()` implementation, `GPIO_v1_Config` table |
| `GPIO_soc.c` | GPIO SoC hardware attributes (4 ports, from PDK) |
| `UART_soc.c` | UART hardware attributes (6 instances, from PDK) |
| `am335x_app_bbbam335x.cfg` | SYS/BIOS configuration with UART + I2C + SPI + GPIO drivers and MMU sections |

## Important Notes

- **GPIO driver uses 1-based port numbers**: The TI GPIO v1 driver checks `portNum > 0U` and indexes `hwAttrs[portNum - 1]`. GPIO0 → port 1, GPIO1 → port 2.
- **SOC_AM335x define**: The SPI SoC headers guard behind `SOC_AM335x` (lowercase x), but the project defines `SOC_AM335X` (uppercase). `SPI_bbbAM335x_board.c` adds `#define SOC_AM335x` before including the SPI driver headers.
- **SPI0 GPIO pins (SCLK, MOSI, CS)**: Pinmux value `0x20` = mode 0 (peripheral function) + RX enable (bit 5).
- **LCD GPIO pins (DC, RST)**: Pinmux value `0x07` = mode 7 (GPIO), no RX.

## Example Output (UART0)

```
Board_init done
GPIO init done
SPI init OK

=== SPI HW CS + GPIO Toggle ===
CLK=P9_22 MOSI=P9_18 CS=P9_17(HW_SPI) DC=P8_26 RST=P8_19

SPI TX=100 RX=00
SPI TX=200 RX=00
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
5. Launch target — task toggles DC/RST and sends SPI byte every 500 ms
