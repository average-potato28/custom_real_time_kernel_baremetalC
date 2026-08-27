#ifndef __LITE_RTOS_H__
#define __LITE_RTOS_H__

#include <stdint.h>
#include <stddef.h>
#include "lite_rtos_config.h"

typedef struct OS_boy OS_boy;

int create_thread(void (*task_func)(void), uint32_t priority);
void rtos_start(void);
void rtos_delay(uint32_t ticks);
void rtos_yield(void);

#endif
