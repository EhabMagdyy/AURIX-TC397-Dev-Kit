# AURIX TC397 TFT — Ethernet Echo Application
### Direct Cable Connection (No Router) — Complete Guide

---

## Table of Contents
1. [Overview](#overview)
2. [Hardware Setup](#hardware-setup)
3. [How the Application Works](#how-the-application-works)
4. [Windows Configuration](#windows-configuration)
5. [Firmware Changes](#firmware-changes)
6. [Build and Flash](#build-and-flash)
7. [Testing the Connection](#testing-the-connection)
8. [Project File Structure](#project-file-structure)
9. [Extending the Example](#extending-the-example)

---

## Overview

This guide documents how to connect the **AURIX TC397 TFT Application Kit** to a Windows laptop via a direct Ethernet cable (no router), run the official `Ethernet_1_KIT_TC397_TFT` echo example, and verify two-way TCP communication.

| Item | Value |
|---|---|
| Board | KIT_A2G_TC397_5V_TFT |
| IDE | AURIX Development Studio (ADS) |
| TCP/IP Stack | LwIP (Lightweight IP) |
| Ethernet Driver | iLLD GETH (`IfxGeth_Eth`) |
| Protocol | TCP — Raw Echo Server on port 80 |
| TC397 IP | `192.168.1.10` |
| Laptop IP | `192.168.1.100` |

---

## Hardware Setup

### What You Need
- Standard RJ45 Ethernet patch cable (no crossover needed — Auto-MDIX handles it)
- USB cable (for flashing and UART debug output)
- Windows laptop with an Ethernet NIC

### Physical Connection

```
TC397 TFT Board
  [ETH Port]
      |
  [Ethernet Cable]
      |
  [Laptop Ethernet NIC]
```

> No router or switch needed. Direct cable works because modern NICs support Auto-MDIX (automatic crossover detection).

### Confirming Physical Link

After connecting the cable and powering the board, check **Control Panel → Network and Sharing Center → Ethernet Status**:

| Field | Expected Value |
|---|---|
| Media State | Enabled |
| Speed | 1.0 Gbps |

If Speed shows 1.0 Gbps, the physical link is confirmed good.

---

## How the Application Works

### Architecture

```
Your Application (Echo.c)
        ↓
    LwIP TCP/IP Stack       ← software, handles TCP/IP automatically
        ↓
    GETH iLLD Driver        ← hardware abstraction
        ↓
    TC397 GETH + PHY chip   ← physical Ethernet hardware on board
        ↓
    Ethernet Cable
        ↓
    Laptop
```

### Communication Flow

```
Laptop (PuTTY)                       TC397 Board
      |                                   |
      |  connects to 192.168.1.10:80 ──►  |  echoAccept() fires
      |  ◄─────────── Infineon Logo ────  |  tcp_write(logo)
      |                                   |
      |  "hello\n"  ──────────────────►   |  echoRecv() fires
      |                                   |  copies to storage[]
      |  ◄──────── "Board: hello\n" ────  |  echoSend() fires
```

### Key Source Files

| File | Purpose |
|---|---|
| `Cpu0_Main.c` | Hardware init, LwIP init, main polling loop |
| `Echo.c` | TCP echo server — all callback logic |
| `Echo.h` | Echo function declarations |
| `Configurations/lwipopts.h` | LwIP stack configuration macros |
| `Libraries/Ethernet/lwip/port/` | GETH ↔ LwIP glue driver |
| `Ifx_Lwip.h / Ifx_Lwip.c` | LwIP init and poll wrappers |

### Echo.c Callbacks Explained

| Callback | Trigger | Action |
|---|---|---|
| `echoAccept()` | Client connects | Allocates session, sends Infineon logo |
| `echoRecv()` | Data arrives | Copies data to `storage[256]` |
| `echoSend()` | Called after recv | Sends `"Board: " + storage` back |
| `echoSent()` | Data delivered | Checks for more data or closes |
| `echoPoll()` | Periodic TCP timer | Handles leftover data or closing |
| `echoClose()` | Session ends | Frees memory, closes TCP PCB |

### LwIP Timer Mechanism

The STM0 (System Timer Module) fires an ISR every 1 ms:

```c
void updateLwIPStackISR(void)
{
    IfxStm_increaseCompare(&MODULE_STM0, IfxStm_Comparator_0, IFX_CFG_STM_TICKS_PER_MS);
    g_TickCount_1ms++;
    Ifx_Lwip_onTimerTick();   // updates ARP, TCP, DHCP, LINK timers
}
```

The main loop polls LwIP continuously:

```c
while (1)
{
    Ifx_Lwip_pollTimerFlags();    // execute protocol handlers
    Ifx_Lwip_pollReceiveFlags();  // process incoming packets
}
```

---

## Windows Configuration

### Why Static IP is Needed

The default example uses DHCP to obtain an IP address. With a direct cable and no router, there is no DHCP server. The board gets `0.0.0.0` and communication fails.

**Symptom of DHCP failure (seen in UART terminal):**
```
netif: netmask of interface
netif: new ip address assigned: 0.0.0.0
```

**Solution:** disable DHCP in firmware and assign static IPs on both sides.

### Setting Static IP on the Laptop

1. Open **Control Panel → Network and Sharing Center → Change adapter settings**
2. Right-click your **Ethernet adapter** → **Properties**
3. Select **Internet Protocol Version 4 (TCP/IPv4)** → **Properties**
4. Select **"Use the following IP address"** and enter:

| Field | Value |
|---|---|
| IP address | `192.168.1.100` |
| Subnet mask | `255.255.255.0` |
| Default gateway | *(leave blank)* |

5. Click OK on all dialogs

---

## Firmware Changes

### Change 1 — Disable DHCP in `Configurations/lwipopts.h`

Find the DHCP macro and change it from `1` to `0`:

```c
// Before:
#define LWIP_DHCP    1

// After:
#define LWIP_DHCP    0
```

### Change 2 — Assign Static IP in `Cpu0_Main.c`

Add the include at the top if not already present:

```c
#include "lwip/netif.h"
```

Then add 5 lines right after `Ifx_Lwip_init(ethAddr)`:

```c
Ifx_Lwip_init(ethAddr);                    /* Initialize LwIP with the MAC address */

/* Static IP — no DHCP/router needed */
ip4_addr_t ipaddr, netmask, gw;
IP4_ADDR(&ipaddr,  192, 168, 1, 10);       /* TC397 board IP */
IP4_ADDR(&netmask, 255, 255, 255, 0);
IP4_ADDR(&gw,        0,   0, 0,  0);       /* no gateway for direct link */
netif_set_addr(&g_Lwip.netif, &ipaddr, &netmask, &gw);

echoInit();                                /* Initialize ECHO application */
```

> **Note:** The LwIP netif handle is `g_Lwip.netif` — verify the exact variable name in `Ifx_Lwip.h` if you get a compile error.

### Summary of IP Assignment

| Device | IP Address | Set Where |
|---|---|---|
| TC397 Board | `192.168.1.10` | `Cpu0_Main.c` (static, in firmware) |
| Laptop | `192.168.1.100` | Windows IPv4 NIC settings |

---

## Build and Flash

1. In ADS, right-click the project → **Build Project** (or click the Build button in the toolbar)
2. After successful build, click the **Flash** button to program the TC397
3. Open the **Serial Terminal** inside ADS (UART monitor icon in toolbar)
4. Serial terminal settings:

| Setting | Value |
|---|---|
| Baud rate | 115200 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |

5. Reset or power-cycle the board
6. Confirm this appears in the terminal:

```
netif: netmask of interface en set to 255.255.255.0
netif: new ip address assigned: 192.168.1.10
```

---

## Testing the Connection

### Step 1 — Ping from CMD

Open Windows Command Prompt and run:

```cmd
ping 192.168.1.10
```

Expected output:
```
Reply from 192.168.1.10: bytes=32 time<1ms TTL=255
Reply from 192.168.1.10: bytes=32 time<1ms TTL=255
```

If ping replies → Ethernet link is fully working.

### Step 2 — Connect with PuTTY

Download PuTTY from **https://www.putty.org** (single `.exe`, no install needed).

Configure PuTTY:

| Field | Value |
|---|---|
| Connection type | **RAW** (not SSH or Telnet) |
| Host Name | `192.168.1.10` |
| Port | `80` |

Click **Open**.

### Step 3 — Verify Echo

On connect, the board sends the Infineon ASCII logo. Then type any text and press Enter:

```
Board: hello
Board: this is working
```

Every line you send is echoed back prefixed with `Board: `.

---

## Project File Structure

```
Ethernet_1_KIT_TC397_TFT/
├── Configurations/
│   └── lwipopts.h              ← LwIP config macros (DHCP, TCP, memory)
├── Libraries/
│   └── Ethernet/
│       └── lwip/
│           ├── port/           ← GETH ↔ LwIP glue driver
│           │   ├── Ifx_Lwip.c
│           │   └── Ifx_Lwip.h
│           └── src/            ← LwIP core source (don't modify)
├── Cpu0_Main.c                 ← Hardware init, main loop ← MODIFIED
├── Echo.c                      ← TCP echo server logic
└── Echo.h                      ← Echo declarations
```

---

## Extending the Example

This echo server is the perfect base for real embedded communication. You only ever modify `Echo.c` — LwIP and the GETH driver stay untouched.

### Example: Command-Response Server

Replace the echo logic in `echoRecv()` with a command parser:

```c
if (strncmp(es->storage, "STATUS\n", 7) == 0)
{
    tcp_write(tpcb, "Board: READY\n", 13, 1);
}
else if (strncmp(es->storage, "OTA_START\n", 10) == 0)
{
    tcp_write(tpcb, "Board: OTA ACK\n", 15, 1);
    // trigger your OTA logic here
}
```

### Use Cases You Can Build From This Base

| Use Case | What to Change |
|---|---|
| Send sensor data to laptop | Replace echo send with periodic ADC readings |
| Remote command execution | Parse incoming strings as commands in `echoRecv()` |
| OTA firmware trigger | On specific command, initiate flash write sequence |
| DoIP (ISO 13400) | Replace LwIP raw API with DoIP protocol handler |
| SOME/IP | Add SOME/IP framing on top of the TCP layer |

---

*Generated from AURIX Development Studio — Ethernet_1_KIT_TC397_TFT example (V2.0.0)*
*Board: KIT_A2G_TC397_5V_TFT | TC39xXX_B-Step*