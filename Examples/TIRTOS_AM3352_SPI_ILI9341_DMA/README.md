# TIRTOS AM3352 SPI1 + ILI9341 LCD Demo (EDMA3)

Embedded firmware untuk **Texas Instruments AM3352** (BeagleBone/AM335x) yang mengintegrasikan:

- **SPI1** dengan **DMA (EDMA3)** untuk transfer data berkecepatan tinggi
- **ILI9341** 2.8" TFT LCD (320x240) via SPI
- **TI-RTOS (BIOS 6.x)** sebagai real-time operating system
- **GPIO** untuk kontrol DC (Data/Command) dan RST (Reset) LCD

## Hardware

| Item | Detail |
|------|--------|
| **MCU** | TI AM3352 (ARM Cortex-A8) |
| **Board** | Antminer L3+ (AM3352 based) |
| **LCD** | ILI9341 2.8" TFT 320x240 |
| **SPI** | SPI1 — SCK=P9_31, MOSI=P9_30, CS=P9_28 |
| **GPIO DC** | P8_26 = GPIO1[29] |
| **GPIO RST** | P8_19 = GPIO0[22] |
| **Clock** | 24 MHz SPI @ DMA mode |

## Software Stack

| Komponen | Versi / Paket |
|----------|--------------|
| **OS** | TI-RTOS (BIOS 6.76) |
| **SDK** | Processor SDK RTOS AM335x 6.03 |
| **SPI Driver** | ti.drv.spi (PDK) |
| **EDMA3 Driver** | ti.sdo.edma3.drv + ti.sdo.edma3.rm (LLD 2.12) |
| **GPIO Driver** | ti.drv.gpio |
| **Compiler** | TI ARM CGT (armcc/clang) |

## Struktur Project

```
TIRTOS_AM3352_SPI_ILI9341_DMA/
├── main.c                  # Entry point — creates LCD demo task
├── tirtos_am3352_spi_ili9341_dma.cfg  # BIOS/XDC configuration
├── GPIO_board.h            # GPIO pin definitions for LCD
├── SPI_log.h               # UART printf wrapper for debug output
│
├── src/
│   └── makefile.libs       # EDMA3/SPI library build rules
│
├── ili9341/                # ILI9341 LCD driver
│   ├── ili9341.c           # Core driver (init, fill, draw primitives)
│   ├── ili9341.h           # Public API + color constants
│   ├── fonts.h             # 5x7 pixel font bitmap
│   └── delay.h             # Delay function header
│
├── simple_gfx.c            # Graphics demo functions
├── simple_gfx.h            # Demo function declarations
│
├── utils.c                 # SPI DMA helpers + GPIO control
├── utils.h                 # Utility function headers
│
└── sample_*.c              # EDMA3 platform-specific drivers
    ├── sample_arm_init.c           # EDMA3 init/deinit
    ├── sample_arm_cs.c             # Cache + semaphore ops
    ├── sample_am335x_cfg.c         # AM335x EDMA3 hardware config
    └── sample_am335x_arm_int_reg.c # Interrupt registration
```

## Fitur Utama

### SPI + EDMA3 DMA
- Transfer SPI menggunakan **EDMA3** untuk efisiensi CPU
- Callback-based transfer dengan semaphore synchronization
- Cache maintainance (Clean/Invalidate) untuk DMA-coherent memory
- 6-line SPI mode (SCK, MOSI, MISO, CS0, CS1, CS2)

### ILI9341 LCD Driver
- Full initialization sequence (power, gamma, display commands)
- Batched command+data transfers untuk mengurangi overhead SPI
- Row-major character rendering
- Primitives: `FillScreen`, `FillRect`, `DrawPixel`, `DrawLine`, `DrawRect`, `DrawCircle`, `FillCircle`, `DrawChar`, `DrawString`
- 16-bit RGB5:6:5 color format

### Simple GFX Demos
1. **Color Bands** — 8 horizontal color bars (Red, Orange, Yellow, Green, Cyan, Blue, Magenta, White)
2. **Shapes** — Rectangles, circles, and diagonal lines
3. **Text** — Alphanumeric strings with multiple colors/sizes
4. **Pixel Grid** — Checkerboard pattern on left half, solid fill on right

### Task Architecture
```
main()
 └── lcd_demo_task (priority 1, stack 0x2000)
      ├── SPI1 init + EDMA3 setup
      ├── GPIO init (DC, RST pins)
      ├── LCD reset + ILI9341_Init()
      └── while(1) → ColorBands → Shapes → Text → PixelGrid
```

## Build & Flash

### Requirements
- **CCS (Code Composer Studio)** 12.x
- **Processor SDK RTOS AM335x** 6.03
- **EDMA3 LLD** 2.12
- **TI ARM Compiler** (cg_xml 2.61+)

### Build Steps
1. Import project ke CCS via **File → Import → Code Composer Studio → Existing CCS Project**
2. Pilih folder workspace
3. Klik kanan project → **Build Project**

### Output Files
- `Debug/TIRTOS_AM3352_SPI_ILI9341_DMA.out` — ELF binary (debug)
- `Debug/TIRTOS_AM3352_SPI_ILI9341_DMA.bin` — Raw binary

### Flashing
Gunakan CCS **Target → Connect** dan **Debug** untuk load ke board AM3352.

## Pin Mapping

### SPI1 (MCSPI1)
| Signal | Pin | Function |
|--------|-----|----------|
| SCK | P9_31 | SPI clock |
| MOSI | P9_30 | Master Out Slave In |
| CS0 | P9_28 | Chip Select 0 |

### GPIO
| Signal | Pin | GPIO |
|--------|-----|------|
| LCD_DC | P8_26 | GPIO1[29] |
| LCD_RST | P8_19 | GPIO0[22] |

## Debug Output

Output debug ditampilkan melalui **UART** menggunakan `UART_printf()` via `SPI_log()` macro. Lihat console CCS Serial Terminal untuk melihat log:

```
=== ILI9341 LCD DEMO ===
SPI1 DMA, DC=P8_26, RST=P8_19

EDMA driver initialization PASS.
SPI initialized for LCD
ILI9341_Init() done
```

## Key APIs

### ILI9341 Driver
```c
void ILI9341_Init(void);
void ILI9341_FillScreen(uint16_t color);
void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ILI9341_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void ILI9341_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_DrawCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
void ILI9341_FillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg);
void ILI9341_DrawString(uint16_t x, uint16_t y, const char *s, uint16_t fg, uint16_t bg);
void ILI9341_WritePixels(const uint16_t *pixels, uint32_t count);
```

### Simple GFX Demos
```c
void simple_gfx_demo_color_bands(void);
void simple_gfx_demo_shapes(void);
void simple_gfx_demo_text(void);
void simple_gfx_demo_pixel_grid(void);
```

### Utilities
```c
void Spi1TxByte(uint8_t b);
void Spi1TxBuffer(const uint8_t *buf, uint32_t len);
void LcdDcLow(void);
void LcdDcHigh(void);
void LcdRstLow(void);
void LcdRstHigh(void);
void delay(uint32_t ms);
```

## Known Issues & Notes

- **Cache disabled** di `tirtos_am3352_spi_ili9341_dma.cfg` (`Cache.enableCache = false`). Memory access tidak di-cache untuk menghindari coherency issues dengan peripheral registers.
- **MMU enabled** dengan section descriptor untuk peripheral registers (non-cacheable, non-executable).
- **HWI pointer cast** pada `sample_am335x_arm_int_reg.c` diperlukan karena perbedaan tipe alias antara EDMA3 LLD sample (BIOS 6.x) dan header `Hwi.h` modern.
- LCD demo berjalan **looping terus-menerus** tanpa exit condition.

## License

Based on Texas Instruments EDMA3 LLD Sample Code (BSD-style license).
