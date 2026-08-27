#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx.h"
#include "lite_rtos.h"

#define mx_thread (MAX_THREADS + 1)

struct OS_boy {
    uint32_t *sp;
    uint32_t timer;
    uint32_t mem_alloc[stack_size];
};

static OS_boy Array[mx_thread] __attribute__((aligned(8)));
OS_boy *curr = NULL;
OS_boy *prev = NULL;
static volatile uint32_t SYS_i = 0;
static volatile uint32_t ready_reg[(mx_thread + 31) / 32] = {0};

static void SYS_ticks(void);
static void SYS_prep(void);
static void idle_thread(void);
static int create_thread_internal(void (*task_func)(void), uint32_t priority);

void SysTick_Handler(void) {
    SYS_ticks();
    SYS_prep();
}

static void SYS_ticks(void) {
    for (int t_i = 0; t_i < (mx_thread - 1); t_i++) {
        if (Array[t_i].timer > 0) {
            Array[t_i].timer--;
            if (Array[t_i].timer == 0) {
                ready_reg[t_i >> 5] |= (1UL << (t_i & 31));
            }
        }
    }
}

static void SYS_prep(void) {
    int curr_i = SYS_i;
    prev = &Array[SYS_i];

    for (int i = 0; i < (mx_thread + 31) / 32; i++) {
        if (ready_reg[i] != 0) {
            SYS_i = 32 * i + __builtin_ctz(ready_reg[i]);
            break;
        }
    }

    if (curr_i == SYS_i) {
        return;
    }

    curr = &Array[SYS_i];
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void rtos_delay(uint32_t ticks) {
    __disable_irq();
    if (ticks > 0) {
        Array[SYS_i].timer = ticks;
        ready_reg[SYS_i >> 5] &= ~(1UL << (SYS_i & 31));
        SYS_prep();
    }
    __enable_irq();
    __ISB();
}

void rtos_yield(void) {
    __disable_irq();
    SYS_prep();
    __enable_irq();
    __ISB();
}

static int create_thread_internal(void (*task_func)(void), uint32_t priority) {
    if (priority > MAX_THREADS) {
        return -1;
    }

    if (ready_reg[priority >> 5] & (1UL << (priority & 31))) {
        return -2;
    }

    int i = 1;

    Array[priority].mem_alloc[stack_size - i++] = 0x01000000;
    Array[priority].mem_alloc[stack_size - i++] = (uint32_t)task_func;
    Array[priority].mem_alloc[stack_size - i++] = 0;
    Array[priority].mem_alloc[stack_size - i++] = 0;
    Array[priority].mem_alloc[stack_size - i++] = 0;
    Array[priority].mem_alloc[stack_size - i++] = 0;
    Array[priority].mem_alloc[stack_size - i++] = 0;
    Array[priority].mem_alloc[stack_size - i++] = 0;

    Array[priority].mem_alloc[stack_size - i++] = 0;
    Array[priority].mem_alloc[stack_size - i++] = 0;
    Array[priority].mem_alloc[stack_size - i++] = 0;
    Array[priority].mem_alloc[stack_size - i++] = 0;
    Array[priority].mem_alloc[stack_size - i++] = 0;
    Array[priority].mem_alloc[stack_size - i++] = 0;
    Array[priority].mem_alloc[stack_size - i++] = 0;
    Array[priority].mem_alloc[stack_size - i++] = 0;

    Array[priority].sp = &Array[priority].mem_alloc[stack_size - 16];
    Array[priority].timer = 0;
    ready_reg[priority >> 5] |= (1UL << (priority & 31));

    return 0;
}

int create_thread(void (*task_func)(void), uint32_t priority) {
    if (priority >= MAX_THREADS) {
        return -1;
    }
    return create_thread_internal(task_func, priority);
}

static void idle_thread(void) {
    while (1) {
        __asm volatile ("wfi");
    }
}

void rtos_start(void) {
    create_thread_internal(idle_thread, MAX_THREADS);

    for (int i = 0; i < (mx_thread + 31) / 32; i++) {
        if (ready_reg[i] != 0) {
            SYS_i = 32 * i + __builtin_ctz(ready_reg[i]);
            break;
        }
    }

    curr = &Array[SYS_i];
    prev = NULL;

    SysTick->LOAD = (SystemCoreClock / 100U) - 1U;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;

    NVIC_SetPriority(SysTick_IRQn, 15);
    NVIC_SetPriority(PendSV_IRQn, 15);

    __enable_irq();
    __asm volatile ("svc 0");

    while (1);
}
