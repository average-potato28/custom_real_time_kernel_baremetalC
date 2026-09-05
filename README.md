# lite-rtos

A priority-preemptive real-time operating system kernel written from scratch in Embedded C and ARM assembly for Cortex-M4 (STM32F4). Zero vendor libraries, no CMSIS-RTOS layer, and no heap allocation.

---

## Origin

I started my embedded journey writing assembly on the 8085 and 8086, then moved to the 8051. Manually manipulating accumulators, status flags, and raw memory addresses gave me an appreciation for what hardware is actually doing underneath the abstractions. Over time, that fascination grew into an obsession with low-level programming: kernel internals, hardware frameworks, operating systems, and bare-metal control.

When I moved to Embedded C, I naturally gravitated toward state machines: finite state machines, switch-case dispatchers, event queues, and non-blocking timers. State machines are clean, but as soon as a state needs to wait on I/O or run a heavy calculation, you end up slicing functions into awkward sub-states with manual state variables. 

That was when I started digging into RTOS design. An RTOS kernel is essentially the ultimate state machine: instead of forcing the programmer to manually break up C functions into sub-states, the operating system saves the entire CPU register context onto a private stack and restores another.

Once that clicked, I did not want to just call `xTaskCreate()` in FreeRTOS and treat context switching as a black box. Driven by my obsession with OS internals and low-level mechanics, I decided to build a kernel from scratch, wire the hardware exception frames, write the ARM assembly register swaps, manage the stack pointers, and build the scheduler state machine myself.

This repository tracks that build across five iterations, from a broken 2-thread prototype (v0) to a modular kernel library (v4).

---

## Evolution: v0 to v4

Detailed notes and debugging logs from earlier iterations are archived in `docs/PROJECT_EVOLUTION.md`. Here is the progression.

### v0: Proof of concept 🙂
I had one goal: alternate execution between two C functions (`blinken` and `blinkoff`) anyhow. I allocated two static `uint32_t` arrays to act as stacks, manually set `xPSR` and `PC`, and triggered context switches inside `SysTick_Handler` by calling a C function to swap the Main Stack Pointer (`MSP`).

Failure points:
- The Cortex-M core only automatically pushes `r0-r3`, `r12`, `LR`, `PC`, and `xPSR` on exception entry. It leaves `r4-r11` in place. As soon as a task used local variables or compiler scratch registers, switching tasks corrupted them.
- Tasks directly modified the global thread index (`i = 0` and `i = 1`) inside their own loops.
- Swapping stack pointers inside a normal C function caused memory corruption because compiler-generated stack frames collided across task stacks.

### v1.0: Callee-Saved Register Preservation
Switched `SysTick_Handler` to a naked assembly function to explicitly push and pop the callee-saved software registers (`push {r4-r11}` / `pop {r4-r11}`). 

Failure points:
- Stack pointer swapping was still delegated to a C function (`context_switch()`). When compiled with `-O2` or `-O3`, GCC placed function prologue/epilogue code on the stack, causing stack frame misalignment on return.
- User tasks still ran on `MSP`, sharing memory space directly with interrupt handlers not good 🙏.

### v2: Pure Assembly Context Switcher
Eliminated the C function during switching entirely. Handled the entire context switch in pure assembly inside `SysTick_Handler`. Scaled the system to 5 round-robin tasks and added USART1 output to monitor execution state.

Failure points:
- Tasks still executed on `MSP`. A task stack overflow corrupted kernel interrupt frames.
- Context switching ran synchronously inside `SysTick_Handler`. If a time-critical peripheral interrupt arrived while SysTick was swapping registers, it had to wait or we had to reserve SysTick for OS only not ggood.
- Task delays relied on busy-wait loops (`__NOP()`), keeping the core at 100% load even if no operation being performed.

### v3: Dual Stacks, PendSV, and O(1) Bitmask Scheduling
A complete architectural redesign aligning with ARM Cortex-M design specifications:
- Process Stack Pointer (**PSP**): User tasks execute on PSP. Interrupt service routines and kernel exceptions execute on the Main Stack Pointer (**MSP**).
- Asynchronous **PendSV**: `SysTick_Handler` only updates software timers and sets the `PENDSVSET` bit in `SCB->ICSR`. The actual context switch occurs inside `PendSV_Handler`, which runs at the lowest interrupt priority (priority 15). Peripheral ISRs preempt the scheduler without latency.
- Bitmask Scheduling: Replaced linear array scanning with a ready bitmask (`ready_reg`). Task selection uses `__builtin_ctz` (Count Trailing Zeros), resolving the highest-priority ready thread in a single CPU instruction.
- Cold Boot via `SVC 0`: Instead of jumping to task 0 with a function call, the kernel boots the first task via a Supervisor Call exception, returning through `0xFFFFFFFD` to enter Thread Mode on PSP this eliminated the branch instruction inside Pendsv.
- Non-Blocking Sleep and Idle State: Added `rtos_delay()`. Tasks yield their ready bit and sleep. When all tasks sleep, the CPU enters an internal idle thread executing `wfi` (Wait For Interrupt).

### v4: Kernel Modularization (Current)
Version 3 had scheduler code was married with board-specific peripheral initialization. Version 4  decouples the kernel into an independent library:
- Public API headers: `inc/lite_rtos.h` and `inc/lite_rtos_config.h`. The internal TCB struct (`OS_boy`) is encapsulated behind forward declarations.
- Architecture-independent kernel: `src/lite_rtos.c` handles task creation, ready bitmasks, software timers, and kernel startup.
- Hardware port: `src/lite_rtos_port.c` contains the naked assembly handlers (`SVC_Handler` and `PendSV_Handler`).
- User tasks now live in application code (`main.c`).

---

## Technical Architecture

### Context Stack Frame

Each task in LiteRTOS has a statically allocated stack buffer. When suspended, the stack holds exactly 16 words (64 bytes):

```text
Higher Memory Addresses
+-------------------------------+
| xPSR (bit 24 set for Thumb)   | 0x3C  <-- Hardware auto-stacked on exception entry
| PC   (Task entry point)       | 0x38  <-- Hardware auto-stacked
| LR   (0x00000000)             | 0x34  <-- Hardware auto-stacked
| R12                           | 0x30  <-- Hardware auto-stacked
| R3                            | 0x2C  <-- Hardware auto-stacked
| R2                            | 0x28  <-- Hardware auto-stacked
| R1                            | 0x24  <-- Hardware auto-stacked
| R0                            | 0x20  <-- Hardware auto-stacked
+-------------------------------+ <--- PSP after exception entry
| R11                           | 0x1C  <-- Software saved via stmdb in PendSV
| R10                           | 0x18  <-- Software saved via stmdb
| R9                            | 0x14  <-- Software saved via stmdb
| R8                            | 0x10  <-- Software saved via stmdb
| R7                            | 0x0C  <-- Software saved via stmdb
| R6                            | 0x08  <-- Software saved via stmdb
| R5                            | 0x04  <-- Software saved via stmdb
| R4                            | 0x00  <-- Software saved via stmdb
+-------------------------------+ <--- Saved into TCB->sp
Lower Memory Addresses (Stack grows downward)
```

`create_thread()` synthesizes this initial frame in RAM. It sets `xPSR` to `0x01000000` (Thumb state bit) and places the task function pointer in `PC`.

### PendSV Context Switcher

The context switch engine in `src/lite_rtos_port.c`:

```c
__attribute__((naked)) void PendSV_Handler(void) {
    __asm volatile (
        "mrs r0, psp            \n" // Read active task PSP
        "ldr r1, =prev          \n" // Pointer to previous TCB
        "ldr r1, [r1]           \n"
        "stmdb r0!, {r4-r11}    \n" // Push r4-r11 to current task stack
        "str r0, [r1]           \n" // Update previous TCB->sp
        
        "ldr r1, =curr          \n" // Pointer to next ready TCB
        "ldr r1, [r1]           \n"
        "ldr r0, [r1]           \n" // Load next task stack pointer
        "ldmia r0!, {r4-r11}    \n" // Pop r4-r11 from next task stack
        "msr psp, r0            \n" // Update hardware PSP
        "bx lr                  \n" // Return using EXC_RETURN (0xFFFFFFFD)
        ::: "memory"
    );
}
```

The exception return instruction `bx lr` loads the `EXC_RETURN` value `0xFFFFFFFD`. This signals the Cortex-M processor to:
1. Return to Thread Mode.
2. Select PSP as the active stack pointer.
3. Automatically unstack `r0-r3`, `r12`, `LR`, `PC`, and `xPSR`.

---

## File Structure

```text
.
├── inc/
│   ├── lite_rtos.h          Public kernel API
│   └── lite_rtos_config.h   Configurable constants (MAX_THREADS, stack_size)
├── src/
│   ├── lite_rtos.c          Kernel implementation (scheduler, timers, idle thread)
│   └── lite_rtos_port.c     Hardware port (SVC_Handler, PendSV_Handler)
├── docs/
│   ├── PROJECT_EVOLUTION.md Engineering notes and retrospectives across v0-v3
│   └── bts-artifacts/       Oscilloscope captures and development artifacts
├── assets/                  System architecture diagrams
├── LICENSE                  GPLv3
└── README.md
```

---

## How to use this lib:

### 1. Configure the Kernel

Edit `inc/lite_rtos_config.h`:

```c
#define MAX_THREADS 5   // Number of user threads (priority 0 to MAX_THREADS - 1)
#define stack_size  80  // Stack depth per thread in 32-bit words (80 * 4 = 320 bytes)
```

The kernel allocates `MAX_THREADS + 1` slots internally to accommodate the idle thread.

### 2. Application Setup (`main.c`)

```c
#include "stm32f4xx.h"
#include "lite_rtos.h"

// Priority 0: Highest priority task
void task_fast(void) {
    while (1) {
        GPIOC->ODR ^= (1 << 13); // Toggle LED
        rtos_delay(20);          // Non-blocking sleep for 20 ticks (200 ms at 100 Hz)
    }
}

// Priority 1: Lower priority task
void task_slow(void) {
    while (1) {
        // Run periodic sensor read or communication processing
        rtos_delay(50);          // Sleep for 50 ticks (500 ms)
    }
}

int main(void) {
    // 1. Configure MCU clocks and peripherals
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER |= (1 << 26);

    // 2. Register tasks with priorities (0 = highest)
    create_thread(task_fast, 0);
    create_thread(task_slow, 1);

    // 3. Start scheduler (configures SysTick at 100 Hz and triggers SVC 0)
    rtos_start();

    // Never reached
    while (1);
}
```

---

## API Reference

Defined in `inc/lite_rtos.h`:

- `int create_thread(void (*task_func)(void), uint32_t priority)`  
  Synthesizes initial 16-word context frame and marks task as ready in `ready_reg`. Priority ranges from `0` to `MAX_THREADS - 1`. Returns `0` on success, `-1` for invalid priority, `-2` if priority is already allocated.

- `void rtos_start(void)`  
  Creates idle thread at priority `MAX_THREADS`, initializes SysTick at 100 Hz (`SystemCoreClock / 100`), sets SysTick and PendSV interrupt priorities to 15, and executes `svc 0` to cold-boot the highest-priority task.

- `void rtos_delay(uint32_t ticks)`  
  Puts current task to sleep for `ticks` system periods (10 ms per tick). Clears ready bit and triggers immediate PendSV context switch.

- `void rtos_yield(void)`  
  Voluntarily give up the CPU to the highest-priority ready thread without sleeping(say if your resource held by some other thread).

---

## System Specs and Constraints

- **Architecture**: ARMv7-M (Cortex-M4, Cortex-M3).
- **Tested Hardware**: STM32F4 series (STM32F401, STM32F411).
- **Scheduling Policy**: Preemptive priority scheduling. Priority resolution is $O(1)$ via single-cycle `__builtin_ctz` over 32-bit ready bitmasks.
- **Memory Overhead**: Zero dynamic heap allocation. Each task consumes `(stack_size + 2) * 4` bytes of static RAM.
- **Tick Frequency**: 100 Hz (10 ms time slice).

### Constraints to Keep in Mind:
1. **FPU Stacking**: Context switching currently preserves core integer registers (`r4-r11`). If compiling with `-mfloat-abi=hard` and tasks execute floating-point instructions, the Cortex-M4 FPU lazy stacking mechanism pushes additional registers (`s0-s15`, `FPSCR`) which are not yet handled by `PendSV_Handler`. Use software floating-point or integer math.
2. **Priority Starvation**: The scheduler strictly picks the lowest active bit in `ready_reg`. If a high-priority task runs an infinite loop without calling `rtos_delay()` or `rtos_yield()`, lower-priority tasks will never execute.
3. **No Priority Inheritance**: Spinlocks across threads with different priorities can lead to priority inversion. Mutexes with priority ceiling or priority inheritance are planned for future revisions.
4. **Stack Bounds Checking**: There is no MPU boundary enforcement. If a thread exceeds `stack_size`, it will silently overwrite adjacent memory.

---

## References

- Joseph Yiu, *The Definitive Guide to ARM Cortex-M3 and Cortex-M4 Processors* (Exception handling, PSP/MSP duality, and NVIC control).
- ARMv7-M Architecture Reference Manual (Issue E, ARM DDI 0403E).
- FreeRTOS Cortex-M4 port implementation (`port.c`).

---

## License

GPLv3. See `LICENSE` for details.
