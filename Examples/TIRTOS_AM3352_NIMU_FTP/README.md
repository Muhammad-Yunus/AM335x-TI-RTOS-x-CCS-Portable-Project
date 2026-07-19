# TIRTOS AM3352 NIMU FTP SERVER

SYSBIOS (TI-RTOS) project for **AM3352** (BeagleBone Black class) — NIMU Ethernet + FTP Server application using CPSW (Complex Packet Switching Wrapper) switch.

## Architecture

- **Task `main_task` (pri 1)** — main application task, prints banner with assigned IP address then initializes FTP server via `ftpserver_init()`
- **Task `EmacStats` (pri 9)** — background statistics task running at priority 9, periodically prints RX/TX Ethernet frame counters (good frames, broadcast, multicast, errors, collisions) via UART every 10 seconds
- **Task `FTP Server` (pri 5)** — listens on port 21 for incoming FTP connections, spawns a client task per connection with 16KB stack
- **Task `FTP Client` (pri 5)** — handles individual FTP client sessions (command parsing, authentication, file transfer)
- **`Board_init()`** — initializes pinmux, module clock & UART stdio via Board driver
- **NIMU (Network Interface Middleware Unit)** — TI's network interface abstraction layer bridging NDK TCP/IP stack with EMAC/CPSW driver
- **EMAC Driver (V4)** — TI PDK EMAC driver for CPSW with PHY configuration (MII mode, full-duplex, PHY address 0)
- **FTP Server** — custom lightweight FTP server implementation supporting active mode (PORT) only
- **SYSBIOS** — real-time kernel (`BIOS_start()`) with Cache & MMU configuration for AM335x

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point, board/EMAC init, task creation, NIMU device table, FTP server init |
| `ftpserver.c` | FTP server core — socket listen/accept, client task spawn, command parser |
| `ftp_commands.c` | FTP command handlers — USER, PASS, STOR, RETR, DELE, CWD, XPWD, LIST, NLST, PORT, QUIT |
| `ftp_filerout.c` | File I/O routing — read/write data over FTP data socket |
| `GPIO_soc.c` | GPIO hardware attributes (AM335x SOC layer) |
| `I2C_soc.c` | I2C hardware attributes (AM335x SOC layer) |
| `UART_soc.c` | UART hardware attributes (AM335x SOC layer) |
| `nimu_osal.c` | OS Abstraction Layer — task sleep/create, memory alloc/free for NIMU |
| `statistics.c` | Ethernet statistics task — prints RX/TX counters every 10 seconds |
| `nimu_bbbam335x.cfg` | SYSBIOS configuration (NDK stack, NIMU, platform, MMU) |

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
   SYS/BIOS Ethernet/IP (CPSW) Sample application, EVM IP address: 192.168.0.130

   All tests have passed.
   ```
6. Connect via FTP client:
   ```
   Host: 192.168.0.130
   Port: 21
   Username: user
   Password: password
   Mode: Active (PORT) — PASV not supported
   ```

## FTP Server Capabilities

| Command | Status | Description |
|---------|--------|-------------|
| **USER** | Supported | Authentication username |
| **PASS** | Supported | Authentication password |
| **QUIT** | Supported | Disconnect client |
| **STOR** | Supported | Upload file (active mode only) |
| **RETR** | Supported | Download file (active mode only) |
| **DELE** | Stub | Delete file (returns 200 OK, not implemented) |
| **XRMD** | Stub | Remove directory (returns 200 OK, not implemented) |
| **CWD** | Stub | Change working directory (returns 550 failure) |
| **XPWD** | Supported | Print working directory |
| **LIST** | Stub | List files (returns empty) |
| **NLST** | Stub | Name list (returns empty) |
| **PORT** | Supported | Active mode data connection setup |
| **SYST** | Not supported | Returns 202 |
| **NOOP** | Not supported | Returns 202 |
| **TYPE** | Not supported | Returns 202 |
| **FEAT** | Not supported | Returns 202 |
| **PWD** | Not supported | Returns 202 |
| **PASV** | Not supported | Passive mode not available |

### FTP Server Details

- **Listen port:** 21
- **Listen timeout:** 10000 ms
- **Max pending connections:** 5
- **Server task stack:** 4096 bytes
- **Client task stack:** 16384 bytes
- **Authentication:** Hardcoded — username `user`, password `password` (see `ftpserver.c:204-205`)
- **Connection mode:** Active (PORT) only — passive (PASV) not supported
- **Transfer mode:** Binary (TYPE not implemented, assumes binary)
- **File storage:** Not persistent — file I/O routing (`ftp_filerout.c`) is a stub, data received/sent via socket only

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
| `ftpserver_init()` | Initialize FTP server task (listens on port 21) |

## Notes

- CPSW configured in **MII mode** for both ports
- PHY address set to **0** for both ports (adjust if hardware differs)
- Full-duplex mode forced via `EMAC_CPSW_CONFIG_MODEFLG_FULLDUPLEX`
- PHY auto-negotiation: **100 Mbps Full-Duplex** (advertises 10/100/1000)
- Statistics task runs at priority 9 (higher than main task at priority 1)
- IP address configured as **static** in `nimu_bbbam335x.cfg` (not DHCP)
- FTP server uses **active mode only** — PASV not supported; Windows firewall may block incoming PORT connections from the board
- FTP file I/O (`ftp_filerout.c`) is a **stub** — data transfer happens over socket but files are not persisted to storage
- Change `Ip.address`, `Ip.mask`, and `Ip.gatewayIpAddr` in `nimu_bbbam335x.cfg` to match your network subnet
