# Multi-Core LED Blinky - AURIX TC397 TFT Kit

## Overview

This application demonstrates basic GPIO control and multi-core programming on the AURIX TC397 TFT Development Kit.

Each TriCore CPU controls a dedicated LED and toggles it at a different rate using the STM timer.

| Core | LED  | GPIO Pin | Blink Period |
| ---- | ---- | -------- | ------------ |
| CPU0 | D107 | P13.0    | 400 ms       |
| CPU1 | D108 | P13.1    | 350 ms       |
| CPU2 | D109 | P13.2    | 300 ms       |
| CPU3 | D110 | P13.3    | 250 ms       |

---

## Features

* GPIO output configuration using iLLD
* Multi-core synchronization using `IfxCpu_syncEvent`
* Independent LED control on four CPU cores
* Delay generation using STM timer services

---

## Project Structure

```text
├── Cpu0_Main.c
├── Cpu1_Main.c
├── Cpu2_Main.c
├── Cpu3_Main.c
├── Blinky_LED.c
└── Blinky_LED.h
```

---

## Software Design

### LED Initialization

The function `initLED()` configures a GPIO pin as a push-pull output and turns the LED off initially.

```c
void initLED(Ifx_P *port, uint8 pinIndex);
```

### LED Blinking

The function `blinkLED()` toggles the selected LED and waits for the specified delay.

```c
void blinkLED(Ifx_P *port, uint8 pinIndex, uint32 delay);
```

---

## LED Definitions

```c
#define LED_D107    &MODULE_P13,0
#define LED_D108    &MODULE_P13,1
#define LED_D109    &MODULE_P13,2
#define LED_D110    &MODULE_P13,3
```

---

## Core Execution

### CPU0

Initializes and blinks LED D107 every 400 ms.

```c
initLED(LED_D107);

while(1)
{
    blinkLED(LED_D107, 400);
}
```

### CPU1

Initializes and blinks LED D108 every 350 ms.

```c
initLED(LED_D108);

while(1)
{
    blinkLED(LED_D108, 350);
}
```

### CPU2

Initializes and blinks LED D109 every 300 ms.

```c
initLED(LED_D109);

while(1)
{
    blinkLED(LED_D109, 300);
}
```

### CPU3

Initializes and blinks LED D110 every 250 ms.

```c
initLED(LED_D110);

while(1)
{
    blinkLED(LED_D110, 250);
}
```

---

## Expected Result

After programming the TC397 TFT board:

* D107 blinks every 400 ms
* D108 blinks every 350 ms
* D109 blinks every 300 ms
* D110 blinks every 250 ms

The LEDs operate independently on separate TriCore CPUs, demonstrating concurrent execution across multiple cores.

---

## Hardware

* Board: AURIX TC397 TFT Development Kit
* MCU: Infineon TC397XA
* Toolchain: TASKING VX
* SDK: Infineon iLLD

---

## Learning Objectives

* Configure GPIO pins using iLLD
* Use STM-based delays
* Understand multi-core startup and synchronization
* Execute independent tasks on multiple TriCore CPUs
