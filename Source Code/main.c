/*
 * main.c
 * 
 * An 80x24 vt100 terminal emulator for STM32F microcontrollers
 *
 * David Cranor and Max Lobovsky
 * 8/30/10
 *
 * (c) Massachusetts Institute of Technology 2010
 * Permission granted for experimental and personal use;
 * license for commercial sale available from MIT.
 */



/*********************************************************************************************
 
 
 Hey kids at home who want to write your own NTSC display stuff using this project!
 All you have to do (assuming you already have the arm toolchain and ST's peripheral library) 
 is make a main.c which includes the following:
 
 stm32f10x.h
 stm32f10x_exti.h
 thinnerclient.h
 
 as well as a call to thinnerClientSetup();
 
 and you have access to the following constructs:
 
 frameBuffer[BUFFER_VERT_SIZE][BUFFER_LINE_LENGTH];
 uint8_t buffer_get_key()
 uint8_t buf_dequeue()
 
 anything written to frameBuffer appears on the screen.
 buffer_get_key and buf_dequeue retrieve bytes from the keyboard buffer and USART receive buffer, respectively.
 
 check out thinnerclient.h and thinnerclient.c for details.
 
**********************************************************************************************/

#include "stm32f10x.h"
#include "stm32f10x_exti.h"

#include "video.h"
#include "termconfig.h"
#include "terminal.h"

#include "Font6x8.h"

#include <stdint.h>
#include "defs.h"

#include "thinnerclient.h"

int main(void){
	
	//Set up video generation, USART, and keyboard reading
	thinnerClientSetup();	
	
	//Set up terminal stuff
	video_setup();
	app_setup();
	
	while (1)
	{
		
		//Process incoming characters, then deal with keystrokes and send them out.
		
		while(bufhead != buftail)  //this is hacky.  doing a while bufsize should work here, but it doesn't; bufsize gets misaligned randomly. I think the problem is to do with non-atomic writes to the buffer.
		{
			receive_char(buf_dequeue());
		}
		
		while(key_buf_size())
		{
			while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
			app_handle_key(buffer_get_key());
		}
		
		//Might help with screen tearing issues
		waitForVsync();
		
		//Copy tilemap into main framebuffer
		updateFrameBuffer(tileMap, Font6x8);
	}
}