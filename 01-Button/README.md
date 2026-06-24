# GPIO LED Button - AURIX TC397 TFT Kit

## Overview

This application demonstrates basic GPIO input and output control on the AURIX TC397 TFT Development Kit.

A push button input is used to control an LED. When the button is pressed, the LED turns ON. When the button is released, the LED turns OFF.

---

## Features

* GPIO output configuration for LED control
* GPIO input configuration with internal pull-up resistor
* Real-time button state monitoring
* Simple polling-based application

---

## Hardware Connections

| Signal | GPIO Pin | Description          |
| ------ | -------- | -------------------- |
| LED    | P13.3    | User LED (D110)      |
| Button | P14.4    | Push button input    |
| Ground | P14.5    | Reference ground pin |

### Test Setup

Connect:

```text
P14.4 <--> P14.5
```

using a jumper wire.

Since P14.5 is driven LOW and P14.4 uses an internal pull-up resistor:

* Pins disconnected → Logic HIGH
* Pins connected → Logic LOW

---

## Project Structure

```text
├── Cpu0_Main.c
├── GPIO_LED_Button.c
└── GPIO_LED_Button.h
```

---

## Software Design

### GPIO Initialization

The function `init_GPIOs()` configures:

* LED pin as push-pull output
* Button pin as input with pull-up resistor
* Ground pin as push-pull output driven LOW

```c
void init_GPIOs(void);
```

---

### LED Control

The function `control_LED()` continuously checks the button state.

```c
void control_LED(void);
```

Behavior:

```text
Button Pressed   -> LED ON
Button Released  -> LED OFF
```

---

## GPIO Definitions

```c
#define LED     &MODULE_P13,3
#define BUTTON  &MODULE_P14,4
#define GROUND  &MODULE_P14,5
```

---

## Application Flow

```text
Start
  │
  ├─ Initialize GPIOs
  │
  └─ Infinite Loop
       │
       ├─ Read Button State
       │
       ├─ Pressed ?
       │      │
       │      ├─ Yes → Turn LED ON
       │      │
       │      └─ No  → Turn LED OFF
       │
       └─ Repeat
```

---

## Core Execution

The application runs on CPU0.

```c
init_GPIOs();

while(1)
{
    control_LED();
}
```

---

## Expected Result

| Button State                       | LED State |
| ---------------------------------- | --------- |
| Released                           | OFF       |
| Pressed (P14.4 connected to P14.5) | ON        |

---

## Hardware

* Board: AURIX TC397 TFT Development Kit
* MCU: Infineon TC397XA
* Toolchain: TASKING VX
* SDK: Infineon iLLD

---

## Learning Objectives

* Configure GPIO inputs and outputs using iLLD
* Use internal pull-up resistors
* Read digital input states
* Control GPIO outputs based on user input
* Understand basic embedded polling applications
