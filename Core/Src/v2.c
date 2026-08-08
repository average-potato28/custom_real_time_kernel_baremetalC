#include <stdint.h>
#include "stm32f4xx.h"
#include "delay.h"
#include "usart.h"

extern uint32_t SystemCoreClock;

__attribute__((naked)) void SysTick_Handler(void)
{
    __asm volatile (
        "push {r4-r11}             \n\t"
        "ldr  r0, =exc_add         \n\t"
        "mrs  r1, msp              \n\t"
        "ldr  r2, =i               \n\t"
        "ldr  r3, [r2]             \n\t"
        "str  r1, [r0, r3, lsl #2] \n\t"
        "add  r3, r3, #1           \n\t"
        "cmp  r3, %[max_threads]   \n\t"
        "it   eq                   \n\t"
        "moveq r3, #0              \n\t"
        "str  r3, [r2]             \n\t"
        "ldr  r2, [r0, r3, lsl #2] \n\t"
        "msr  msp, r2              \n\t"
        "pop  {r4-r11}             \n\t"
        "ldr  lr, =0xFFFFFFF9      \n\t"
        "bx   lr                   \n\t"
        :
        : [max_threads] "i" (mx_thread)
        : "memory", "r0", "r1", "r2", "r3"
    );
}

void blue(void)
{
    while (1) {
        LED_PC13_ALIAS = 0;
        __disable_irq();
        HAL_UART_Transmit(&huart1, (uint8_t *)"b0\r\n", 4, 8);
        __enable_irq();
        delay(500);
    }
}

void green(void)
{
    while (1) {
        __disable_irq();
        HAL_UART_Transmit(&huart1, (uint8_t *)"g0\r\n", 4, 8);
        __enable_irq();
        delay(50);
    }
}

void red(void)
{
    while (1) {
        __disable_irq();
        HAL_UART_Transmit(&huart1, (uint8_t *)"r0\r\n", 4, 8);
        __enable_irq();
        delay(10);
    }
}

void yellow(void)
{
    while (1) {
        __disable_irq();
        HAL_UART_Transmit(&huart1, (uint8_t *)"y0\r\n", 4, 8);
        __enable_irq();
        delay(200);
    }
}

void white(void)
{
    while (1) {
        LED_PC13_ALIAS = 1;
        __disable_irq();
        HAL_UART_Transmit(&huart1, (uint8_t *)"w0\r\n", 4, 8);
        __enable_irq();
        delay(100);
    }
}
