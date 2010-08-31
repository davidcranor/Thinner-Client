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