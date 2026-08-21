#include <stdint.h>
#include "stm32f4xx.h"
#include "delay.h"
#include "usart.h"

OS_boy Array[mx_thread] __attribute__((aligned(8)));
OS_boy *arr[mx_thread];
OS_boy *curr = NULL;
OS_boy *prev = NULL;
volatile uint32_t SYS_i = 0;
volatile uint32_t ready_reg[(mx_thread + 31) / 32] = {0};

volatile int uart_lock = 0;

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

__attribute__((naked)) void PendSV_Handler(void) {
    __asm volatile (
        "mrs r0, psp            \n"
        "ldr r1, =prev          \n"
        "ldr r1, [r1]           \n"
        "stmdb r0!, {r4-r11}    \n"
        "str r0, [r1]           \n"
        "ldr r1, =curr          \n"
        "ldr r1, [r1]           \n"
        "ldr r0, [r1]           \n"
        "ldmia r0!, {r4-r11}    \n"
        "msr psp, r0            \n"
        "bx lr                  \n"
        ::: "memory"
    );
}

void rtos_yield(void) {
    __disable_irq();
    SYS_prep();
    __enable_irq();
    __ISB();
}

void blue(void) {
    while (1) {
        while (uart_lock == 1) {
            rtos_yield();
        }
        uart_lock = 1;
        HAL_UART_Transmit(&huart1, (uint8_t*)"b0\r\n", 4, 10);
        uart_lock = 0;
        rtos_delay(50);
    }
}

void green(void) {
    while (1) {
        while (uart_lock == 1) {
            rtos_yield();
        }
        uart_lock = 1;
        HAL_UART_Transmit(&huart1, (uint8_t*)"g0\r\n", 4, 10);
        uart_lock = 0;
        rtos_delay(30);
    }
}

void red(void) {
    while (1) {
        while (uart_lock == 1) {
            rtos_yield();
        }
        uart_lock = 1;
        HAL_UART_Transmit(&huart1, (uint8_t*)"r0\r\n", 4, 8);
        uart_lock = 0;
        rtos_delay(10);
    }
}

void yellow(void) {
    while (1) {
        while (uart_lock == 1) {
            rtos_yield();
        }
        uart_lock = 1;
        HAL_UART_Transmit(&huart1, (uint8_t*)"y0\r\n", 4, 8);
        GPIO_C = (1ul << 13);
        uart_lock = 0;
        rtos_delay(20);
    }
}

void white(void) {
    while (1) {
        while (uart_lock == 1) {
            rtos_yield();
        }
        uart_lock = 1;
        HAL_UART_Transmit(&huart1, (uint8_t*)"w0\r\n", 4, 8);
        GPIO_C = (1ul << 29);
        uart_lock = 0;
        rtos_delay(40);
    }
}
