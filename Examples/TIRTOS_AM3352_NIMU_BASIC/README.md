# TIRTOS AM3352 NIMU ETHERNET

SYSBIOS (TI-RTOS) project for **AM3352** (BeagleBone Black class) — NIMU Ethernet application using CPSW (Complex Packet Switching Wrapper) switch.

## Architecture

- **Task `simple_task`** — main application task, prints a banner message with the assigned IP address after the NIMU/NDK stack initializes the Ethernet interface
- **Task `EmacStats` (app_stats)** — background statistics task running at priority 9, periodically prints RX/TX Ethernet frame statistics (good frames, broadcast, multicast, errors, collisions) via UART
- **`Board_init()`** — initializes pinmux, module clock & UART stdio via Board driver
- **NIMU (Network Interface Middleware Unit)** — TI's network interface abstraction layer that bridges the NDK TCP/IP stack with the EMAC/CPSW driver
- **EMAC Driver (V4)** — TI PDK EMAC driver for CPSW with PHY configuration (MII mode, full-duplex, PHY address 0)
- **SYSBIOS** — real-time kernel (`BIOS_start()`) with Cache & MMU configuration for AM335x

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point, board/EMAC init, task creation, NIMU device table |
| `GPIO_soc.c` | GPIO hardware attributes (AM335x SOC layer) |
| `I2C_soc.c` | I2C hardware attributes (AM335x SOC layer) |
| `UART_soc.c` | UART hardware attributes (AM335x SOC layer) |
| `nimu_osal.c` | OS Abstraction Layer — task sleep/create, memory alloc/free for NIMU |
| `statistics.c` | Ethernet statistics task — prints RX/TX counters every 10 seconds |
| `nimu_bbbam335x.cfg` | SYSBIOS configuration (NDK stack, NIMU, platform) |

## Toolchain

- **Processor SDK RTOS AM335x (PDK)** — [v1.0.17](https://www.ti.com/tool/download/PDKAM335X/1.0.17)
- **Code Composer Studio** — [v12.8.1](https://www.ti.com/tool/download/CCSTUDIO/12.8.1)
- **Compiler** — GCC ARM 7.3.1 (`gcc-arm-none-eabi-7-2018-q2-update`)
- **XDCtools** — v3.55.2.22_core
- **SYSBIOS** — v6.76.3.01
- **NDK** — v3.61.01.01

## Build & Run

1. Build via CCS — Configuration **Debug**
2. Debug using **JLink** with target configuration:
   - Device: `AM3352` / `AM3359` (Cortex-A8)
   - CCXML: use target configuration with JLink
   - GEL file: `C:\ti\ccs1281\ccs\ccs_base\emulation\boards\beaglebone\gel\beagleboneblack.gel`
3. Open serial terminal (e.g. PuTTY, TeraTerm) — 115200 baud
4. Configure static IP in `nimu_bbbam335x.cfg`:
   ```c
   Ip.address = "192.168.0.130";
   Ip.mask    = "255.255.255.0";
   Ip.gatewayIpAddr = "192.168.0.1";
   ```
5. Expected output:
   ```
   ========================================
     AM3352 NIMU Ethernet Application
     Board: AM3352
     Interface: CPSW (MII mode)
     IP Address: 192.168.0.130
   ========================================
   ```
6. Ping test from Windows host:
   ```
   Pinging 192.168.0.130 with 32 bytes of data:
   Reply from 192.168.0.130: bytes=32 time=1ms TTL=255
   Reply from 192.168.0.130: bytes=32 time=2ms TTL=255
   ```
7. Ethernet statistics print every 10 seconds (counters increment after traffic):
   ```
   --------------------------------------------------------------------------------------
    RX |     Good:       5 |   Bcast:       1 |    Mcast:       0 |    Oct:         376 |
    RX |    Pause:       0 |     CRC:       0 | AlignErr:       0 | Oversz:           0 |
    RX |   Jabber:       0 | Undersz:       0 |     Frag:       0 |   Filt:           0 |
    RX |      QoS:       0 |  SOFOvr:       0 |   MOFOvr:       0 | DMAOvr:           0 |
    TX |     Good:       5 |   Bcast:       0 |    Mcast:       0 |    Oct:         376 |
    TX |    Pause:       0 | Deferred:      0 |     Coll:       0 |   Udrn:           0 |
   --------------------------------------------------------------------------------------
   ```

## Key APIs

| API | Purpose |
|-----|---------|
| `Board_init(cfg)` | Initialize board pinmux, clocks, UART |
| `SOCCtrlCpswPortMacModeSelect(port, mode)` | Configure CPSW port MAC interface type (MII/RMII) |
| `EMAC_socGetInitCfg()` / `EMAC_socSetInitCfg()` | Get/set EMAC hardware attributes (PHY addr, duplex) |
| `NIMUDeviceTable[]` | Registry of NIMU device init callbacks |
| `CpswEmacInit()` | NIMU driver initialization callback for CPSW |
| `emac_get_statistics()` | Retrieve RX/TX frame counters from EMAC driver |
| `Task_create()` | SYSBIOS task creation for application tasks |
| `BIOS_start()` | Start SYSBIOS scheduler (never returns) |

## Notes

- CPSW configured in **MII mode** for both ports
- PHY address set to **0** for both ports (adjust if hardware differs)
- Full-duplex mode forced via `EMAC_CPSW_CONFIG_MODEFLG_FULLDUPLEX`
- PHY auto-negotiation: **100 Mbps Full-Duplex** (advertises 10/100/1000)
- Statistics task runs at priority 9 (higher than main task at priority 1)
- IP address configured as **static** in `nimu_bbbam335x.cfg` (not DHCP)
- Change `Ip.address`, `Ip.mask`, and `Ip.gatewayIpAddr` in the cfg file to match your network subnet
