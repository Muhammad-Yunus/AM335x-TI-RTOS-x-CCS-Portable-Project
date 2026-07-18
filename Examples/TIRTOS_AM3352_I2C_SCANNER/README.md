# TIRTOS AM3352 I2C1 Bus Scanner

SYSBIOS (TI-RTOS) project for **AM3352** (Antminer L3+ Control Board) — scans **I2C1** bus and prints results in `i2cdetect -y 1` format via UART0.

## Hardware Setup

| Item | Value |
|------|-------|
| SoC | AM3352 (Cortex-A8) |
| Bus | I2C1 — 100 kHz standard mode |
| SCL | P9_17 (`SPI0_D1`, MUXMODE=2) |
| SDA | P9_18 (`SPI0_CS0`, MUXMODE=2) |
| Pull-up | **10 kΩ SCL→3.3 V + 10 kΩ SDA→3.3 V** — mandatory |
| UART | UART0 @ 115200 8N1 (P9_11 TX, P9_13 RX) |

> I2C1 pins require **external 10 kΩ pull-up resistors to 3.3 V**. The internal pull-ups (~50 kΩ) are not enabled by the pinmux configuration and are too weak for reliable 100 kHz operation. Without external pull-ups, the bus rise time is too slow and slaves will not ACK correctly.

## Architecture

- **Task `i2c_scanner_task`** — probes every valid 7-bit address (0x03–0x77) on I2C1, builds a device map, and prints a 16×16 grid to UART
- **`Board_initI2C1()`** — enables I2C1 module clock (CM_PER) and configures pinmux via direct register access (StarterWare CSL)
- **`SetupI2C1Master()`** — configures I2C1 controller: prescaler, SCL timings for 100 kHz, enable
- **`I2C1ProbeAddress()`** — polled probe: start + wait ARDY + check NACK bit → present if ARDY && !NACK && !AL; soft-recover on timeout
- **SYSBIOS** — real-time kernel (`BIOS_start()`) with Task, Clock, Cache & MMU

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point & scanner task (80 lines) |
| `i2c1_scanner.c` / `.h` | I2C1 init, probe, and soft-recovery using direct register access |
| `UART_soc.c` | UART hardware attributes (6 instances, from PDK) |
| `am335x_app_bbbam335x.cfg` | SYSBIOS configuration |
| `AM335x.lds` | Linker script |

## Example Output

```
+---------------------------------------------+
|   AM3352  I2C1  Bus  Scanner                |
|   SCL = P9_17   SDA = P9_18   100 kHz       |
+---------------------------------------------+

      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:
10:    -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20:    -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30:    -- -- -- -- -- -- -- -- -- -- -- -- 3c -- -- --
40:    -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50:    -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60:    -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
70:    -- -- -- -- -- -- -- --

1 device(s) detected on I2C1.
```

- `XX` = ACK received (device present)
- `--` = NACK (no device)
- blank = reserved range, not probed

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
   - GEL file: `C:\ti\ccs1281\ccs\ccs_base\emulation\boards\beaglebone\gel\beagleboneblack.gel`
3. Open serial terminal (e.g. PuTTY, TeraTerm) — 115200 8N1
4. Connect **10 kΩ pull-up resistors** from P9_17 (SCL) and P9_18 (SDA) to 3.3 V
5. Launch target — scanner runs once and prints the grid
