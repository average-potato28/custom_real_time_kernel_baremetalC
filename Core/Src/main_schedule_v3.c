#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "lite_rtos.h"
#include "cmsis_compiler.h"
#include <stdint.h>
#include "stm32f4xx.h"

#define GPIO_C *((volatile uint32_t*) 0x40020818u)

volatile int uart_lock = 0;

void SystemClock_Config(void);

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

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;
    *((volatile uint32_t*) 0x40020800u) &= ~(3 << 26);
    *((volatile uint32_t*) 0x40020800u) |= (1 << 26);

    create_thread(blue, 0);
    create_thread(green, 1);
    create_thread(red, 2);
    create_thread(yellow, 3);
    create_thread(white, 4);

    rtos_start();

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
