# AURIX TC397 TFT Development Kit – Board Specifications

## Core Information

| Feature | Specification |
|----------|-------------|
| Microcontroller | Infineon AURIX TC397XA |
| CPU Cores | 6 × TriCore™ cores |
| Max Frequency | Up to 300 MHz |
| Architecture | 32-bit TriCore™ |
| Safety | ASIL-D capable |

---

## Memory

| Memory Type | Size |
|------------|------|
| Program Flash (PFlash) | 16 MB |
| Data Flash (DFlash) | 1 MB |
| LMU RAM | 6 MB |
| Core Local RAM | DSPR / PSPR per core |

---

## GPIO / Ports

The TC397 exposes multiple GPIO ports. Commonly available ports on the TFT kit include:

### Port Groups

- P00, P01, P02
- P10, P11, P12, P13, P14, P15
- P20, P21, P22, P23, P24, P25, P26
- P30, P31, P32, P33, P34
- P40, P41

Each port typically contains up to 16 GPIO pins.

Example:

```text
P33.1 = Port 33, Pin 1
P13.0 = Port 13, Pin 0
```

---

## On-Board LEDs

| LED | MCU Pin |
|------|---------|
| D107 | P13.1 |
| D108 | P13.2 |
| D109 | P13.3 |
| D110 | P13.4 |

---

## Display

| Feature | Description |
|----------|-------------|
| TFT LCD | Integrated |
| Touch Interface | Available |
| LVDS Display Support | Supported |

---

## Communication Interfaces

| Interface | Description |
|------------|-------------|
| CAN / CAN FD | Multiple nodes |
| Ethernet | 10/100 Mbps |
| QSPI | High-speed SPI |
| ASCLIN | UART / LIN |
| I²C | Supported |
| SENT | Automotive sensor interface |

---

## Debug & Programming

- On-board MiniWiggler / DAS debugger
- USB programming and debugging
- JTAG / DAP support

---

## Power

| Feature | Value |
|----------|-------|
| Supply Input | 12 V DC |
| USB Power | Supported for debugging |
| On-board Regulators | 3.3 V / 5 V rails |

---

## Useful Notes

- Designed for automotive and real-time embedded applications.
- GPIOs are referenced as `Pxx.y` where:
  - `xx` = Port number
  - `y` = Pin number

Example:

```text
P33.1 = Port 33, Pin 1
P13.0 = Port 13, Pin 0
```

- Fully supported by:
  - AURIX Development Studio (ADS)
  - Infineon iLLD (Infineon Low-Level Drivers)

---

## Quick Summary

| Specification | Value |
|--------------|-------|
| CPU | 6 × TriCore™ @ 300 MHz |
| Program Flash | 16 MB |
| Data Flash | 1 MB |
| RAM (LMU) | 6 MB |
| GPIO Ports | P00–P41 |
| LEDs | 4 User LEDs (D107–D110) |
| Ethernet | 10/100 Mbps |
| CAN FD | Supported |
| Debugger | On-board MiniWiggler |
| Display | Integrated TFT LCD |