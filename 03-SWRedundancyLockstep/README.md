# AURIX TC397 — Lockstep & Dual-Core Redundancy
### Hardware Lockstep vs Software Redundancy — Complete Guide

---

## Table of Contents
1. [What is Lockstep?](#what-is-lockstep)
2. [Hardware Lockstep on TC397](#hardware-lockstep-on-tc397)
3. [Software Redundancy — What We Built](#software-redundancy--what-we-built)
4. [Full Source Code](#full-source-code)
5. [Execution Flow](#execution-flow)
6. [Fault Injection — How to Trigger Failures](#fault-injection--how-to-trigger-failures)
7. [What Real Hardware Lockstep Detects](#what-real-hardware-lockstep-detects)
8. [Hardware vs Software Comparison](#hardware-vs-software-comparison)
9. [Debugging Multi-Core on ADS](#debugging-multi-core-on-ads)

---

## What is Lockstep?

Lockstep is a **fault detection technique** where two processing units run the **exact same instructions simultaneously** and their outputs are compared every cycle. If the outputs ever differ, a fault is detected and a safety reaction is triggered.

It is a core requirement in **ISO 26262 (Automotive Functional Safety)** for achieving **ASIL-D** — the highest automotive safety integrity level.

```
Core A  ──►  [instruction 1]  [instruction 2]  [instruction 3]
                    ↓                ↓                ↓
              COMPARE ──────── COMPARE ──────── COMPARE
                    ↑                ↑                ↑
Core B  ──►  [instruction 1]  [instruction 2]  [instruction 3]
```

If **any** comparison fails → **fault detected → safety reaction**.

---

## Hardware Lockstep on TC397

On the TC397, lockstep is implemented **entirely in silicon** — it is not a software feature. Each CPU has a dedicated **shadow lockstep core** built into the chip:

```
┌─────────────────────────────────────┐
│           TC397 Silicon             │
│                                     │
│  CPU0  ──►  [runs your code]        │
│               ↓ hardware comparator │
│  CPU0_LS ──►  [mirror core, hidden] │
│                                     │
│  CPU1  ──►  [runs your code]        │
│               ↓ hardware comparator │
│  CPU1_LS ──►  [mirror core, hidden] │
│                                     │
│  CPU2  ──►  [runs your code]        │
│               ↓ hardware comparator │
│  CPU2_LS ──►  [mirror core, hidden] │
└─────────────────────────────────────┘
```

### Key Facts About Hardware Lockstep on TC397

- The lockstep shadow cores are **invisible to software** — you cannot address, program, or read them
- They run **automatically** — no configuration needed in your application code
- Comparison happens at **hardware speed**, every single clock cycle
- On mismatch → **SMU (Safety Management Unit)** triggers a trap, reset, or safe state
- This is what achieves the TC397's **ASIL-D** rating out of the box
- You **cannot choose two arbitrary cores** (e.g. CPU0 + CPU1) and put them in hardware lockstep — each core's shadow is fixed in silicon

---

## Software Redundancy — What We Built

Since hardware lockstep is transparent to software, we implemented **software-based dual-core redundancy** to demonstrate the concept at the application level.

### Concept

Two cores (CPU0 and CPU1) independently run the **same computation** with the **same input**. CPU0 then compares both results. On mismatch → fault handler triggered.

This is called **software redundancy** or **dual-core diverse redundancy** and is used in:
- Application-level safety checks
- ASIL decomposition (splitting ASIL-C into two ASIL-A cores)
- Cross-checking sensor readings, CRC values, or algorithm outputs

### Architecture

```
        CPU0                              CPU1
          │                                │
    ──── SYNC BARRIER (once) ─────────────│
          │                                │
    [loop start]                     [loop start]
          │                                │
    reset flags                     wait for g_cpu0_done
          │                                │
    criticalComputation(100)        criticalComputation(100)
          │                                │
    g_cpu0_result = X               g_cpu1_result = X
    g_cpu0_done = 1                 g_cpu1_done = 1
          │                                │
    wait for g_cpu1_done            wait for g_cpu0_done to reset
          │
    compare results
    ┌─────────────────┐
    │ match?    ✅    │──► next iteration
    │ mismatch? ❌    │──► while(1)  [fault handler]
    └─────────────────┘
```

---

## Full Source Code

### `Cpu0_Main.c`

```c
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"

// Shared memory between cores in DSPR
volatile uint32 g_cpu0_result = 0;
volatile uint32 g_cpu1_result = 0;
volatile uint8  g_cpu0_done   = 0;
volatile uint8  g_cpu1_done   = 0;

IFX_ALIGN(4) IfxCpu_syncEvent cpuSyncEvent = 0;

// The "critical" computation both cores must run identically
static uint32 criticalComputation(uint32 input){
    uint32 result = 0;
    for(uint32 i = 0; i < input; i++)
        result += i * 3;
    return result;
}

void core0_main(void){
    IfxCpu_enableInterrupts();
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    // Sync both cores ONCE before starting
    IfxCpu_emitEvent(&cpuSyncEvent);
    IfxCpu_waitEvent(&cpuSyncEvent, 1);

    while(1){
        // Reset flags each iteration
        g_cpu0_done = 0;
        g_cpu1_done = 0;

        // CPU0 runs the computation
        g_cpu0_result = criticalComputation(100);
        g_cpu0_done   = 1;

        // Wait for CPU1 to finish
        while(g_cpu1_done == 0);

        // Compare results — lockstep check
        if(g_cpu0_result == g_cpu1_result){
            // Match — safe, continue next iteration
        }
        else{
            // Mismatch — fault detected, halt
            while(1);
        }
    }
}
```

### `Cpu1_Main.c`

```c
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"

// Access shared variables from CPU0
extern volatile uint8  g_cpu0_done;
extern volatile uint32 g_cpu1_result;
extern volatile uint8  g_cpu1_done;
extern IfxCpu_syncEvent cpuSyncEvent;

// The "critical" computation both cores must run identically
static uint32 criticalComputation(uint32 input){
    uint32 result = 0;
    for(uint32 i = 0; i < input; i++)
        result += i * 3;
    return result;
}

void core1_main(void){
    IfxCpu_enableInterrupts();
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());

    // Sync with CPU0 ONCE before starting
    IfxCpu_emitEvent(&cpuSyncEvent);
    IfxCpu_waitEvent(&cpuSyncEvent, 1);

    while(1){
        // Wait for CPU0 to signal computation started
        while(g_cpu0_done == 0);

        // CPU1 runs the SAME computation independently
        g_cpu1_result = criticalComputation(100);
        g_cpu1_done   = 1;

        // Wait for CPU0 to reset flags before next iteration
        // Prevents CPU1 overwriting result before CPU0 reads it
        while(g_cpu0_done == 1);
    }
}
```

### Why `volatile` on Shared Variables

The `volatile` keyword prevents the compiler from caching these variables in registers. Without it, each core would read a stale cached value and never see updates made by the other core — the spin-wait loops would never exit.

### Why the Third Wait in CPU1

```c
// Wait for CPU0 to reset flags before next iteration
while(g_cpu0_done == 1);
```

Without this, CPU1 could loop back immediately and overwrite `g_cpu1_result` with the **next iteration's value** before CPU0 finishes comparing the current one — causing a false mismatch or a missed fault. This wait ensures strict iteration synchronization.

---

## Execution Flow

```
Iteration N:

CPU0                                    CPU1
  │                                       │
  │ g_cpu0_done = 0                       │
  │ g_cpu1_done = 0                       │
  │                                       │ (waiting: while g_cpu0_done == 0)
  │ compute → g_cpu0_result = 14850       │
  │ g_cpu0_done = 1 ─────────────────────►│
  │                                       │ compute → g_cpu1_result = 14850
  │                                       │ g_cpu1_done = 1
  │◄───────────────────────── g_cpu1_done │
  │ compare: 14850 == 14850 ✅            │
  │                                       │ (waiting: while g_cpu0_done == 1)
  │ [loop back → reset flags]             │
  │ g_cpu0_done = 0 ─────────────────────►│ (exit wait)
  │                                       │ [loop back]
```

---

## Fault Injection — How to Trigger Failures

The example will **never fail on its own** because both cores run deterministic code with the same input. To test the fault detection path, inject faults manually:

---

### Method 1 — Bit Flip on Result (Simulate SEU)

In `Cpu0_Main.c`, flip a bit in the result before comparison:

```c
g_cpu0_result = criticalComputation(100);

/* Inject fault: flip bit 0 — simulate Single Event Upset */
g_cpu0_result ^= 0x01;

g_cpu0_done = 1;
```

**Result:** CPU0 result = `14851`, CPU1 result = `14850` → mismatch → `while(1)` fault handler.

---

### Method 2 — Wrong Input on One Core (Simulate Data Corruption)

Change the input on CPU1's computation only:

```c
// In Cpu1_Main.c
g_cpu1_result = criticalComputation(101);  // should be 100
```

**Result:** Different inputs → different outputs → mismatch detected.

---

### Method 3 — Wrong Algorithm on One Core (Simulate SW Bug)

Change the multiplier in CPU1's `criticalComputation`:

```c
// CPU1 version — bug injected
static uint32 criticalComputation(uint32 input){
    uint32 result = 0;
    for(uint32 i = 0; i < input; i++)
        result += i * 2;   // wrong: should be * 3
    return result;
}
```

**Result:** CPU0 = `14850`, CPU1 = `9900` → mismatch detected.

---

### Method 4 — Direct Memory Corruption (Simulate RAM Fault)

Corrupt the shared result variable directly after computation:

```c
// Anywhere in the loop after both cores finish:
g_cpu1_result = g_cpu0_result + 1;
```

**Result:** Forced mismatch — simulates a stuck bit in RAM.

---

### Method 5 — Conditional Fault (Simulate Intermittent Error)

Inject a fault only on specific iterations to simulate an intermittent hardware problem:

```c
static uint32 iterationCount = 0;
iterationCount++;

g_cpu0_result = criticalComputation(100);

/* Inject fault every 100th iteration */
if(iterationCount % 100 == 0)
    g_cpu0_result ^= 0xFF;

g_cpu0_done = 1;
```

---

### Fault Injection Summary

| Method | Simulates | Where to Add |
|---|---|---|
| `result ^= 0x01` | Single Event Upset (bit flip) | CPU0 after computation |
| Different input `(101)` | Data corruption / wrong sensor read | CPU1 computation call |
| Wrong multiplier `* 2` | Software logic bug | CPU1 algorithm |
| `g_cpu1_result += 1` | RAM stuck bit / memory fault | After both cores done |
| Periodic `^= 0xFF` | Intermittent hardware fault | CPU0, every N iterations |

---

## What Real Hardware Lockstep Detects

The TC397's hardware lockstep (silicon-level) detects faults that software redundancy **cannot**:

### 1. Single Event Upsets (SEU)
A cosmic ray or high-energy particle flips a single bit inside a CPU register or cache mid-instruction. The lockstep shadow core's register will still hold the correct value → mismatch → SMU fires instantly.

### 2. ALU Faults
The Arithmetic Logic Unit produces a wrong result (e.g. `3 + 4 = 8` instead of `7`) due to a transistor stuck at a fixed voltage. The shadow core's ALU still produces `7` → mismatch detected at that instruction.

### 3. Pipeline Faults
The instruction pipeline mis-stages an instruction (executes out of order or skips a stage). The shadow core's pipeline operates correctly → output diverges → detected.

### 4. Register File Corruption
A CPU register gets corrupted mid-computation by a power glitch or ESD event. The shadow register still holds the correct value → mismatch.

### 5. Cache Errors (with ECC support)
AURIX caches include **ECC (Error Correction Code)**. Single-bit errors are corrected silently; double-bit errors are detected and reported to the SMU — even without lockstep triggering.

### 6. Stuck-at Faults
A transistor permanently stuck at logic `0` or `1` causes one core's output to always deviate. Detected immediately on first divergence.

### 7. Timing Faults
Clock jitter or power supply noise causes a core to latch a wrong value at a clock edge. The shadow core, sharing the same clock domain, either also latches the wrong value (common cause — both fail together, not detected) or the fault is localized → detected.

> **Important:** Hardware lockstep does **not** detect common-cause failures — faults that affect both the main core and its shadow simultaneously (e.g. a power supply collapse affecting the whole die). This is why diverse redundancy across physically separate chips is used for the highest safety levels.

---

### SMU (Safety Management Unit) Reaction

When hardware lockstep detects a mismatch on TC397, the fault is reported to the **SMU**. The SMU can be configured to:

| Reaction | Description |
|---|---|
| **Trap** | CPU jumps to trap handler — software can log the fault and attempt recovery |
| **Reset** | System resets and restarts from a known-good state |
| **Safe State** | Drive outputs to a defined safe state (e.g. disable actuators) |
| **NMI** | Non-maskable interrupt fired to all cores |

---

## Hardware vs Software Comparison

| Feature | Hardware Lockstep (TC397 silicon) | Software Redundancy (our example) |
|---|---|---|
| Shadow core visibility | Invisible to software | CPU1 — fully programmable |
| Comparison granularity | Every instruction, every cycle | End of computation only |
| Detection latency | Immediate (same clock cycle) | End of each loop iteration |
| Overhead | Zero CPU overhead | CPU1 fully dedicated |
| Fault types detected | SEU, ALU, pipeline, register, stuck-at | Wrong result, wrong algorithm, data corruption |
| Common-cause immunity | No | Partial (different code paths possible) |
| ASIL achievable | ASIL-D | ASIL-B / ASIL-C (with diverse SW) |
| Configuration needed | None — always on | Full software implementation |
| Use case | Silicon-level safety guarantee | Application-level cross-checking |
