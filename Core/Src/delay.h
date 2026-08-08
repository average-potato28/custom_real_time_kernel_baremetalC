#ifndef __DELAY_H__
#define __DELAY_H__

#include <stdint.h>
#include <stddef.h>

#define mx_thread   5
#define stack_size  80

#define GPIO_C             (*((volatile uint32_t *)0x40020818u))
#define LED_PC13_ALIAS     (*((volatile uint32_t *)0x424102B4u))

extern int i;
extern int exc_add[mx_thread];
extern int mem_alloc[mx_thread][stack_size];

void SystemClock_Config(void);
void delay(int ms);
void SysTick_Handler(void);
void fake_stack(int *ptr);

void blue(void);
void green(void);
void red(void);
void yellow(void);
void white(void);

#endif /* __DELAY_H__ */
