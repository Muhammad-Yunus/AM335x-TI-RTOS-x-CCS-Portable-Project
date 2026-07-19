# Installation Guide — AM335x TI-RTOS (SYS/BIOS) x CCS on Antminer L3+

This guide walks through setting up the full toolchain on Windows:

1. **Code Composer Studio (CCS) v12.8.1** — installed at `C:\ti\ccs1281`
2. **Processor SDK RTOS AM335x 05.03.00.07** — installed at `C:\ti\` (installs PDK, SYS/BIOS, XDCtools, GCC ARM compiler)
3. **Import the portable sample programs** in this repository
4. **Build & run** on the Antminer L3+

> 🎯 **Target hardware:** Antminer L3+ (TI AM3352, Cortex-A8). The Antminer L3+ has the same die as the BeagleBone Black except the **PRU subsystems are not populated**, so PRU-ICSS demos will not run. Stick to GPIO / UART / Timer / I2C / SPI / MMCSD — they're all Cortex-A8 main peripherals driven by the PDK driver layer and work fine.

---

## Table of Contents

- [1. Prerequisites](#1-prerequisites)
- [2. Install Code Composer Studio](#2-install-code-composer-studio)
- [3. Install Processor SDK RTOS AM335x](#3-install-processor-sdk-rtos-am335x)
- [4. Verify the SDK components on disk](#4-verify-the-sdk-components-on-disk)
- [5. Import the portable sample programs](#5-import-the-portable-sample-programs)
- [6. Build & run a sample](#6-build--run-a-sample)
- [Troubleshooting](#troubleshooting)

---

## 1. Prerequisites

| Requirement | Notes |
|---|---|
| OS | Windows 10 / 11 (64-bit) |
| Disk space | ~6 GB for CCS + ~3 GB for Processor SDK RTOS |
| RAM | 4 GB minimum, 8 GB recommended |
| Admin rights | Required for the CCS installer |
| Internet | Required for the download links below |

---

## 2. Install Code Composer Studio

### 2.1 Download

Grab the **offline installer** for **CCS 12.8.1** from TI:

🔗 **<https://www.ti.com/tool/download/CCSTUDIO/12.8.1>**

Pick the **Windows** build. The offline installer is a single `.exe` (around 2 GB) — easier than the web installer because it doesn't need to redownload components.

### 2.2 Run the installer

1. Right-click the downloaded `.exe` → **Run as administrator**.
2. When prompted for the install location, set:

   ```
   C:\ti\ccs1281
   ```

3. In the **Select Components** screen, make sure to tick:

   - ✅ **Sitara AM3x ARM Processors** (this pulls in the ARM compiler, debug drivers, and the XDS emulation support for AM335x)

   The other families are optional — only AM3x is needed for the Antminer L3+.

4. Accept the license agreements and click **Next → Install**.
5. Wait for the installation to finish (~10–20 minutes depending on disk speed).
6. **Launch CCS** when prompted.

> 💡 If you skipped the launcher, start it manually from `C:\ti\ccs1281\ccs\eclipse\ccstudio.exe`.

---

## 3. Install Processor SDK RTOS AM335x

### 3.1 Download

Grab the Windows installer from TI:

🔗 **<https://www.ti.com/tool/download/PROCESSOR-SDK-RTOS-AM335X/05.03.00.07>**

Look for the Windows (Self-Extracting Executable) — this is a single `.exe` that bundles everything:

- **PDK** (`pdk_am335x_1_0_17`) — Platform Development Kit: board drivers, peripheral drivers, and example sources
- **SYS/BIOS** (`bios_6_76_03_01`) — real-time kernel
- **XDCtools** (`xdctools_3_55_02_22_core`) — SYS/BIOS configuration & build tools
- **GCC ARM Compiler** (`gcc-arm-none-eabi-7-2018-q2-update`) — cross-compiler for Cortex-A8

### 3.2 Run the installer

1. Right-click the downloaded installer → **Run as administrator**.
2. The installer will extract and place components into `C:\ti\` automatically.
3. Accept the license agreements and click **Next → Install → Finish**.

> ⚠️ **Do not change the install paths.** The portable projects in this repo assume the standard SDK layout under `C:\ti\`. If you must use different paths, you will need to update the project build variables (PDK_INSTALL_PATH, BIOS_INSTALL_DIR, XDC_INSTALL_DIR, GCC_ARM_TOOLCHAIN) in every `.ccsproject` file.

---

## 4. Verify the SDK components on disk

After the installer finishes, you should see a folder layout like:

```
C:\ti\
├── ccs1281\                                         ← Code Composer Studio
│   └── ccs\
│       ├── eclipse\
│       └── ccs_base\
├── pdk_am335x_1_0_17\                               ← PDK (board support + drivers)
│   └── packages\
│       ├── ti\board\                                ← Board library, pinmux, SOC init
│       ├── ti\drv\                                  ← Peripheral drivers (GPIO, I2C, SPI, UART...)
│       ├── ti\osal\                                 ← OS abstraction layer
│       └── ti\csl\                                  ← Chip support library
├── bios_6_76_03_01\                                 ← SYS/BIOS real-time kernel
│   └── packages\
│       ├── ti\bios\                                 ← Kernel API, config, examples
│       ├── ti\catalog\                              ← CPU/cache/MMU catalog
│       └── ti\sysbios\                              ← SYS/BIOS kernel sources
├── xdctools_3_55_02_22_core\                        ← XDCtools
│   ├── eclipse\
│   └── packages\
├── gcc-arm-none-eabi-7-2018-q2-update\              ← GCC ARM cross-compiler
│   ├── bin\
│   └── lib\
└── ...
```

> If any of these folders are missing, the Processor SDK installer may not have extracted all components. Re-run the installer or manually run the component installers inside the downloaded package.

### 4.1 Register the toolchains in CCS

The first time you launch CCS, it will scan `C:\ti\` and auto-discover the installed tools. To verify:

1. **CCS → Window → Preferences → Code Composer Studio → Build → Build Tools Discovery**
2. Under **Auto-discover located tools**, you should see:
   - `GCC ARM...` (the cross-compiler)
   - `XDCtools`
   - `SYS/BIOS`

3. If anything is missing, click **Scan for new tools** or manually add the paths.

> 💡 The projects in this repo use **GCC ARM 7.3.1** (`gcc-arm-none-eabi-7-2018-q2-update`) as the compiler. If your CCS auto-discovered a different version, you may need to adjust the compiler version in each project's build settings.

---

## 5. Import the portable sample programs

### 5.1 Start the import

In CCS:

1. **Project → Import CCS Projects...**
2. In the *Select search-directory* field, browse to the **`Examples/`** folder inside this workspace, e.g.:

   ```
   C:\D\MY\DEV\TI-CCS-IDE\Workspace_12\Examples
   ```

3. CCS will scan the folder and list all projects under `Examples/`.

   **Projects:**

    - `TIRTOS_AM3352_GPIO_LED` — SYS/BIOS blinky: toggle D2 (GPIO1[23]) every 500 ms via `Task_sleep()`
    - `TIRTOS_AM3352_GPIO_LED_SEQUENCE` — SYS/BIOS 4-LED sweeping sequence: ping-pong pattern across D2–D5 (GPIO1[21]–GPIO1[24])
    - `TIRTOS_AM3352_GPIO_INTERRUPT` — SYS/BIOS GPIO interrupt demo: LED D2 blink speed controlled by push button on P9_12
    - `TIRTOS_AM3352_UART_ECHO` — UART echo via SYS/BIOS Task: reads characters and echoes back with `echo> ` prefix
    - `TIRTOS_AM3352_I2C_SCANNER` — I2C1 bus scanner: probes addresses 0x03–0x77, prints `i2cdetect`-style grid over UART0
    - `TIRTOS_AM3352_I2C_SSD1306` — SSD1306 128×32 OLED LCD driver over I2C1 @ 100 kHz via PDK I2C
    - `TIRTOS_AM3352_SPI_TX` — SPI0 TX with hardware CS: toggles DC/RST GPIOs and sends bytes via SPI0 @ 100 kHz

4. **Leave "Copy projects into workspace" UNCHECKED.** The projects in this repo are already self-contained and portable — copying them into a separate workspace folder defeats the purpose.

5. Click **Finish**.

Each project should appear in the Project Explorer with its own source files and a working build configuration.

### 5.2 Resolve build variables

If the project shows build errors immediately after import, the build variables may need to be resolved:

1. Right-click the imported project → **Properties → CCS Build → Variables**
2. Ensure these variables are defined (they should be auto-discovered):

   | Variable | Expected value |
   |---|---|
   | `PDK_INSTALL_PATH` | `C:/ti/pdk_am335x_1_0_17/packages` |
   | `BIOS_INSTALL_DIR` | `C:/ti/bios_6_76_03_01/packages` |
   | `XDC_INSTALL_DIR` | `C:/ti/xdctools_3_55_02_22_core` |
   | `GCC_ARM_TOOLCHAIN` | `C:/ti/gcc-arm-none-eabi-7-2018-q2-update` |
   | `TOOLCHAIN_PATH_arm` | `${GCC_ARM_TOOLCHAIN}` |

3. If any variable is missing, add it manually.

---

## 6. Build & run a sample

Let's use `TIRTOS_AM3352_GPIO_LED` as the canonical sanity check:

### 6.1 Build

1. Right-click `TIRTOS_AM3352_GPIO_LED` → **Build Project**.
2. The build process:
   - XDCtools processes `am335x_app_bbbam335x.cfg` → generates `configPkg/` with C sources
   - GCC ARM compiles `main_led_blink.c` + generated config sources
   - Linker produces the `.out` file

### 6.2 Debug

1. Connect your J-Link (or XDS debugger) to the Antminer L3+ JTAG header.
2. Right-click `TIRTOS_AM3352_GPIO_LED` → **Debug As → Code Composer Debug Session**.
3. CCS will load the `.out` file and halt at `main()`. Press **F8** (Resume) to run.
4. LED **D2** on the board should start blinking every 500 ms. 🎉

### 6.3 Custom target configuration

If the launch configuration doesn't match your debug probe:

1. Open the `targetConfigs/` folder in the project → double-click the `.ccxml` file.
2. In the **Target Configuration** editor, set your debug probe:
   - **Connection:** J-Link / XDS100v2 / XDS200
   - **Device:** AM3352 (or AM3359 — both work for the Cortex-A8 core)
3. **Setup → GEL file:** browse to `C:\ti\ccs1281\ccs\ccs_base\emulation\boards\beaglebone\gel\beagleboneblack.gel`
4. Save and re-launch.

---

## Troubleshooting

### ❌ "Cannot find file `Board.h`" (or similar PDK header)

- Make sure Processor SDK RTOS is installed and the `PDK_INSTALL_PATH` variable points to `C:/ti/pdk_am335x_1_0_17/packages`.
- If you installed the SDK elsewhere, update the variable in each project's build properties.

### ❌ "Cannot open config package" / XDCtools error

- Verify `XDC_INSTALL_DIR` and `BIOS_INSTALL_DIR` are set correctly.
- Make sure XDCtools version 3.55.2.22_core is installed. Newer XDCtools may introduce incompatible schema changes.
- Try cleaning the project (Project → Clean) and rebuilding.

### ❌ Linker error: undefined reference to `Board_initGPIO` or similar

- The PDK board library needs to be built for your specific board variant.
- Open the Board.syscfg or check the PDK build configuration. For custom boards without a PDK board package, you may need to provide a `Board_BBB.c` or similar board-specific init file.

### ❌ "Workspace already in use" / `.metadata` lock

- Another CCS instance is running against the same workspace folder, OR a previous CCS crashed and left `workspace/.metadata/.lock` behind. Close all CCS instances, then delete `.metadata/.lock` if present.

### ❌ "Project already exists" when re-importing

- You previously imported the project but didn't check the box to **delete from disk**. The `.project` file is still in the workspace folder. Either:
  - Use **Project → Existing Eclipse Projects** instead, or
  - Manually delete the `<project>/.project`, `<project>/.cproject`, `<project>/.ccsproject` files from inside the project folder (safe — they regenerate on import).

### ❌ Board doesn't respond to debugger

- Verify the JTAG connection is plugged into the correct header on the Antminer L3+.
- Verify the Antminer L3+ is powered externally (the JTAG does not power the board).
- For the target configuration, ensure the GEL file matches AM3352.
- If using J-Link, make sure the J-Link drivers are installed and the probe is recognized by CCS.

### ❌ XDCtools fails with `js: "xdc.tools.configuro... Error: Can't find platform package`

- The project's `.cfg` file references a platform (e.g., `ti.platforms.sitara`). Make sure the SYS/BIOS installation is complete and the platform packages are available under `bios_6_76_03_01/packages/ti/platforms/`.

---

## What you should have on disk when done

```
C:\ti\
├── ccs1281\                                    ← Code Composer Studio
├── pdk_am335x_1_0_17\                          ← PDK (board + drivers)
├── bios_6_76_03_01\                            ← SYS/BIOS kernel
├── xdctools_3_55_02_22_core\                   ← XDCtools
└── gcc-arm-none-eabi-7-2018-q2-update\         ← GCC ARM compiler

<your workspace>\
├── .metadata\                                  ← CCS workspace state (auto-generated)
├── RemoteSystemsTempFiles\                     ← RSE cache (auto-generated)
├── README.md
├── INSTALL.md                                  ← you are here
├── .gitignore
├── Doc\
│   └── bg.png                                  ← banner image
└── Examples\                                   ← portable CCS projects
    └── TIRTOS_AM3352_GPIO_LED\                 ← SYS/BIOS GPIO blinky
```

---

<p align="center"><sub>Happy hacking on the Antminer L3+ — for fun and profit.</sub></p>
