# TIRTOS AM3352 SPI1 ILI9341 LCD Demo with DMA

TI-RTOS (SYSBIOS) project for **AM3352** — **ILI9341 2.8" TFT LCD** demo via **SPI1** with **EDMA3 DMA** transfer.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      main() - BIOS Start                    │
│   1. Create LCD Demo Task (priority 1, stack 0x2000)        │
│   2. Initialize Board (pinmux, clock, UART)                 │
│   3. Start BIOS (runs lcd_demo_task)                        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              lcd_demo_task() — Main Application             │
│   1. Pinmux override for SPI1 + GPIO0 clock enable          │
│   2. Init SPI + EDMA3                                       │
│   3. Create semaphores (cbSem for callback, lcdSem for LCD) │
│   4. Open SPI1 at 24 MHz, callback mode                     │
│   5. LCD Reset sequence (DC/RST GPIO toggle)                │
│   6. ILI9341_Init() — send init commands                    │
│   7. Loop: run graphics demos forever                       │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              SPI_callback() — DMA Completion ISR            │
│   Called by SPI driver when EDMA3 finishes a transfer.      │
│   Posts cbSem and/or lcdSem to unblock waiting caller       │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│         Spi1TxByte() / Spi1TxBuffer() — SPI Helpers         │
│   Blocking SPI transfer wrappers using DMA + semaphore wait │
│   Handles cache maintenance (wb/Inv) for DMA coherence      │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              simple_gfx.* — Graphics Demos                  │
│   - Color bands (8 colored stripes)                         │
│   - Shapes (rect, circle, line)                             │
│   - Text (uppercase, lowercase, digits)                     │
│   - Pixel grid (checkerboard pattern)                       │
└─────────────────────────────────────────────────────────────┘
```

### File Organization

| File | Description |
|------|-------------|
| `main.c` | SPI init, DMA callbacks, task entry points |
| `ili9341.c/.h` | LCD driver (commands, pixel drawing primitives) |
| `simple_gfx.c/.h` | Higher-level demo scenes (shapes, text, patterns) |
| `utils.c/.h` | Low-level helpers (delay, DC/RST GPIO control) |
| `SPI_log.c` | UART logging helper |
| `spi_arm_dma_bbbAM335x_slavemode.cfg` | SYSBIOS configuration |
| `TIRTOS_AM3352_SPI_ILI9341_DMA.cmd` | Linker script |

## Hardware

- **MCU**: AM3352 (Cortex-A8) — Antminer L3+ board
- **Display**: ILI9341 2.8" TFT LCD (240x320 pixels)
- **Interface**: SPI1 @ 24 MHz, DMA mode (EDMA3)

### Pin Mapping

| Function | Pin | Location | Signal |
|----------|-----|----------|--------|
| SCK | P9-31 | A13 | SPI1_SCLK |
| MOSI | P9-30 | D12 | SPI1_D0 |
| CS | P9-28 | C12 | SPI1_CS0 |
| DC (Data/Command) | P8-26 | GPIO1[29] | GPIO output |
| RST (Reset) | P8-19 | GPIO0[22] | GPIO output |

## Graphics Demos

The LCD cycles through these demo scenes continuously:

1. **Color Bands** — 8 horizontal color stripes (Red, Orange, Yellow, Green, Cyan, Blue, Magenta, White)
2. **Shapes** — Rectangle, filled rectangle, outline circle, filled circle, diagonal lines
3. **Text** — Uppercase, lowercase, and digit strings in various colors
4. **Pixel Grid** — Checkerboard pattern on left half, solid green on right half

## Toolchain

- **Processor SDK RTOS AM335x** — [v1.0.17](https://www.ti.com/tool/download/PROCESSOR-SDK-RTOS-AM335X)
- **Code Composer Studio** — v12.8.1
- **Compiler** — GCC ARM 7.3.1 (`gcc-arm-none-eabi-7-2018-q2-update`)
- **XDCtools** — v3.55.2.22_core
- **SYSBIOS** — v6.76.3.01
- **EDMA3 LLD** — v2.12.05.30E

## Build & Run

1. Build via CCS — Configuration **Debug**
2. Debug using **JLink** with target configuration:
   - Device: `AM3352` / `AM3359` (Cortex-A8)
   - CCXML: use target configuration with JLink
   - GEL file: `C:\ti\ccs1281\ccs\ccs_base\emulation\boards\beaglebone\gel\beagleboneblack.gel`
3. Open serial terminal (e.g. PuTTY, TeraTerm) — 115200 baud
4. Observe output:
   ```
   === ILI9341 LCD DEMO ===
   SPI1 DMA, DC=P8_26, RST=P8_19

   EDMA driver initialization PASS.
   SPI initialized for LCD
   ILI9341_Init() done
   ```
5. LCD will flash red briefly, then cycle through graphics demos.

## Key Implementation Details

### DMA Transfer Flow
1. `Spi1TxByte()` / `Spi1TxBuffer()` prepare TX buffer and call `SPI_transfer()`
2. EDMA3 hardware transfers data from RAM to SPI FIFO without CPU involvement
3. On completion, `SPI_callback()` posts `lcdSem` semaphore
4. Caller blocks on `SPI_osalPendLock(lcdSem)` until transfer finishes

### Cache Coherence
- **CacheP_wb()** — Write Back: flushes CPU cache before DMA reads TX data
- **CacheP_wbInv()** — Write Back + Invalidate: prepares RX buffer for DMA write
- **CacheP_Inv()** — Invalidate: refreshes CPU cache after DMA writes RX data

### LCD Initialization Sequence
1. GPIO RST high → low (10ms) → high (10ms) → wait 120ms
2. Send ILI9341 command sequence (power, gamma, column/page address, display on)
3. Total init takes ~170ms (10ms reset + 120ms wait + command transmission)
