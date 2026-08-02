#ifndef __DELAY_H__
#define __DELAY_H__

#include <stdint.h>
#include <stddef.h>

#define GPIO_C          (*((volatile uint32_t *)0x40020818u))
#define LED_PC13_ALIAS  (*((volatile uint32_t *)0x424102B4u))
#define mx_thread 2
void SystemClock_Config(void);
void delay(int itr);
void blinken(void);
void blinkoff(void);
void context_switch(void);
void SysTick_Handler(void);

#endif /* __DELAY_H__ */
