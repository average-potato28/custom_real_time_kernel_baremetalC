#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "delay.h"
#include "cmsis_compiler.h"

int i = 1;
int exc_add[mx_thread];
int mem_alloc[mx_thread][stack_size];

__attribute__((naked)) void fake_stack(int *ptr)
{
    __disable_irq();
    __set_MSP((uint32_t)&mem_alloc[i][stack_size - 1]);
    __asm volatile (
        "mov r0, 0x01000000  \n\t"
        "push {r0}           \n\t"
        "push {%[my_ptr]}    \n\t"
        "push {r0-r3,r12,r14}\n\t"
        "push {r4-r11}       \n\t"
        :
        : [my_ptr] "r" (ptr)
        : "r0", "memory"
    );
    exc_add[i] = __get_MSP();
    i = (i + 1) % mx_thread;
    __set_MSP((uint32_t)&mem_alloc[i][stack_size - 1]);
    __enable_irq();
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_UART_Init();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;

    *((volatile uint32_t *)0x40020800u) &= ~(3 << 26);
    *((volatile uint32_t *)0x40020800u) |= (1 << 26);

    SysTick->LOAD = (SystemCoreClock / 100U) - 1U;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;

    SCB->SHP[11] = 0xFF;

    fake_stack((int *)&green);
    fake_stack((int *)&red);
    fake_stack((int *)&yellow);
    fake_stack((int *)&white);

    blue();

    while (1) {
    }
}

void SystemClock_Config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_3) {
    }

    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    LL_RCC_HSE_Enable();

    while (LL_RCC_HSE_IsReady() != 1) {
    }

    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 200, LL_RCC_PLLP_DIV_2);
    LL_RCC_PLL_Enable();

    while (LL_RCC_PLL_IsReady() != 1) {
    }

    while (LL_PWR_IsActiveFlag_VOS() == 0) {
    }

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
    }

    LL_SetSystemCoreClock(100000000);

    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
        Error_Handler();
    }

    LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM11) {
        HAL_IncTick();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
