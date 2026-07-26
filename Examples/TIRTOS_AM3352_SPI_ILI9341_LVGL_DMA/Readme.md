# TIRTOS AM3352 SPI1 ILI9341 LCD + LVGL Music Demo with EDMA3 DMA

TI-RTOS (SYS/BIOS) project for **AM3352** running **LVGL v9.2.1 Music Demo** on an **ILI9341 2.8" TFT LCD** (240x320) via **SPI1 @ 24 MHz** with **EDMA3 DMA** transfers.

Hardware target: **Antminer L3+ board** (form factor similar to BeagleBone Black, AM3352 SoC).

## Performance

| Scenario | FPS |
|----------|-----|
| Music demo — full animation (spectrum bars, cover art transitions, scroll) | ~1 FPS |
| Simple gfx pages (static shapes, text, color bands) | Up to 30 FPS |

The 1 FPS baseline during heavy animation is expected given SPI bandwidth constraints. The ILI9341 at 24 MHz through SPI delivers ~3 MB/s raw throughput; pushing 16-bit RGB565 pixel data at 320x240 requires ~150 KB per frame. With stripe-based partial rendering (40-pixel tall stripes) and SPI overhead, full-frame animation moves slowly. Static or low-motion UI achieves significantly higher frame rates.

## Architecture

```
main()
 |
 +-- Board_init()        : pinmux, clocks, UART stdio
 +-- Clock_create()      : 5ms LVGL tick callback
 +-- Task_create()       : lvgl_demo_task (priority 1, 8 KB stack)
 +-- BIOS_start()
         |
         v
lvgl_demo_task()
 |
 +-- Pinmux override (SPI1 pads) + GPIO clock enable
 +-- SPI_init() + EDMA3 init
 +-- SPI_open() @ 24 MHz, callback mode
 +-- LCD reset (RST toggle + 120ms wait)
 +-- ILI9341_Init()      : power-on sequence, display config
 +-- lv_init()           : LVGL core initialization
 +-- lv_port_disp_init() : register display driver, double buffer
 +-- lv_demo_music()     : launch music demo
 +-- while(1): lv_timer_handler() + Task_sleep(5)
         |
         v
LCD_FlushDisplay()      : LVGL stripe-based flush callback
 |
 +-- ILI9341 COLADDR + PAGEADDR set
 +-- ILI9341 GRAM command
 +-- Cache maintenance (wb/Inv) for DMA coherence
 +-- Spi1TxBuffer()      : DMA transfer pixel data
 +-- lv_disp_flush_ready()
         |
         v
SPI_callback()          : EDMA3 transfer complete ISR
 |
 +-- Posts lcdSem (unblocks Spi1TxBuffer caller)
```

### File Organization

| File | Description |
|------|-------------|
| `main.c` | Entry point, SPI/DMA helpers, task creation, LCD init |
| `ili9341.c/.h` | ILI9341 LCD driver — commands, pixel/shape/text drawing primitives |
| `simple_gfx.c/.h` | Standalone graphics demos (color bands, shapes, text, pixel grid) |
| `lv_port_disp.c/.h` | LVGL display port — double-buffered stripe flush via SPI DMA |
| `utils.c/.h` | GPIO helpers for DC/RST pins, delay wrapper |
| `GPIO_bbbAM335x_board.c/.h` | Board GPIO pin definitions (DC=P8_26, RST=P8_19) |
| `spi_arm_dma_bbbAM335x_slavemode.cfg` | SYS/BIOS XDC configuration (MMU, cache, packages) |
| `sample_arm_init.c` | EDMA3 driver initialization |
| `sample_am335x_cfg.c` | EDMA3 platform config (64 channels, 256 PaRAM sets) |
| `sample_arm_cs.c` | EDMA3 critical sections (mutex + cache ops) |
| `sample_am335x_arm_int_reg.c` | EDMA3 interrupt registration with SYS/BIOS Hwi |
| `SPI_log.c/.h` | UART printf logging wrapper |
| `Middlewares/lvgl/` | LVGL v9.2.1 source tree + demos |

## Hardware

- **MCU**: AM3352 (ARM Cortex-A8, 600 MHz)
- **Board**: Antminer L3+ (BeagleBone Black form factor)
- **Display**: ILI9341 2.8" TFT LCD, 240x320 pixels, RGB565
- **Interface**: SPI1 @ 24 MHz with EDMA3 DMA

### Pin Mapping

| Function | Pin | Location | Signal |
|----------|-----|----------|--------|
| SCK | P9-31 | A13 | SPI1_SCLK |
| MOSI | P9-30 | D12 | SPI1_D0 |
| CS | P9-28 | C12 | SPI1_CS0 |
| DC (Data/Command) | P8-26 | GPIO1[29] | GPIO output |
| RST (Reset) | P8-19 | GPIO0[22] | GPIO output |

## Display Driver

### Stripe-Based Partial Rendering

The display driver uses horizontal stripe rendering to keep frame buffers small:

- **Stripe height**: 40 pixels
- **Framebuffer size**: 2 buffers x 320 cols x 40 rows x 2 bytes = ~51 KB total
- **Render mode**: `LV_DISPLAY_RENDER_MODE_PARTIAL` — only dirty areas are flushed

This minimizes RAM usage (critical on the 64 KB OCMC + limited DDR allocation) while still allowing smooth incremental updates.

### Double Buffering

Two framebuffers are alternated by LVGL. When one stripe is being flushed via SPI DMA, LVGL composes the next stripe into the other buffer. The flush callback (`LCD_FlushDisplay`) signals completion via `lv_disp_flush_ready()`.

### SPI DMA Transfer Flow

1. `Spi1TxBuffer()` splits data into 64 KB chunks
2. Cache write-back on TX buffer (DMA coherence)
3. `SPI_transfer()` submits to TI SPI driver in callback mode
4. EDMA3 hardware moves data from RAM to SPI FIFO — zero CPU copy
5. `SPI_callback()` posts `lcdSem` on completion
6. Caller blocks on `SPI_osalPendLock(lcdSem, 5000)` until done
7. Cache invalidate on RX buffer after transfer

### Memory Layout

```
SRAM_LO   : 0x402F0000, 1 KB   (OCMC start)
SRAM_HI   : 0x402F0400, 63 KB  (OCMC remaining)
OCMC_SRAM : 0x40300000, 64 KB
DDR3      : 0x80000000, 512 MB (code, data, stacks, framebuffers)
```

All application sections (.text, .rodata, .data, .bss, .heap, .stack) are linked to DDR3.

## Toolchain

- **TI Processor SDK RTOS AM335x** v1.0.17
- **Code Composer Studio** v12.8.1
- **Compiler**: GCC ARM 7.3.1 (`gcc-arm-none-eabi-7-2018-q2-update`)
- **XDCtools**: v3.55.2.22_core
- **SYS/BIOS**: v6.76.03.01
- **EDMA3 LLD**: v2.12.05.30E
- **LVGL**: v9.2.1 (with Music Demo)

## Build & Run

1. Open project in CCS v12.8.1
2. Build with **Debug** configuration
3. Connect via **JLink** debugger, target device `AM3352` / `AM3359` (Cortex-A8)
4. GEL file: `ccs_base/emulation/boards/beaglebone/gel/beagleboneblack.gel`
5. Open serial terminal — 115200 baud, 8N1
6. Expected boot output:
   ```
   === LVGL MUSIC DEMO ===
   SPI initialized for LCD
   ILI9341_Init() done
   EDMA driver initialization PASS.
   Starting Music Demo...
   Music Demo running
   ```
7. LCD will show the LVGL Music Demo playing automatically (cover art, spectrum visualization, song title scroll, progress bar)

## LVGL Configuration

| Parameter | Value | Notes |
|-----------|-------|-------|
| Color depth | 16-bit RGB565 | Standard TFT format |
| Byte swap | Enabled | `LV_COLOR_16_SWAP=1` for SPI transmission order |
| Memory pool | 256 KB | `LV_MEM_SIZE` — heap allocator for LVGL objects |
| OS | None (self-managed) | Runs on SYS/BIOS Task via `lv_timer_handler()` |
| Tick source | 5 ms SYS/BIOS Clock | `lv_tick_inc(5)` every 5 ms |
| Render mode | Partial (stripe) | 40-pixel tall horizontal stripes |
| Font | Montserrat 12pt, 16pt | Embedded compressed fonts |
| Perf monitor | Enabled | `LV_USE_PERF_MONITOR=1` |
| Sysmon | Enabled | `LV_USE_SYSMON=1` |

## Graphics Demos

In addition to the LVGL Music Demo, the project includes standalone graphics demos in `simple_gfx.*`:

1. **Color Bands** — 8 horizontal color stripes
2. **Shapes** — rectangles, filled/unfilled circles, diagonal lines
3. **Text** — uppercase, lowercase, digits in various colors
4. **Pixel Grid** — checkerboard + solid fill pattern

These are NOT currently called from main (replaced by the LVGL demo) but remain available for testing the SPI LCD pipeline at high frame rates.

## Key Implementation Notes

### Cache Coherence

Despite `Cache.enableCache = false` in the SYS/BIOS config, explicit `CacheP_*()` calls are made before/after every DMA transfer. On this build they act as no-ops, but the code is structured to be cache-safe if MMU caching is enabled in future revisions.

### LVGL Tick Discipline

A SYS/BIOS Clock fires every 5 ticks (5 ms), calling `lv_tick_inc(5)`. This gives LVGL a consistent timebase. The `lv_timer_handler()` loop runs at the same 5 ms interval, processing pending animations, transitions, and redraws.

### No Input Device

Touch input is not implemented. The Music Demo runs in fully automatic mode (`LV_DEMO_MUSIC_AUTO_PLAY=1`), cycling through songs and animations without user interaction.
