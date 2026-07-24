# AM335x TI-RTOS (SYS/BIOS) x CCS Portable Project

<p align="center">
  <img src="Doc/bg.png" alt="AM335x TI-RTOS Banner" width="500px">
</p>

> ⚠️ **Important — Use Case:** This project is **not** targeting the original BeagleBone. It is targeted at the **Antminer L3+** mining board, which is built around the **TI AM3352** (Cortex-A8) SoC. The PCB form factor is roughly similar to the BeagleBone Black, **but without the PRU subsystems**. This makes the Antminer L3+ a **low-cost option to repurpose** retired mining hardware as a learning platform for TI's RTOS Cortex-A stack via Processor SDK RTOS — "for fun and profit". The banner image above is the Antminer L3+ board itself.

<p align="center">
  <strong>A curated collection of standalone, portable Processor SDK TI-RTOS (SYS/BIOS) demo projects for the AM3352 (Antminer L3+) platform, ready to import into Code Composer Studio.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/platform-AM335x-orange?logo=texas-instruments" alt="Platform">
  <img src="https://img.shields.io/badge/toolchain-CCS%20v12%2B-blue?logo=texas-instruments" alt="CCS">
  <img src="https://img.shields.io/badge/language-C-green?logo=c" alt="Language">
  <img src="https://img.shields.io/badge/SDK-PROCESSOR--SDK--RTOS--AM335X%2005.03.00.07-red" alt="SDK">
  <img src="https://img.shields.io/badge/kernel-SYS%2FBIOS%20v6.76.3.01-purple" alt="SYSBIOS">
  <img src="https://img.shields.io/badge/status-active-success" alt="Status">
</p>

---

## Overview

This repository hosts **independent, portable** Code Composer Studio (CCS) projects derived from TI's **Processor SDK RTOS for AM335x 05.03.00.07** package. Each project has been extracted from the monolithic PDK tree and converted into a **self-contained CCS project** that lives entirely inside its own folder.

Unlike a "copied" PDK example — which usually drags in hundreds of `linkedResources` pointing back into the SDK tree (`.../pdk_am335x_1_0_17/...`) — these projects carry **only the application source** for the demo. The PDK driver libraries, SYS/BIOS kernel, and XDCtools are referenced through centralized paths on disk:

```
C:\ti\pdk_am335x_1_0_17
C:\ti\bios_6_76_03_01
C:\ti\xdctools_3_55_02_22_core
C:\ti\gcc-arm-none-eabi-7-2018-q2-update
```

This makes the projects:

- **Portable** — move the workspace anywhere, only update the SDK root paths if needed.
- **Clean** — no `linkedResources` from inside the project folder pointing into the PDK tree.
- **Reproducible** — every project is a standalone CCS C project you can import in one click.

---

## Prerequisites

| Component | Version / Detail |
|---|---|
| Code Composer Studio | **[v12.8.1](https://www.ti.com/tool/download/CCSTUDIO/12.8.1)** — must include **Sitara AM3x ARM Processors** component |
| Processor SDK RTOS AM335x | **version 05.03.00.07** — install from [here](https://www.ti.com/tool/download/PROCESSOR-SDK-RTOS-AM335X/05.03.00.07) |
| PDK | ships with the SDK above — installed at `C:\ti\pdk_am335x_1_0_17` |
| SYS/BIOS | ships with the SDK — installed at `C:\ti\bios_6_76_03_01` |
| XDCtools | ships with the SDK — installed at `C:\ti\xdctools_3_55_02_22_core` |
| GCC ARM Compiler | **gcc-arm-none-eabi-7-2018-q2-update** (ships with the SDK) |
| Target Board | **Antminer L3+** (TI AM3352, Cortex-A8) — repurposed mining hardware |
| Emulator / Debugger | **J-Link** (tested & confirmed working) — solder a JTAG cable onto the Antminer L3+ board first. Follow the [BeagleBone Black JTAG connector soldering tutorial](https://dr-kino.github.io/2020/07/22/Beaglebone-black-soldering-jyag-connector/) (the JTAG footprint on the Antminer L3+ is in the same rear-edge position). XDS100v2 / XDS200 will also work if you prefer TI emulators. |

> The PDK driver libraries and include paths are pulled from:
> `C:\ti\pdk_am335x_1_0_17`
>
> The SYS/BIOS kernel is pulled from:
> `C:\ti\bios_6_76_03_01`
>
> **Note on compatibility:** Processor SDK RTOS was originally written for the BeagleBone (AM335x). The Antminer L3+ uses the AM3352 — same ARM Cortex-A8 core, but **without the PRU** subsystems. Any demo that uses PRU-ICSS features will not work on this board. Stick to the GPIO / UART / Timer / I2C / SPI / MMCSD demos built on top of the PDK driver layer and you're good.

---

## Getting Started

Looking for the full step-by-step setup (download CCS, install Processor SDK RTOS, set up target contents, then import the portable samples)?

👉 **See [`INSTALL.md`](./INSTALL.md)** for the complete installation guide.

Quick summary:

1. Install **Code Composer Studio v12.8.1** into `C:\ti\ccs1281` with **Sitara AM3x ARM Processors** support.
2. Download & install **Processor SDK RTOS AM335x 05.03.00.07** — this installs the PDK (`pdk_am335x_1_0_17`), SYS/BIOS (`bios_6_76_03_01`), XDCtools, and the GCC ARM toolchain all at once.
3. In CCS, set the **Discover Build Tools** paths so that the GCC ARM compiler, XDCtools, and SYS/BIOS are auto-resolved.
4. **Project → Import CCS Projects...** → point at the `Examples/` folder inside this workspace → tick the portable demos you want.
5. Build, launch a debug session, and run on the Antminer L3+.

---

## Project Index

> Each project below lives in its own folder under [`Examples/`](./Examples/). Click any title to jump to the folder. Descriptions are intentionally short — the per-project `README.md` inside each folder has the long version.
>
> **Project origin labels:**
> - 🟦 **PDK ref** — project adapted directly from the PDK example tree (`C:\ti\pdk_am335x_1_0_17\packages\ti\board\examples\` or similar). Application code is the TI reference implementation.
> - 🟧 **Custom from empty** — project built **from scratch** as an empty CCS project: hand-written `main.c`, hand-picked `AM335x.lds` linker script, and a manual `.cfg` SYS/BIOS configuration, but still references the PDK board/driver libraries under `pdk_am335x_1_0_17`.

### GPIO

- 🟧 [**`Examples/TIRTOS_AM3352_GPIO_LED/`**](./Examples/TIRTOS_AM3352_GPIO_LED/) — SYS/BIOS blinky: toggle D2 (GPIO1[23]) every 500 ms via `Task_sleep()`.
- 🟧 [**`Examples/TIRTOS_AM3352_GPIO_LED_SEQUENCE/`**](./Examples/TIRTOS_AM3352_GPIO_LED_SEQUENCE/) — SYS/BIOS 4-LED sweeping sequence: ping-pong pattern across D2–D5 (GPIO1[21]–GPIO1[24]), 500 ms per step via `Task_sleep()`.
- 🟧 [**`Examples/TIRTOS_AM3352_GPIO_INTERRUPT/`**](./Examples/TIRTOS_AM3352_GPIO_INTERRUPT/) — SYS/BIOS GPIO interrupt demo: LED D2 blinks at 1 s; push button on P9_12 (GPIO1[28]) decrements blink delay by 100 ms per press.

### Communication

- 🟧 [**`Examples/TIRTOS_AM3352_UART_ECHO/`**](./Examples/TIRTOS_AM3352_UART_ECHO/) — UART interrupt echo via SYS/BIOS Task: reads characters and echoes back with `echo> ` prefix. Demonstrates `Board_initUART()`, UART_read()/UART_write() in a task context.
- 🟧 [**`Examples/TIRTOS_AM3352_I2C_SCANNER/`**](./Examples/TIRTOS_AM3352_I2C_SCANNER/) — I2C1 bus scanner: probes addresses 0x03–0x77 and prints an `i2cdetect -y 1`-style grid over UART0. Uses direct register-access I2C driver with polled probe and soft-recovery.
- 🟧 [**`Examples/TIRTOS_AM3352_I2C_SSD1306/`**](./Examples/TIRTOS_AM3352_I2C_SSD1306/) — SSD1306 128×32 OLED LCD driver over I2C1 @ 100 kHz via PDK `I2C_transfer()`. Demo loop: splash screen, shapes, filled shapes, live counter. Includes fonts 7×10, 11×18, 16×26.
- 🟧 [**`Examples/TIRTOS_AM3352_SPI_TX/`**](./Examples/TIRTOS_AM3352_SPI_TX/) — SPI0 TX baseline with hardware CS: toggles DC/RST GPIOs (P8_26, P8_19) and sends bytes via SPI0 @ 100 kHz. Demonstrates MCSPI 4-pin mode, GPIO0 clock enable, and direct pinmux.
- 🟧 [**`Examples/TIRTOS_AM3352_SPI_ILI9341/`**](./Examples/TIRTOS_AM3352_SPI_ILI9341/) — ILI9341 2.8" TFT LCD driver over SPI0 @ 24 MHz (TX_ONLY, polling mode) with primitives (pixel, fill, line, circle, rectangle, 5×7 text). 4 demo scenes: color bands, shapes, text, pixel grid. Optimized FillRect with 64 KB chunk buffer.
- 🟧 [**`Examples/TIRTOS_AM3352_SPI_ILI9341_LVGL/`**](./Examples/TIRTOS_AM3352_SPI_ILI9341_LVGL/) — ILI9341 2.8" TFT LCD with LVGL v9.2.1: Music Player Demo with album art, track list, and play controls. 2 frame buffers (307 KB), SPI burst mode via direct register access.
- 🟧 [**`Examples/TIRTOS_AM3352_MMCSD_SDCARD_FATFS/`**](./Examples/TIRTOS_AM3352_MMCSD_SDCARD_FATFS/) — MMCSD FATFS with interactive shell: auto card detect via GPIO interrupt, dynamic mount/unmount, full file ops (ls, cd, mkdir, rm, cat, pwd) over UART0.
- 🟧 [**`Examples/TIRTOS_AM3352_MMCSD_SDCARD/`**](./Examples/TIRTOS_AM3352_MMCSD_SDCARD/) — Raw MMCSD read/write: open SDMMC, read card params, write/verify/update/verify pattern to sectors.
- 🟧 [**`Examples/TIRTOS_AM3352_NIMU_BASIC/`**](./Examples/TIRTOS_AM3352_NIMU_BASIC/) — NIMU Ethernet: CPSW interface with NDK TCP/IP stack, static IP assignment, RX/TX statistics task printing counters every 10 seconds.
- 🟧 [**`Examples/TIRTOS_AM3352_NIMU_FTP/`**](./Examples/TIRTOS_AM3352_NIMU_FTP/) — NIMU Ethernet + FTP Server: listens on port 21, supports USER/PASS auth, STOR/RETR/XPWD/PORT commands. Active mode only, max 5 concurrent clients. File I/O is a stub (socket data only).
- 🟧 [**`Examples/TIRTOS_AM3352_SPI_TX_V2/`**](./Examples/TIRTOS_AM3352_SPI_TX_V2/) — SPI1 TX baseline v2 with hardware CS: continuous `0xAF` loop @ 100 kHz. Cleaned project structure with local SOC files and absolute PDK paths.

---

## Project Status

| Project | Status | Notes |
|---|---|---|
| `TIRTOS_AM3352_GPIO_LED` | ✅ Stable | GPIO1[23] blink via SYS/BIOS Task + Board driver |
| `TIRTOS_AM3352_GPIO_LED_SEQUENCE` | ✅ Stable | 4-LED ping-pong sweep across D2–D5 via SYS/BIOS Task |
| `TIRTOS_AM3352_GPIO_INTERRUPT` | ✅ Stable | GPIO interrupt-driven blink speed control via push button |
| `TIRTOS_AM3352_UART_ECHO` | ✅ Stable | UART echo via SYS/BIOS Task + Board driver |
| `TIRTOS_AM3352_I2C_SCANNER` | ✅ Stable | I2C1 bus scanner with `i2cdetect`-style output over UART0 |
| `TIRTOS_AM3352_I2C_SSD1306` | ✅ Stable | SSD1306 128×32 OLED driver over I2C1 via PDK I2C |
| `TIRTOS_AM3352_SPI_TX` | ✅ Stable | SPI0 TX with hardware CS + GPIO DC/RST toggle |
| `TIRTOS_AM3352_SPI_ILI9341` | ✅ Stable | ILI9341 2.8" TFT LCD driver over SPI0 @ 24 MHz |
| `TIRTOS_AM3352_SPI_ILI9341_LVGL` | ✅ Stable | ILI9341 + LVGL v9.2.1 Music Player Demo |
| `TIRTOS_AM3352_MMCSD_SDCARD_FATFS` | ✅ Stable | MMCSD FATFS with interactive shell + auto card detect |
| `TIRTOS_AM3352_MMCSD_SDCARD` | ✅ Stable | Raw MMCSD read/write with data verify |
| `TIRTOS_AM3352_NIMU_BASIC` | ✅ Stable | NIMU Ethernet: CPSW + NDK TCP/IP stack with stats task |
| `TIRTOS_AM3352_NIMU_FTP` | ✅ Stable | NIMU FTP Server: port 21, USER/PASS auth, STOR/RETR, active mode only |
| `TIRTOS_AM3352_SPI_TX_V2` | ✅ Stable | SPI1 TX continuous `0xAF` loop @ 100 kHz (cleaned structure) |

---

## How These Projects Are Different

A typical PDK "copied project" pulls source files via Eclipse `linkedResources` like this:

```xml
<link>
  <name>src/main.c</name>
  <locationURI>PARENT-2-Project_LOC/ti/board/examples/gpio_led_blink/main.c</locationURI>
</link>
```

That means moving the project breaks it — the `PARENT-N-Project_LOC` chain only resolves correctly if the project sits at the exact same depth inside the PDK tree.

**This repository does it differently.** Each project folder contains:

- The demo's own `main.c` and application source
- The SYS/BIOS `.cfg` configuration file
- The CCS `.project`, `.cproject`, `.ccsproject` and launch files
- A `targetConfigs/` folder with the board `.ccxml` debug configuration
- Build output (`Debug/`, `Release/`)

The only external dependencies are the **PDK library + include paths**, the **SYS/BIOS kernel**, and the **XDCtools** — each is a single environment-variable / project-variable pointer. Update one variable, and every project follows.

---

## Folder Layout

```
Workspace_12/
├── README.md                  ← you are here
├── INSTALL.md
├── .gitignore
├── Doc/
│   └── bg.png                 ← banner image
└── Examples/                  ← all portable CCS projects live here
    ├── TIRTOS_AM3352_GPIO_LED/             ← SYS/BIOS GPIO blinky via Task_sleep()
    ├── TIRTOS_AM3352_GPIO_LED_SEQUENCE/    ← SYS/BIOS 4-LED sweep via Task
    ├── TIRTOS_AM3352_GPIO_INTERRUPT/       ← SYS/BIOS GPIO interrupt blink control
    ├── TIRTOS_AM3352_UART_ECHO/            ← SYS/BIOS UART echo via Task
    ├── TIRTOS_AM3352_I2C_SCANNER/          ← SYS/BIOS I2C1 bus scanner
    ├── TIRTOS_AM3352_I2C_SSD1306/          ← SYS/BIOS SSD1306 OLED driver
    ├── TIRTOS_AM3352_SPI_TX/               ← SYS/BIOS SPI0 TX with HW CS + GPIO toggle
    ├── TIRTOS_AM3352_SPI_ILI9341/          ← SYS/BIOS ILI9341 2.8" TFT LCD driver
    ├── TIRTOS_AM3352_SPI_ILI9341_LVGL/     ← SYS/BIOS ILI9341 + LVGL v9.2.1 Music Demo
    ├── TIRTOS_AM3352_MMCSD_SDCARD_FATFS/   ← SYS/BIOS MMCSD FATFS with interactive shell
    ├── TIRTOS_AM3352_MMCSD_SDCARD/         ← SYS/BIOS raw MMCSD read/write with DMA
    ├── TIRTOS_AM3352_NIMU_BASIC/           ← SYS/BIOS NIMU Ethernet CPSW + NDK
    ├── TIRTOS_AM3352_NIMU_FTP/             ← SYS/BIOS NIMU FTP Server (port 21, active mode)
    └── TIRTOS_AM3352_SPI_TX_V2/            ← SYS/BIOS SPI1 TX continuous 0xAF loop
```

---

## License & Credits

These projects are derived from Texas Instruments' **Processor SDK RTOS for AM335x** package. Refer to the original SDK documentation for licensing details of the underlying PDK driver libraries, SYS/BIOS kernel, and XDCtools.

<p align="center"><sub>Built for the BeagleBone • Powered by Processor SDK RTOS AM335x 05.03.00.07</sub></p>
