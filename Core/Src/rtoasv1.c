#include <stdint.h>
#include "stm32f4xx.h"
#include "delay.h"

extern int i;
extern int mx_thread;
extern int exc_add[2];

__attribute__((naked)) void SysTick_Handler(void)
{
    __asm volatile (
        "bl context_switch    \n"
        "ldr r0, =0xfffffff9  \n"
        "mov lr, r0           \n"
        "bx lr                \n"
    );
}

void context_switch(void)
{
    exc_add[i] = (uint32_t)__get_MSP();
    i = (i + 1) % mx_thread;
    __set_MSP(exc_add[i]);
}

void blinken(void)
{
    i = 0;
    while (1) {
        LED_PC13_ALIAS = 1;
        delay(500);
        delay(500);
    }
}

void blinkoff(void)
{
    i = 1;
    while (1) {
        delay(500);
        LED_PC13_ALIAS = 0;
        delay(500);
    }
}
