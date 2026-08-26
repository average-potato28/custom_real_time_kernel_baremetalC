#ifndef __LITE_RTOS_H__
#define __LITE_RTOS_H__

#include <stdint.h>
#include <stddef.h>
#include "lite_rtos_config.h"

#define mx_thread (MAX_THREADS + 1)

#define GPIO_C         *((volatile uint32_t*) 0x40020818u)
#define LED_PC13_ALIAS *((volatile uint32_t*) 0x424102B4u)

typedef struct {
    uint32_t *sp;
    uint32_t timer;
    uint32_t mem_alloc[stack_size];
} OS_boy;

extern OS_boy Array[mx_thread];
extern OS_boy *arr[mx_thread];
extern OS_boy *curr;
extern OS_boy *prev;
extern volatile uint32_t SYS_i;
extern volatile uint32_t ready_reg[(mx_thread + 31) / 32];

void SystemClock_Config(void);

void white(void);
void blue(void);
void green(void);
void yellow(void);
void red(void);
void idle_thread(void);

void SysTick_Handler(void);
void PendSV_Handler(void);
void SVC_Handler(void);

int create_thread(void (*task_func)(void), uint32_t priority);
void SYS_ticks(void);
void SYS_prep(void);
void rtos_delay(uint32_t ticks);
void rtos_yield(void);

#endif
