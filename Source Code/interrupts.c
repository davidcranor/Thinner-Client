/*
* Interrupt vector implementations go here. Just add any you need.
* A full list of possible vectors is in lib/CMSIS_CM3/startup/gcc/stm32f10x_*.s
* You can also put the interrupt vector anywhere that gets compiled in,
* including one source file per interrupt, in main.c, etc. Be sure to keep
* the ones listed here, though.
*/

#include "interrupts.h"



/* The following interrupts should be present, though you can of course
 * modify them as required.
 */

void NMI_Handler(void) {
}

void HardFault_Handler(void) {
	/* Go to infinite loop when Hard Fault exception occurs */
	while (1);
}


void MemManage_Handler(void) {
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1);
}

void BusFault_Handler(void) {
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1);
}

void UsageFault_Handler(void) {
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1);
}

void SVC_Handler(void) {
}

void DebugMon_Handler(void) {
}

void PendSV_Handler(void) {
}

void SysTick_Handler(void) {
}
