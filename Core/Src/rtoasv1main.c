#include "main.h"
#include "gpio.h"
#include "delay.h"
#include "cmsis_compiler.h"
#include "stm32f4xx.h"

int i = 0;
int mx_thread;
int exc_add[2];
int arr1[40];
int arr2[40];

void fake_stack(int *ptr);

__attribute__((naked)) void fake_stack(int *ptr)
{
    __set_MSP((uint32_t)&arr1[40]);
    __asm volatile (
        "mov r0, 0x01000000 \n"
        "push {r0}          \n"
        "push {%[my_ptr]}   \n"
        "bx lr              \n"
        :
        : [my_ptr] "r" (ptr)
        : "r0", "memory"
    );
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR; // Dummy read to force clock enable to take effect

    *((volatile uint32_t *)0x40020800u) &= ~(3 << 26); // Clear GPIOC MODER13
    *((volatile uint32_t *)0x40020800u) |= (1 << 26);  // Set GPIOC MODER13 as Output

    SysTick->LOAD = (SystemCoreClock / 100U) - 1U;
    SysTick->VAL  = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;

    __disable_irq();
    fake_stack((int *)&blinken);
    __enable_irq();

    __set_MSP((int)&arr2[40]);
    exc_add[i] = (uint32_t)&arr1[32];
    mx_thread = 2;

    blinkoff();

    while (1) {
    }
}

void SystemClock_Config(void)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
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
        while (1) {
        }
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
