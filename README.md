# Minimal RTOS Kernel — Version 0 (Proof of Concept)

Version 0 is the absolute bare-minimum proof-of-concept to get context switching working on an ARM Cortex-M4 (STM32F4). 

The goal here wasn't to write a production-ready scheduler, but to understand how the Cortex-M hardware exception frame works and manually manipulate the stack pointer to alternate execution between two functions.

---

## How It Works

1. **Memory Allocation**: Two static global arrays (`arr1[40]` and `arr2[40]`) act as raw stack buffers for two tasks (`blinken` and `blinkoff`). A 2-element array (`exc_add`) tracks the saved stack pointer (`MSP`) for each task.
2. **Bootstrapping (`fake_stack`)**: Before starting the scheduler, `fake_stack()` manually constructs a minimal Cortex-M hardware exception frame inside `arr1` by pushing `xPSR` (with the Thumb bit set) and the function pointer (`PC = &blinken`).
3. **Preemption & Switching**: 
   * The SysTick timer fires periodically (100 Hz).
   * Inside `SysTick_Handler`, it calls `context_switch()`.
   * `context_switch()` saves the current Main Stack Pointer (`MSP`) into `exc_add[i]`, updates the active index `i = (i + 1) % mx_thread`, loads the next thread's saved `MSP` from `exc_add[i]`, and returns using `EXC_RETURN` (`0xFFFFFFF9`).

---

## Architecture

![Version 0 Architecture](assets/v0architecture.jpg)

---

## Features

* **Barely works with 2 lightweight threads**: Alternates execution between `blinken()` (LED ON) and `blinkoff()` (LED OFF).
* **Static Memory Allocation**: Uses static RAM arrays instead of heap/dynamic memory.
* **Basic SysTick Preemption**: Relies on SysTick interrupts to drive round-robin context switches.

---

## The 5 Biggest Bugs & Architectural Problems

While the LED blinks, Version 0 has severe architectural flaws that make it unusable for real applications. These are the main issues identified for future versions:

### 1. Missing Callee-Saved Register Handling ($R4–R11$)
The hardware exception entry automatically saves $R0–R3, R12, LR, PC,$ and $xPSR$. However, Version 0 **completely ignores $R4–R11$**. If local variables or loops inside `delay()` assign values to $R4–R11$, those registers get overwritten during a context switch, causing subtle data corruption when returning to the previous task.

### 2. Stack Pointer Pointing to Global RAM / Memory Corruption
In early iterations, setting `MSP` to global pointers (`&exc_add`) caused stack `push` operations to write directly into `.bss` RAM, overwriting surrounding global variables (`i` and `mx_thread`). Even with `arr1`/`arr2`, there is zero stack overflow protection, and changing `MSP` inside a normal C function (`context_switch()`) risks compiler-generated stack frame collisions.

### 3. Tasks Modifying Internal Scheduler State (`i`)
Both task functions (`blinken()` and `blinkoff()`) manually execute `i = 0;` and `i = 1;` inside their execution loops. Tasks should never touch private scheduler indices. This corrupts scheduler tracking and causes stack pointers to be saved into the wrong index of `exc_add[]`.

### 4. Uninitialized Hardware Exception Frame Registers
`fake_stack()` only pushes `xPSR` and `PC`, leaving $R0–R3, R12,$ and $LR$ as uninitialized garbage RAM values. If `blinken()` or `blinkoff()` ever returns or exits its infinite loop, $LR$ points to a random address, causing an instant HardFault.

### 5. Lack of Process Stack Pointer (PSP) Task Isolation
Running user tasks directly on the Main Stack Pointer (`MSP`) mixes interrupt exception frames with task execution stacks. Standard Cortex-M RTOS design requires tasks to run on the Process Stack Pointer (`PSP`) so kernel interrupts remain isolated on `MSP`. Using `MSP` for user tasks prevents stack protection and violates standard ARM exception return conventions (`EXC_RETURN` `0xFFFFFFFD`).