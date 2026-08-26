#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx.h"
#include "delay.h"

OS_boy Array[mx_thread] __attribute__((aligned(8)));
OS_boy *arr[mx_thread];
OS_boy *curr = NULL;
OS_boy *prev = NULL;
volatile uint32_t SYS_i = 0;
volatile uint32_t ready_reg[(mx_thread + 31) / 32] = {0};

void SysTick_Handler(void) {
    SYS_ticks();
    SYS_prep();
}

void SYS_ticks(void) {
    for (int t_i = 0; t_i < (mx_thread - 1); t_i++) {
        if (arr[t_i]->timer > 0) {
            arr[t_i]->timer--;
            if (arr[t_i]->timer == 0) {
                ready_reg[t_i >> 5] |= (1UL << (t_i & 31));
            }
        }
    }
}

void SYS_prep(void) {
    int curr_i = SYS_i;
    prev = arr[SYS_i];

    for (int i = 0; i < (mx_thread + 31) / 32; i++) {
        if (ready_reg[i] != 0) {
            SYS_i = 32 * i + __builtin_ctz(ready_reg[i]);
            break;
        }
    }

    if (curr_i == SYS_i) {
        return;
    }

    curr = arr[SYS_i];
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void rtos_delay(uint32_t ticks) {
    __disable_irq();
    if (ticks > 0) {
        arr[SYS_i]->timer = ticks;
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

void pseudo_stack(int *ptr) {
    int i = 1;

    // Hardware Exception Frame
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0x01000000;    // xPSR
    arr[SYS_i]->mem_alloc[stack_size - i++] = (uint32_t)ptr; // PC
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // LR
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R12
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R3
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R2
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R1
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R0

    // Software Saved Frame
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R11
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R10
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R9
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R8
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R7
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R6
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R5
    arr[SYS_i]->mem_alloc[stack_size - i++] = 0;             // R4

    arr[SYS_i]->sp = &arr[SYS_i]->mem_alloc[stack_size - 16];
    arr[SYS_i]->timer = 0;
    ready_reg[SYS_i >> 5] |= (1UL << (SYS_i & 31));

    SYS_i = (SYS_i + 1) % mx_thread;
}
