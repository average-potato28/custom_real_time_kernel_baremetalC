#include <stdint.h>
#include "stm32f4xx.h"
#include "delay.h"
#include "usart.h"



int* swap (int* x,int* y){
	static int temp [2];
	temp[0] = *y;
	temp[1]= *x;
	return temp;

}

volatile int bright[2]={300,700};




extern int i;extern  int   exc_add[mx_thread];
extern uint32_t SystemCoreClock;
__attribute__((naked))void SysTick_Handler(void)
{ __asm volatile (
//"push {lr} \n"
		"push {r4-r11} \n"
		"bl context_switch \n"
	//	"pop {lr} \n"
		"pop {r4-r11}  \n"
		 "ldr r0, = 0xfffffff9  \n"
		"mov lr, r0 \n"
		//"add sp, sp, #4 \n"
		"bx lr \n"

);

	  }
void context_switch(void){
	exc_add[i] = (uint32_t)__get_MSP() ;
	i= (i+1)%mx_thread ;
	__set_MSP (exc_add[i]);

}














		void blinken (void) {
			//i =0;
		while(1){ LED_PC13_ALIAS = 1; delay(500);         delay(5000);
			 }
		}




		void blinkoff(void){
			//i=1;
	while(1) {              delay(500);  LED_PC13_ALIAS = 0;delay(5000);
			 }
		}








