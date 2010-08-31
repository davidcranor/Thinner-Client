/*
 *  thinnerclient.h
 *
 * NTSC display, UART reading, and PS/2 keyboard driver
 * for STM32F series microcontrollers
 *
 * David Cranor and Max Lobovsky (PS/2 stuff adapted from work by Matt Sarnoff)
 * 8/30/10
 *
 * (c) Massachusetts Institute of Technology 2010
 * Permission granted for experimental and personal use;
 * license for commercial sale available from MIT.
 */

#ifndef _THINNERCLIENT_H_
#define _THINNERCLIENT_H_

#include "defs.h"
#include "stm32f10x.h"
#include "stm32f10x_exti.h"

#define BUFFER_LINE_LENGTH         31  //Yes, in 16 bit halfwords.
#define BUFFER_VERT_SIZE           240

#define _BV(bit) (1 << (bit))  //Useful macro to ease the transition from using avrlibc.

//Peripheral init structures.  Extern'd in case the main program needs to mess with them.
extern GPIO_InitTypeDef			GPIO_InitStructure;
extern TIM_TimeBaseInitTypeDef		TIM_TimeBaseStructure;
extern TIM_OCInitTypeDef			TIM_OCInitStructure;
extern NVIC_InitTypeDef			NVIC_InitStructure;
extern SPI_InitTypeDef				SPI_InitStructure;
extern DMA_InitTypeDef				DMA_InitStructure;
extern USART_InitTypeDef			USART_InitStructure;
extern EXTI_InitTypeDef			EXTI_InitStructure;

//Set everything up
void thinnerClientSetup(void);

//A simple delay
void Delay(volatile unsigned long delay);

//May help with screen tearing
void waitForVsync(void);

//Write junk to the framebuffer for debug purposes.
void fillFrameBuffer(void);

//key buffer stuff
uint8_t key_buf_size(void);
uint8_t buffer_get_key(void);

//serial rx buffer stuff
uint8_t buf_dequeue();
void buf_clear();
void buf_enqueue(uint8_t c);
uint8_t buf_size();

//Anything that is written to frameBuffer is automagically transferred to the screen in the background.
extern uint16_t frameBuffer[BUFFER_VERT_SIZE][BUFFER_LINE_LENGTH];

extern volatile uint8_t bufhead;
extern volatile uint8_t buftail;

#endif



