/*
 * terminal.c
 * 
 * Serial terminal and ANSI/VT100 escape sequence handling
 *
 * Originally by Matt Sarnoff, 
 * hacked up by David Cranor and Max Lobovsky to work with STM32F and thinnerclient.
 * Also, added support for a couple more escape sequences.
 * 8/30/10
 *
 * (c) Massachusetts Institute of Technology 2010
 * Permission granted for experimental and personal use;
 * license for commercial sale available from MIT.
 */

#include "terminal.h"
#include "video.h"
#include "keycodes.h"
#include "termconfig.h"

#include "thinnerclient.h"

#include "stm32f10x.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define MAX_ESC_LEN 48

/* key sequences sent by non-ASCII keys */
static const char * specialkeyseqs[K_NUMLK-K_F1] = {
	"\x1BOP",   /* F1 */
	"\x1BOQ",   /* F2 */
	"\x1BOR",   /* F3 */
	"\x1BOS",   /* F4 */
	"\x1B[15~", /* F5 */
	"\x1B[17~", /* F6 */
	"\x1B[18~", /* F7 */
	"\x1B[19~", /* F8 */
	"\x1B[20~", /* F9 */
	"\x1B[21~", /* F10 */
	"\x1B[23~", /* F11 */
	"\x1B[24~", /* F12 */
	"\x1BOA",   /* UP */
	"\x1BOD",   /* LEFT */
	"\x1BOB",   /* DOWN */
	"\x1BOC",   /* RIGHT */
	"\x1B[2~",  /* INS */
	"\x1B[3~",  /* DEL */
	"\x1B[H",   /* HOME */
	"\x1B[F",   /* END */
	"\x1B[5~",  /* PGUP */
	"\x1B[6~",  /* PGDN */
};

enum
{
	NOT_IN_ESC,
	ESC_GOT_1B,
	ESC_CSI,
	ESC_NONCSI,
};

typedef struct
{
	int8_t cx;
	int8_t cy;
	uint8_t graphicchars;
	uint8_t revvideo;
} termstate_t;

/* setup screen active? */
static bool in_setup;

/* escape sequence processing */
static uint8_t in_esc;
static char paramstr[MAX_ESC_LEN+1];
static char *paramptr;
static uint8_t paramch;

/* parameters from setup */
static uint32_t newlineseq;
static uint32_t process_escseqs;
static uint32_t local_echo;

/* current attributes */
static uint8_t graphicchars;  /* set to 1 with an SI and set to 0 with an SO */
static uint8_t revvideo;      /* reverse video attribute */
static termstate_t savedstate;/* state used for save/restore sequences */

/* prototypes */
void escseq_process(char c);
void escseq_process_noncsi(char c);
void escseq_process_csi(char c);
void escseq_csi_start();
uint8_t escseq_get_param(uint8_t defaultval);
void save_term_state();
void restore_term_state();
void reset_term();
extern uint16_t frame;

void uart_update()
{	
	USART_InitStructure.USART_BaudRate = cfg_param_value(TC_BAUDRATE); //9600; //57600; //9600; //921600;
	USART_InitStructure.USART_WordLength = cfg_param_value(TC_DATABITS); //USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = cfg_param_value(TC_STOPBITS); //USART_StopBits_1;
	USART_InitStructure.USART_Parity = cfg_param_value(TC_PARITY); //USART_Parity_No;
	
	USART_Init(USART1, &USART_InitStructure);
}

void uart_putchar(char c)
{	
	USART_SendData(USART1, c);
	
	if (local_echo)
		receive_char(c);
}

void receive_char(uint8_t c)
{
		if (!process_escseqs)
		{
			video_putc_raw(c);
			return;
		}
	
	if (c)
	{
		if (in_esc)
			escseq_process(c);
		else switch (c)
		{
			case 0x07: /* BEL */
				break;   /* ignore bells */
			case '\b': /* backspace */
				video_cback();
				break;
			case 0x0A: /* LF, VT, and FF all print a linefeed */
			case 0x0B:
			case 0x0C:
				video_lf();
				break;
			case 0x0D: /* CR */
				video_setx(0);
				break;
			case 0x0E: /* SO; enable box-drawing characters */
				graphicchars = 1;
				break;
			case 0x0F: /* SI; return to normal characters */
				graphicchars = 0;
				break;
			case 0x1B: /* ESC */
				in_esc = ESC_GOT_1B;
				break;
			case 0x7F: /* DEL */
				break;
			default:
				if (c >= ' ')
				{
					if (graphicchars && c >= '_' && c <= '~')
						c -= 95;
					c |= revvideo;
					video_putc_raw(c);
				}
		}
	}
}


void escseq_process(char c)
{
	/* CAN and SUB interrupt escape sequences */
	if (c == 0x18 || c == 0x1A)
	{
		in_esc = NOT_IN_ESC;
		return;
	}
	
	if (in_esc == ESC_CSI)
		escseq_process_csi(c);
	else if (in_esc == ESC_NONCSI)
	{
		/* received a non-CSI sequence that requires a parameter
		 * (ESC #, ESC %, etc)
		 * These aren't supported, so eat this character */
		in_esc = NOT_IN_ESC;
	} 
	else if (in_esc == ESC_GOT_1B)
	{
		in_esc = ESC_NONCSI;
		escseq_process_noncsi(c);
	}
}

/* Process sequences that begin with ESC */
void escseq_process_noncsi(char c)
{
	switch (c)
	{
		case '[': /* got the [; this is a CSI sequence */
			escseq_csi_start();
			in_esc = ESC_CSI;
			break;
		case '%': /* non-CSI codes that require parameters */
		case '#': /* (we don't support these) */
		case '(':
		case ')':
			break;  /* return without setting in_esc to NOT_IN_ESC */
		case '7': /* save cursor position and attributes */
			save_term_state();
			goto esc_done;
		case '8': /* restore cursor position and attributes */
			restore_term_state();
			goto esc_done;
		case 'E': /* next line */
			video_movesol(); /* fall through */
		case 'D': /* index */
			if (video_gety() == video_bottom_margin())
				video_scrollup();
			else
				video_movey(1);
			goto esc_done;
		case 'M': /* reverse index */
			if (video_gety() == video_top_margin())
				video_scrolldown();
			else
				video_movey(-1);
			goto esc_done;
		case 'c': /* reset */
			video_clrscr();
			reset_term();
			goto esc_done;
		default: /* other non-CSI codes */
		esc_done:
			in_esc = NOT_IN_ESC; /* unimplemented */
			break;
	}
}

/* Process sequences that begin with ESC [ */
void escseq_process_csi(char c)
{
	if ((c >= '0' && c <= '9') || c == ';' || c == '?') /* digit or separator */
	{
		/* save the character */
		if (paramch >= MAX_ESC_LEN) /* received too many characters */
		{
			in_esc = NOT_IN_ESC;
			return;
		}
		paramstr[paramch] = c;
		paramch++;
	}
	else
	{
		/* take the appropriate action.  scroll back not implemented */
		switch (c)
		{
			case 'A': /* cursor up */
				video_movey(-escseq_get_param(1));
				break;
			case 'B': /* cursor down */
				video_movey(escseq_get_param(1));
				break;
			case 'C': /* cursor forward */
				video_movex(escseq_get_param(1));
				break;
			case 'D': /* cursor back */
				video_movex(-escseq_get_param(1));
				break;
			case 'E': /* cursor to next line */
				video_movey(escseq_get_param(1));
				video_movesol();
				break;
			case 'F': /* cursor to previous line */
				video_movey(-escseq_get_param(1));
				video_movesol();
				break;
			case 'G': /* cursor horizontal absolute */
				video_setx(escseq_get_param(1)-1); /* one-indexed */
				break;
			case 'H': case 'f': /* horizonal and vertical position */
			{
				uint8_t y = escseq_get_param(1);
				uint8_t x = escseq_get_param(1);
				video_gotoxy(x-1, y-1);
				break;
			}
			case 'J': /* erase */
				video_erase(escseq_get_param(0));
				break;
			case 'K': /* erase in line */
				video_eraseline(escseq_get_param(0));
				break;
			case 'm': /* set graphic rendition */
				while (paramptr) /* read attributes until we reach the end */
				{
					uint8_t attr = escseq_get_param(0);
					if (attr == 0 || attr == 27)
						revvideo = 0;
					else if (attr == 7)
						revvideo = 0x80;
				}
				break;
				
			case 'M': 
			{
				int ii = escseq_get_param(1);
				while (ii--)
					video_deleteline();
			}
				break;
				
			case 'P':
			{
				int ii = escseq_get_param(1);
				while (ii--)
					video_deletechar();
			}
				break;
				
			case '@':
			{
				int ii = escseq_get_param(1);
				while (ii--)
					video_insertchar();
			}
				break;
				
			case 'r': /* set top and bottom margins */
			{
				uint8_t top = escseq_get_param(1);
				uint8_t bottom = escseq_get_param(TILES_HIGH);
				video_set_margins(top-1, bottom-1);
				break;
			}
				
			case 'S':
			{
				int ii = escseq_get_param(1);
				while (ii--)
					video_scrollup();
			}
				break;
				
			case 'T':
			{
				int ii = escseq_get_param(1);
				while (ii--)
					video_scrolldown();
			}
				break;
				
			case 'L':
			{
				int ii = escseq_get_param(1);
				while (ii--)
					video_insertline();
			}
				break;
				
			default: /* unknown */
				break;
		}
		
		in_esc = NOT_IN_ESC;
	}
}

void escseq_csi_start()
{
	paramch = 0;
	memset(paramstr, 0, MAX_ESC_LEN+1);
	paramptr = &paramstr[0];
}

uint8_t escseq_get_param(uint8_t defaultval)
{
	if (!paramptr)
		return defaultval;
	
	uint8_t val = defaultval;
	
	char *startptr = paramptr;
	/* get everything up to the semicolon, move past it */
	char *endptr = strchr(paramptr, ';');
	if (endptr)
	{
		*endptr = '\0'; /* replace the semicolon to make atoi stop here */
		paramptr = endptr+1;
	}
	else
		paramptr = NULL;
	
	/* ascii to integer, as long as the string isn't empty */
	/* default value is given if the string is empty */
	if (*startptr)
		val = atoi(startptr); /* will read up to the null */
	
	
	return val;
}

void save_term_state()
{
	savedstate.cx = video_getx();
	savedstate.cy = video_gety();
	savedstate.graphicchars = graphicchars;
	savedstate.revvideo = revvideo;
}

void restore_term_state()
{
	video_gotoxy(savedstate.cx, savedstate.cy);
	graphicchars = savedstate.graphicchars;
	revvideo = savedstate.revvideo;
}

void reset_term()
{
	graphicchars = 0;
	revvideo = 0;
	in_esc = 0;
	save_term_state();
}

void apply_config()
{
	uart_update();
	
	video_set_reverse(cfg_param_value(TC_REVVIDEO));
	
	/* cache the values from the config struct */
	newlineseq = cfg_param_value(TC_ENTERCHAR);
	process_escseqs = cfg_param_value(TC_ESCSEQS);
	local_echo = cfg_param_value(TC_LOCALECHO);
}

void app_setup()
{	
	in_setup = false;
	
	reset_term();
	cfg_set_defaults();
	
	apply_config();
	video_welcome();
	video_lfwd();
	video_show_cursor();
}

void send_newline()
{
	if (newlineseq & 2) /* CR */
		uart_putchar('\r');
	if (newlineseq & 1) /* LF */
		uart_putchar('\n');
}

void send_special_key(uint8_t key)
{
	if (key >= K_F1 && key <= K_PGDN)
	{
		int i=0;
		while(specialkeyseqs[key-K_F1][i])
		{
			while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
			uart_putchar(specialkeyseqs[key-K_F1][i]);
			i++;
		}
	}
}

void app_handle_key(uint8_t key)
{
	if (in_setup)
	{
		uint8_t finish = setup_handle_key(key);
		if (finish)
		{
			in_setup = false;
			if (finish == SETUP_SAVE) /* need to reapply settings */
				apply_config();
			setup_leave();
		}
	}
	else
	{
		if (key == K_NUMLK) /* start setup */
		{
			in_setup = true;
			setup_start();
		}
		else if (key == '\n') /* send appropriate newline sequence */
			send_newline();
		else if (key >= 0x80) /* special keys */
			send_special_key(key);
		else /* normal ASCII character */
			uart_putchar(key);
	}
}

