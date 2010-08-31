/*
 * thinnerclient.c
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

#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "platform_config.h"

#include "thinnerclient.h"

#include "video.h"
#include "keycodes.h"
#include "termconfig.h"
#include "terminal.h"

#include <stdint.h>
#include "defs.h"


//Peripheral configuration structures
GPIO_InitTypeDef			GPIO_InitStructure;
TIM_TimeBaseInitTypeDef		TIM_TimeBaseStructure;
TIM_OCInitTypeDef			TIM_OCInitStructure;
NVIC_InitTypeDef			NVIC_InitStructure;
SPI_InitTypeDef				SPI_InitStructure;
DMA_InitTypeDef				DMA_InitStructure;
USART_InitTypeDef			USART_InitStructure;
EXTI_InitTypeDef			EXTI_InitStructure;

//Timer stuf to generate sync and video display.  Need to tweak these if doing a PAL port.
#define TIMER_PERIOD 5083
#define INTERRUPT_DELAY 700
uint16_t CCR2_Val = 380;
uint16_t PrescalerValue = 0;

volatile uint16_t lineCount = 0;

//PLL hackery
void overclockSystemInit(void);

// Peripheral configuration functions
void GPIO_Config(void);
void RCC_Config(void);
void TIM_Config(void);
void NVIC_Config(void);
void SPI_Config(void);
void DMA_Config(void);
void USART_Config(void);
void EXTI_Config(void);

//Interrupt handlers
void TIM2_IRQHandler(void);			//Sync interrupt
void USART1_IRQHandler(void);		//USART received stuff interrupt
void EXTI15_10_IRQHandler(void);	//Keyboard interrupt handler

//Decode a keypress
void decode(uint8_t code);

//The framebuffer itself
uint16_t frameBuffer[BUFFER_VERT_SIZE][BUFFER_LINE_LENGTH];

uint32_t charCounter = 0;

#define ESC K_ESC
#define CLK K_CAPSLK
#define NLK K_NUMLK
#define SLK K_SCRLK
#define F1  K_F1
#define F2  K_F2
#define F3  K_F3
#define F4  K_F4
#define F5  K_F5
#define F6  K_F6
#define F7  K_F7
#define F8  K_F8
#define F9  K_F9
#define F10 K_F10
#define F11 K_F11
#define F12 K_F12
#define INS K_INS
#define DEL K_DEL
#define HOM K_HOME
#define END K_END
#define PGU K_PGUP
#define PGD K_PGDN
#define ARL K_LEFT
#define ARR K_RIGHT
#define ARU K_UP
#define ARD K_DOWN
#define PRS K_PRTSC
#define BRK K_BREAK


//Keyboard lookup tables
__attribute__((section("FLASH"))) const char codetable[] = {
	//   1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	0,   F9,  0,   F5,  F3,  F1,  F2,  F12, 0,   F10, F8,  F6,  F4,  '\t','`', 0,
	0,   0,   0,   0,   0,   'q', '1', 0,   0,   0,   'z', 's', 'a', 'w', '2', 0,
	0,   'c', 'x', 'd', 'e', '4', '3', 0,   0,   ' ', 'v', 'f', 't', 'r', '5', 0,
	0,   'n', 'b', 'h', 'g', 'y', '6', 0,   0,   0,   'm', 'j', 'u', '7', '8', 0,
	0,   ',', 'k', 'i', 'o', '0', '9', 0,   0,   '.', '/', 'l', ';', 'p', '-', 0,
	0,   0,   '\'',0,   '[', '=', 0,   0,   CLK, 0,   '\n',']', 0,   '\\',0,   0,
	0,   0,   0,   0,   0,   0,   '\b',0,   0,   '1', 0,   '4', '7', 0,   0,   0,
	'0', '.', '2', '5', '6', '8', ESC,  NLK, F11, '+', '3', '-', '*', '9', SLK, 0,
	0,   0,   0,   F7
};

__attribute__((section("FLASH"))) const char codetable_shifted[] = {
	//   1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	0,   F9,  0,   F5,  F3,  F1,  F2,  F12, 0,   F10, F8,  F6,  F4,  '\t','~', 0,
	0,   0,   0,   0,   0,   'Q', '!', 0,   0,   0,   'Z', 'S', 'A', 'W', '@', 0,
	0,   'C', 'X', 'D', 'E', '$', '#', 0,   0,   ' ', 'V', 'F', 'T', 'R', '%', 0,
	0,   'N', 'B', 'H', 'G', 'Y', '^', 0,   0,   0,   'M', 'J', 'U', '&', '*', 0,
	0,   '<', 'K', 'I', 'O', ')', '(', 0,   0,   '>', '?', 'L', ':', 'P', '_', 0,
	0,   0,   '"', 0,   '{', '+', 0,   0,   CLK, 0,   '\n','}', 0,   '|', 0,   0,
	0,   0,   0,   0,   0,   0,   '\b',0,   0,   '1', 0,   '4', '7', 0,   0,   0,
	'0', '.', '2', '5', '6', '8', ESC, NLK, F11, '+', '3', '-', '*', '9', SLK, 0,
	0,   0,   0,   F7
};

//codes that follow E0 or E1

__attribute__((section("FLASH"))) const char codetable_extended[] = {
	//   1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   PRS, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '\n',0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   END, 0,   ARL, HOM, 0,   0,   0,
	INS, DEL, ARD, '5', ARR, ARU, 0,   BRK, 0,   0,   PGD, 0,   PRS, PGU, 0,   0,
	0,   0,   0,   0
};


/* keyboard init */
static int8_t keyup = 0;
static int8_t extended = 0;
static int8_t bitcount = 11;
static uint8_t scancode = 0;
static int8_t mods = 0;

/* circular buffer for keys */
#define MAX_KEY_BUF 32
volatile uint8_t charbufsize = 0;
volatile uint8_t charbuf[MAX_KEY_BUF];
volatile uint8_t charbufhead = 0;
volatile uint8_t charbuftail = 0;

/* circular UART buffer */
#define MAX_BUF 254
volatile uint8_t bufsize;
volatile uint8_t buf[MAX_BUF];
volatile uint8_t bufhead;
volatile uint8_t buftail;

void thinnerClientSetup(void)
{
	
    // Setup (overclocked to 80MHz) STM32 system (clock, PLL and Flash configuration)  This is totally unnecessary.
	// Needed a clock speed that is agreeable to being divided by the spi port clock divider to give the resolution desired.
	// There is probably a lower system sepeed which will also work, but wanted to make sure we had enough cycles to get the job done....
	overclockSystemInit();
	
	//Set up the system clocks
	RCC_Config();
	
	// Setup the GPIOs
	GPIO_Config();
	
	//Setup the SPI port (video is output by its shift register).
	SPI_Config();
	
	//Setup the DMA to keep the spi port fed
	DMA_Config();
	
	//Setup the timers for generating sync and drawing to the screen.
	TIM_Config();
	
	//Set up the USART for sending and receiving data.
	USART_Config();
	
	//Setup the necessary interrupts.  I *think* that messing wiht priority levels will fix the screen distortion when receiving data from USART or keyboard.
	NVIC_Config();
	
	//Hook the external interrupts to the right pins.
	EXTI_Config();
}


void RCC_Config(void)
{

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);	
	RCC_AHBPeriphClockCmd(SPI_MASTER_DMA_CLK, ENABLE);	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
}

void GPIO_Config(void)
{	
	/* GPIOA Configuration:TIM2 Channel 2 as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_7 | GPIO_Pin_5 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_0; //Debugging Pins
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15; //setting up for keyboard pin change interrupts.
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_EXTILineConfig( GPIO_PortSourceGPIOB, GPIO_PinSource13 );  //connect exti
	
	
	/* Configure USART1 Rx as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	
	/* Configure USART1 Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);		
}

void TIM_Config(void)
{
	/* Compute the prescaler value */
	PrescalerValue = (uint16_t) 0;
	
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = TIMER_PERIOD; //72mhz/15.73426 khz line rate
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	
	/* PWM1 Mode configuration: Channel 2, because there is a broken wire stuck in the header for channel 1 on my protoboard.*/
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = CCR2_Val;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);
	
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);	
	
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_Pulse = INTERRUPT_DELAY;
	
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);
	
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
	
	/* Enable TIM2 Count Compare interrupt */
	TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
	
	/* TIM2 enable counter */
  	TIM_Cmd(TIM2, ENABLE);
}

void NVIC_Config(void)
{
	// Enable the EXTI10_15 Interrupt for keyboard transmissions
	NVIC_InitStructure.NVIC_IRQChannel	= EXTI15_10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd	= ENABLE;
	
	NVIC_Init(&NVIC_InitStructure);
	
	//Enable timer2 interrupt
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd	= ENABLE;
	
	NVIC_Init(&NVIC_InitStructure);
	
	//Enable USART interrupt
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	
	NVIC_Init(&NVIC_InitStructure);
}

void SPI_Config(void)
{
	//Set up SPI port.  This acts as a pixel buffer.
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	
	SPI_Init(SPI1, &SPI_InitStructure);
	
	SPI_Cmd(SPI1, ENABLE);	
}

void DMA_Config(void)
{
	
	//Set up the DMA to keep the SPI port fed from the framebuffer.
	DMA_DeInit(DMA1_Channel3);  
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)0x4001300C;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)frameBuffer[1];
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	
	DMA_InitStructure.DMA_BufferSize = BUFFER_LINE_LENGTH;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);
}

void USART_Config(void)
{	
	
	//Set up the USART.
	USART_InitStructure.USART_BaudRate = 57600; //57600; //9600; //921600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	
	USART_Init(USART1, &USART_InitStructure);
	
	//Enable receive interrupts
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	
	//enable the usart
	USART_Cmd(USART1, ENABLE);
}


void EXTI_Config(void)
{
	// Enable an interrupt on EXTI line 13 rising
	EXTI_InitStructure.EXTI_Line		= EXTI_Line13;
	EXTI_InitStructure.EXTI_Mode		= EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger	= EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd	= ENABLE;
	
	EXTI_Init(&EXTI_InitStructure);
}

void EXTI15_10_IRQHandler(void)
{
	//figure out what the keyboard is sending us
	EXTI_ClearFlag(EXTI_Line13);
	--bitcount;
	if (bitcount >= 2 && bitcount <= 9)
	{
		scancode >>= 1;
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15))
			scancode |= 0x80;
	}
	else if (bitcount == 0)
	{
		//puthex(scancode);
		decode(scancode);
		scancode = 0;
		bitcount = 11;
	}
}

void USART1_IRQHandler(void)
{
	//receive data from the serial port
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_0);		
		buf_enqueue(USART_ReceiveData(USART1));
		USART_ClearFlag(USART1, USART_FLAG_RXNE);
	}
}

void TIM2_IRQHandler(void)
{
	
	//here's where the ntsc video drawing magic happens!
	TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
	lineCount++;
	
	//DMA the next line of data to the screen!
	if(lineCount < BUFFER_VERT_SIZE)
	{
		DMA_DeInit(DMA1_Channel3);  
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)frameBuffer[lineCount];
		DMA_Init(DMA1_Channel3, &DMA_InitStructure);
		SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
		DMA_Cmd(DMA1_Channel3, ENABLE);
	}
	
	//vertical sync
	if(lineCount == 242)
	{
		TIM_SetCompare2(TIM2, (4575-342));
	}
	
	//transition back to normal sync
	if(lineCount == 261)
	{
		TIM_SetCompare2(TIM2, 511);
	}
	
	//go back to normal
	if(lineCount == 262)
	{
		TIM_SetCompare2(TIM2, 342);
		lineCount = 0;
	}
}


void waitForVsync(void)
{
	while(lineCount < 242);
}

void Delay(volatile unsigned long delay)
{
	for(; delay; --delay );
}

//fill the framebuffer with junk, used for debugging
void fillFrameBuffer(void)
{	
	uint16_t i;
	uint16_t j;
	for(i = 0; i<(BUFFER_VERT_SIZE); i++)
	{
		for(j = 0; j<(BUFFER_LINE_LENGTH); j++)
		{
			if(j < LEFT_MARGIN)
			{
				frameBuffer[i][j] = 0;
			} else if (j > 32){
				frameBuffer[i][j] = 0;
			} else {
				frameBuffer[i][j] = i*j;
			}
		}
	}
}

//Decode PS/2 keycodes
void decode(uint8_t code)
{
	if (code == 0xF0)
		keyup = 1;
	else if (code == 0xE0 || code == 0xE1)
		extended = 1;
	else
	{
		if (keyup) // handling a key release; don't do anything
		{
			if (code == 0x12) // left shift
				mods &= ~_BV(0);
			else if (code == 0x59) // right shift
				mods &= ~_BV(1);
			else if (code == 0x14) // left/right ctrl
				mods &= (extended) ? ~_BV(3) : ~_BV(2);
		}
		else // handling a key press; store character
		{
			if (code == 0x12) // left shift
				mods |= _BV(0);
			else if (code == 0x59) // right shift
				mods |= _BV(1);
			else if (code == 0x14) // left/right ctrl
				mods |= (extended) ? _BV(3) : _BV(2);
			else if (code <= 0x83)
			{
				uint8_t chr;
				if (extended)
					chr = codetable_extended[code];
				else if (mods & 0b1100) // ctrl
					chr = codetable[code] & 31;
				else if (mods & 0b0011) // shift
					chr = codetable_shifted[code];
				else
					chr = codetable[code];
				
				if (!chr) chr = '?';
				
				// add to buffer
				if (charbufsize < MAX_KEY_BUF)
				{
					charbuf[charbuftail] = chr;
					if (++charbuftail >= MAX_KEY_BUF) charbuftail = 0;
					charbufsize++;
				}
			}
		}
		extended = 0;
		keyup = 0;
	}
}

/*********Key and serial buffer stuff.  Needs to be updated to support atomic writing (if possible without messing up screen drawing, the sizes get mismatched easily******/
uint8_t buffer_get_key()
{
	if (charbufsize == 0)
		return 0;
	
	uint8_t newchar = charbuf[charbufhead];
	if (++charbufhead >= MAX_KEY_BUF) charbufhead = 0;
	charbufsize--;
	
	return newchar;
}

uint8_t key_buf_size()
{
	uint8_t sz;
	sz = charbufsize;
	return sz;
}


void buf_clear()
{
	bufsize = bufhead = buftail = 0;
}

void buf_enqueue(uint8_t c)
{
	if (1) //bufsize < MAX_BUF)
	{
		buf[buftail] = c;
		if (++buftail >= MAX_BUF) buftail = 0;
		bufsize++;
	}
}

uint8_t buf_dequeue()
{
	uint8_t ret = 0;
	if (1) //bufsize > 0)
	{
		uint8_t c = buf[bufhead];
		if (++bufhead >= MAX_BUF) bufhead = 0;
		bufsize--;
		ret = c;
	}
	return ret;
}

uint8_t buf_size()
{
	uint8_t sz;
	sz = bufsize;
	return sz;
}


/*************End of key and serial buffer stuff********************/




//hack hack hack hack copypaste hack hack hack yuck
void overclockSystemInit(void)
{
	/* Reset the RCC clock configuration to the default reset state(for debug purpose) */
	/* Set HSION bit */
	RCC->CR |= (uint32_t)0x00000001;
	
	/* Reset SW, HPRE, PPRE1, PPRE2, ADCPRE and MCO bits */
#ifndef STM32F10X_CL
	RCC->CFGR &= (uint32_t)0xF8FF0000;
#else
	RCC->CFGR &= (uint32_t)0xF0FF0000;
#endif /* STM32F10X_CL */   
	
	/* Reset HSEON, CSSON and PLLON bits */
	RCC->CR &= (uint32_t)0xFEF6FFFF;
	
	/* Reset HSEBYP bit */
	RCC->CR &= (uint32_t)0xFFFBFFFF;
	
	/* Reset PLLSRC, PLLXTPRE, PLLMUL and USBPRE/OTGFSPRE bits */
	RCC->CFGR &= (uint32_t)0xFF80FFFF;
	
#ifndef STM32F10X_CL
	/* Disable all interrupts and clear pending bits  */
	RCC->CIR = 0x009F0000;
#else
	/* Reset PLL2ON and PLL3ON bits */
	RCC->CR &= (uint32_t)0xEBFFFFFF;
	
	/* Disable all interrupts and clear pending bits  */
	RCC->CIR = 0x00FF0000;
	
	/* Reset CFGR2 register */
	RCC->CFGR2 = 0x00000000;
#endif /* STM32F10X_CL */
    
	/* Configure the System clock frequency, HCLK, PCLK2 and PCLK1 prescalers */
	/* Configure the Flash Latency cycles and enable prefetch buffer */
	__IO uint32_t StartUpCounter = 0, HSEStatus = 0;
	
	/* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/    
	/* Enable HSE */    
	RCC->CR |= ((uint32_t)RCC_CR_HSEON);
	
	/* Wait till HSE is ready and if Time out is reached exit */
	do
	{
		HSEStatus = RCC->CR & RCC_CR_HSERDY;
		StartUpCounter++;  
	} while((HSEStatus == 0) && (StartUpCounter != HSEStartUp_TimeOut));
	
	if ((RCC->CR & RCC_CR_HSERDY) != RESET)
	{
		HSEStatus = (uint32_t)0x01;
	}
	else
	{
		HSEStatus = (uint32_t)0x00;
	}  
	
	if (HSEStatus == (uint32_t)0x01)
	{
		/* Enable Prefetch Buffer */
		FLASH->ACR |= FLASH_ACR_PRFTBE;
		
		/* Flash 2 wait state */
		FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
		FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2;    
		
		
		/* HCLK = SYSCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
		
		/* PCLK2 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
		
		/* PCLK1 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;
		
#ifdef STM32F10X_CL
		/* Configure PLLs ------------------------------------------------------*/
		/* PLL2 configuration: PLL2CLK = (HSE / 5) * 8 = 40 MHz */
		/* PREDIV1 configuration: PREDIV1CLK = PLL2 / 5 = 8 MHz */
        
		RCC->CFGR2 &= (uint32_t)~(RCC_CFGR2_PREDIV2 | RCC_CFGR2_PLL2MUL |
								  RCC_CFGR2_PREDIV1 | RCC_CFGR2_PREDIV1SRC);
		RCC->CFGR2 |= (uint32_t)(RCC_CFGR2_PREDIV2_DIV5 | RCC_CFGR2_PLL2MUL8 |
								 RCC_CFGR2_PREDIV1SRC_PLL2 | RCC_CFGR2_PREDIV1_DIV5);
		
		/* Enable PLL2 */
		RCC->CR |= RCC_CR_PLL2ON;
		/* Wait till PLL2 is ready */
		while((RCC->CR & RCC_CR_PLL2RDY) == 0)
		{
		}
		
		
		/* PLL configuration: PLLCLK = PREDIV1 * 10 = 80 MHz */ 
		RCC->CFGR &= (uint32_t)~(RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL);
		RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLXTPRE_PREDIV1 | RCC_CFGR_PLLSRC_PREDIV1 | 
								RCC_CFGR_PLLMULL10); 
#else    
		/*  PLL configuration: PLLCLK = HSE * 10 = 80 MHz */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE |
											RCC_CFGR_PLLMULL));
		RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL10);
#endif /* STM32F10X_CL */
		
		/* Enable PLL */
		RCC->CR |= RCC_CR_PLLON;
		
		/* Wait till PLL is ready */
		while((RCC->CR & RCC_CR_PLLRDY) == 0)
		{
		}
		
		/* Select PLL as system clock source */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
		RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    
		
		/* Wait till PLL is used as system clock source */
		while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08)
		{
		}
	}
	else
	{ /* If HSE fails to start-up, the application will have wrong clock 
	   configuration. User can add here some code to deal with this error */    
		
		/* Go to infinite loop */
		while (1)
		{
		}
	}
}







