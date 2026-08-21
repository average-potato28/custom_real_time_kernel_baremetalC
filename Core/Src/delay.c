#include "delay.h"
#include <stdint.h>
#include "stm32f4xx.h"
#include "cmsis_compiler.h"

void delay(int itr) {
    for (volatile uint32_t i = 0; i < itr * 1000; i++) {
        __NOP();
    }
}
