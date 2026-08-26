#include <stdint.h>
#include "delay.h"

__attribute__((naked)) void SVC_Handler(void) {
    __asm volatile (
        "ldr r1, =curr          \n"
        "ldr r1, [r1]           \n"
        "ldr r0, [r1]           \n"
        "ldmia r0!, {r4-r11}    \n"
        "msr psp, r0            \n"
        "ldr lr, =0xFFFFFFFD    \n"
        "bx lr                  \n"
        ::: "memory"
    );
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
