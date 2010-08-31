/*
 * video.c
 * 
 * Output and cursor handling routines
 *
 * Originally by Matt Sarnoff, 
 * hacked up by David Cranor and Max Lobovsky (with some help from Peter Schmitt-Nielsen)
 * to work with STM32F and thinnerclient
 * 8/30/10
 *
 * (c) Massachusetts Institute of Technology 2010
 * Permission granted for experimental and personal use;
 * license for commercial sale available from MIT.
 */

#include <string.h>
#include "video.h"
#include "defs.h"

uint8_t tileMap[TILES_HIGH][TILES_WIDE];

static int8_t cx;
static int8_t cy;
static uint8_t showcursor;

/* Vertical margins */
static int8_t mtop;
static int8_t mbottom;

/* reverse video */
static uint8_t revvideo;

static void CURSOR_INVERT() __attribute__((noinline));
static void CURSOR_INVERT()
{
  tileMap[cy][cx] ^= showcursor;
}

void video_welcome()
{
  video_clrscr();

  /* Print border */
  uint8_t i;
  for (i = 1; i < TILES_WIDE-1; i++)
  {
    video_putcxy(i, 0, '\x12');
    video_putcxy(i, 2, '\x12');
  }
  video_putcxy(0, 1, '\x19');
  video_putcxy(TILES_WIDE-1, 1, '\x19');
  video_putcxy(0, 0, '\x0D');
  video_putcxy(TILES_WIDE-1, 0, '\x0C');
  video_putcxy(0, 2, '\x0E');
  video_putcxy(TILES_WIDE-1, 2, '\x0B');
  video_putsxy(2,1, "Welcome to the thinner client.");

  cx = 0;
  cy = 3;

  showcursor = 0;
}

void video_setup()
{
  revvideo = 0;
}

void puthex(uint8_t n)
{
	static char hexchars[] = "0123456789ABCDEF";
	char hexstr[5];
	hexstr[0] = hexchars[(n >> 4) & 0xF];
	hexstr[1] = hexchars[n & 0xF];
	hexstr[2] = hexstr[3] = ' ';
	hexstr[4] = '\0';
	video_puts(hexstr);
}

void sprinthex(char *str, uint8_t n)
{
	static char hexchars[] = "0123456789ABCDEF";
	str[0] = hexchars[(n >> 4) & 0xF];
	str[1] = hexchars[n & 0xF];
	str[2] = ' ';
}

//fill the tilemap with junk, useful for debugging
void filltileMap(void)
{
	uint16_t i;
	uint16_t j;
	for(i = 0; i<TILES_HIGH; i++)
	{
		for(j = 0; j<TILES_WIDE; j++)
		{
			tileMap[i][j] = j+39;
		}
	}
}

/****** Output routines ******/

//copy the tiles referenced in the tilemap into the frambuffer.  cramming 6 pixel wide tiles into a series of 16 bit halfwords is annoying.
void updateFrameBuffer(uint8_t map[][TILES_WIDE], uint8_t font[][FONT_HEIGHT])
{
	uint8_t i;
	uint8_t j;
	uint8_t k;
	uint32_t l;
	
	for(j = 0; j < TILES_HIGH ; j++)
	{
		for(k=0;k<8;k++)
		{
			l=0;
			for(i = 0; i < TILES_WIDE; i += 8 )
			{
				frameBuffer[(j+TOP_MARGIN)*FONT_HEIGHT+k][l++] = font[map[j][i]][k] << 8 | font[map[j][i+1]][k] << 2 | font[map[j][i+2]][k] >> 4;
				frameBuffer[(j+TOP_MARGIN)*FONT_HEIGHT+k][l++] = font[map[j][i+2]][k] << 12 | font[map[j][i+3]][k] << 6 | font[map[j][i+4]][k] | font[map[j][i+5]][k] >> 6;
				frameBuffer[(j+TOP_MARGIN)*FONT_HEIGHT+k][l++] = font[map[j][i+5]][k] << 10 | font[map[j][i+6]][k] << 4 | font[map[j][i+7]][k] >> 2;
			}
			frameBuffer[(j+TOP_MARGIN)*FONT_HEIGHT+k][BUFFER_LINE_LENGTH-1]=0;
		}
	}
}

void video_reset_margins()
{
  video_set_margins(0, TILES_HIGH-1);
}

void video_set_margins(int8_t top, int8_t bottom)
{
  /* sanitize input */
  if (top < 0) top = 0;
  if (bottom >= TILES_HIGH) bottom = TILES_HIGH-1;
  if (top >= bottom) { top = 0; bottom = TILES_HIGH-1; }

  mtop = top;
  mbottom = bottom;
  video_gotoxy(mtop, 0);
}

int8_t video_top_margin()
{
  return mtop;
}

int8_t video_bottom_margin()
{
  return mbottom;
}

void video_set_reverse(uint32_t val)
{
  revvideo = (val) ? 0x80 : 0;
}

static void _video_scrollup()
{
  memmove(&tileMap[mtop], &tileMap[mtop+1], (mbottom-mtop)*TILES_WIDE);
  memset(&tileMap[mbottom], revvideo, TILES_WIDE);
}

static void _video_scrolldown()
{
  memmove(&tileMap[mtop+1], &tileMap[mtop], (mbottom-mtop)*TILES_WIDE);
  memset(&tileMap[mtop], revvideo, TILES_WIDE);
}

void video_scrollup()
{
  CURSOR_INVERT();
  _video_scrollup();
  CURSOR_INVERT();
}

void video_scrolldown()
{
  CURSOR_INVERT();
  _video_scrolldown();
  CURSOR_INVERT();
}

void _video_insertline()
{
	if (cy < mbottom) {
		memmove(&tileMap[cy+1], &tileMap[cy], (mbottom-cy)*TILES_WIDE);
	}
	if (cy <= mbottom) {
		memset(&tileMap[cy], revvideo, TILES_WIDE);
	}
}

void video_insertline()
{
	CURSOR_INVERT();
	_video_insertline();
	CURSOR_INVERT();
}

void _video_deleteline()
{
	if (cy < mbottom) {
		memmove(&tileMap[cy], &tileMap[cy+1], (mbottom-cy)*TILES_WIDE);
	}
	if (cy <= mbottom) {
		memset(&tileMap[mbottom], revvideo, TILES_WIDE);
	}
}

void video_deleteline()
{
	CURSOR_INVERT();
	_video_deleteline();
	CURSOR_INVERT();
}

void _video_deletechar()
{
	memmove(&tileMap[cy][cx], &tileMap[cy][cx+1], TILES_WIDE-cx-1);
	tileMap[cy][TILES_WIDE-1] = revvideo;
}

void video_deletechar()
{
	CURSOR_INVERT();
	_video_deletechar();
	CURSOR_INVERT();
}

void _video_insertchar()
{
	memmove(&tileMap[cy][cx+1], &tileMap[cy][cx], TILES_WIDE-cx-1);
	tileMap[cy][cx] = revvideo;
}

void video_insertchar()
{
	CURSOR_INVERT();
	_video_insertchar();
	CURSOR_INVERT();
}


void video_movesol()
{
  CURSOR_INVERT();
  cx = 0;
  CURSOR_INVERT();
}

void video_setx(int8_t x)
{
  CURSOR_INVERT();
  cx = x;
  if (cx < 0) cx = 0;
  if (cx >= TILES_WIDE) cx = TILES_WIDE-1;
  CURSOR_INVERT();
}

/* Absolute positioning does not respect top/bottom margins */
void video_gotoxy(int8_t x, int8_t y)
{
  CURSOR_INVERT();
  cx = x;
  if (cx < 0) cx = 0;
  if (cx >= TILES_WIDE) cx = TILES_WIDE-1;
  cy = y;
  if (cy < 0) cy = 0;
  if (cy >= TILES_HIGH) cy = TILES_HIGH-1;
  CURSOR_INVERT();
}

void video_movex(int8_t dx)
{
  CURSOR_INVERT();
  cx += dx;
  if (cx < 0) cx = 0;
  if (cx >= TILES_WIDE) cx = TILES_WIDE-1;
  CURSOR_INVERT();
}

void video_movey(int8_t dy)
{
  CURSOR_INVERT();
  cy += dy;
  if (cy < mtop) cy = mtop;
  if (cy > mbottom) cy = mbottom;
  CURSOR_INVERT();
}

static void _video_lfwd()
{
  cx = 0;
  if (++cy > mbottom)
  {
    cy = mbottom;
    _video_scrollup();
  }
}

static inline void _video_cfwd()
{
  if (++cx > TILES_WIDE)
    _video_lfwd();
}

void video_cfwd()
{
  CURSOR_INVERT();
  _video_cfwd();
  CURSOR_INVERT();
}

void video_lfwd()
{
  CURSOR_INVERT();
  cx = 0;
  if (++cy > mbottom)
  {
    cy = mbottom;
    _video_scrollup();
  }
  CURSOR_INVERT();
}

void video_lf()
{
  CURSOR_INVERT();
  if (++cy > mbottom)
  {
    cy = mbottom;
    _video_scrollup();
  }
  CURSOR_INVERT();
}

static void _video_lback()
{
  cx = TILES_WIDE-1;
  if (--cy < 0)
  { cx = 0; cy = mtop; }
}

void video_lback()
{
  CURSOR_INVERT();
  cx = TILES_WIDE-1;
  if (--cy < 0)
  { cx = 0; cy = mtop; }
  CURSOR_INVERT();
}

void video_cback()
{
  CURSOR_INVERT();
  if (--cx < 0)
    _video_lback();
  CURSOR_INVERT();
}

int8_t video_getx()
{
  return cx;
}

int8_t video_gety()
{
  return cy;
}

char video_charat(int8_t x, int8_t y)
{
  return tileMap[y][x];
}

void video_clrscr()
{
  CURSOR_INVERT();
  video_reset_margins(); 
  memset(tileMap, revvideo, TILES_WIDE*TILES_HIGH);
  cx = cy = 0;
  CURSOR_INVERT();
}

void video_clrline()
{
  CURSOR_INVERT();
  memset(&tileMap[cy], revvideo, TILES_WIDE);
  cx = 0;
  CURSOR_INVERT();
}

void video_clreol()
{
  memset(&tileMap[cy][cx], revvideo, TILES_WIDE-cx);
}

void video_erase(uint8_t erasemode)
{
  CURSOR_INVERT();
  switch(erasemode)
  {
    case 0: /* erase from cursor to end of screen */
      memset(&tileMap[cy][cx], revvideo,
          (TILES_WIDE*TILES_HIGH)-(cy*TILES_WIDE+cx));
      break;
    case 1: /* erase from beginning of screen to cursor */
      memset(tileMap, revvideo, cy*TILES_WIDE+cx+1);
      break;
    case 2: /* erase entire screen */
      memset(tileMap, revvideo, TILES_WIDE*TILES_HIGH);
      break;
  }
  CURSOR_INVERT();
}

void video_eraseline(uint8_t erasemode)
{
  CURSOR_INVERT();
  switch(erasemode)
  {
    case 0: /* erase from cursor to end of line */
      memset(&tileMap[cy][cx], revvideo, TILES_WIDE-cx);
      break;
    case 1: /* erase from beginning of line to cursor */
      memset(&tileMap[cy], revvideo, cx+1);
      break;
    case 2: /* erase entire line */
      memset(&tileMap[cy], revvideo, TILES_WIDE);
      break;
  }
  CURSOR_INVERT();
}

/* Does not respect top/bottom margins */
void video_putcxy(int8_t x, int8_t y, char c)
{
  if (x < 0 || x >= TILES_WIDE) return;
  if (y < 0 || y >= TILES_HIGH) return;
  tileMap[y][x] = c ^ revvideo;
}

/* Does not respect top/bottom margins */
void video_putsxy(int8_t x, int8_t y, const char *str)
{
  if (x < 0 || x >= TILES_WIDE) return;
  if (y < 0 || y >= TILES_HIGH) return;
  int len = strlen(str);
  if (len > TILES_WIDE-x) len = TILES_WIDE-x;
  memcpy((char *)(&tileMap[y][x]), str, len);
  if (revvideo) video_invert_range(x, y, len);
}

/* Does not respect top/bottom margins */
void video_putline(int8_t y, char *str)
{
  if (y < 0 || y >= TILES_HIGH) return;
  /* strncpy fills unused bytes in the destination with nulls */
  strncpy((char *)(&tileMap[y]), str, TILES_WIDE);
  if (revvideo) video_invert_range(0, y, TILES_WIDE);
}

void video_setc(char c)
{
  CURSOR_INVERT();
  tileMap[cy][cx] = c ^ revvideo;
  CURSOR_INVERT();
}

static inline void _video_putc(char c)
{
  /* If the last character printed exceeded the right boundary,
   * we have to go to a new line. */
  if (cx >= TILES_WIDE) _video_lfwd();

  if (c == '\r') cx = 0;
  else if (c == '\n') _video_lfwd();
  else
  {
    tileMap[cy][cx] = c ^ revvideo;
    _video_cfwd();
  }
}

void video_putc(char c)
{
  CURSOR_INVERT();
  _video_putc(c);
  CURSOR_INVERT();
}

void video_putc_raw(char c)
{
  CURSOR_INVERT();
  
  /* If the last character printed exceeded the right boundary,
   * we have to go to a new line. */
  if (cx >= TILES_WIDE) _video_lfwd();
  
  tileMap[cy][cx] = c ^ revvideo;
  _video_cfwd();
  CURSOR_INVERT();
}

void video_puts(char *str)
{
  /* Characters are interpreted and printed one at a time. */
  char c;
  CURSOR_INVERT();
  while ((c = *str++))
    _video_putc(c);
  CURSOR_INVERT();
}

void video_show_cursor()
{
  if (!showcursor)
  {
    showcursor = 0x80;
    CURSOR_INVERT();
  }
}

void video_hide_cursor()
{
  if (showcursor)
  {
    CURSOR_INVERT();
    showcursor = 0;
  }
}

uint8_t video_cursor_visible()
{
  return showcursor != 0;
}

void video_invert_range(int8_t x, int8_t y, uint8_t rangelen)
{
  uint8_t *start = &tileMap[y][x];
  uint8_t i;
  for (i = 0; i < rangelen; i++)
  {
    *start ^= 0x80;
    start++;
  }
}

