#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "delay.h"
#include "cmsis_compiler.h"
#include <stdint.h>
#include "stm32f4xx.h"

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

void idle_thread(void) {
    while (1) {
        __asm volatile ("wfi");
    }
}

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

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    for (int idx = 0; idx < mx_thread; idx++) {
        arr[idx] = &Array[idx];
    }
    SYS_i = 0;

    pseudo_stack((int*)&blue);
    pseudo_stack((int*)&green);
    pseudo_stack((int*)&red);
    pseudo_stack((int*)&yellow);
    pseudo_stack((int*)&white);
    pseudo_stack((int*)&idle_thread);

    SYS_i = 0;
    curr = arr[SYS_i];
    prev = NULL;

    SysTick->LOAD = (SystemCoreClock / 100U) - 1U;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;

    HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;
    *((volatile uint32_t*) 0x40020800u) &= ~(3 << 26);
    *((volatile uint32_t*) 0x40020800u) |= (1 << 26);

    __enable_irq();
    __asm volatile ("svc 0");

    while (1) {
    }
}

void SystemClock_Config(void) {
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_3) {}

    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    LL_RCC_HSE_Enable();

    while (LL_RCC_HSE_IsReady() != 1) {}

    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 200, LL_RCC_PLLP_DIV_2);
    LL_RCC_PLL_Enable();

    while (LL_RCC_PLL_IsReady() != 1) {}
    while (LL_PWR_IsActiveFlag_VOS() == 0) {}

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {}

    LL_SetSystemCoreClock(100000000);

    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
        Error_Handler();
    }
    LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM11) {
        HAL_IncTick();
    }
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
}
#endif
