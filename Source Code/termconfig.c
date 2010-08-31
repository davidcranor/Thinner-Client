/*
 * termconfig.c
 * 
 
 hi
 * terminal configuration and setup screen
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


#include "termconfig.h"
#include "video.h"
#include "keycodes.h"

#include "stm32f10x.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define PARAM_NAME_LEN  16
#define PARAM_MAX_VALS  5
#define PARAM_VAL_LEN   5

typedef struct
{
  const char name[PARAM_NAME_LEN+1];
  const char valnames[PARAM_MAX_VALS][PARAM_VAL_LEN+1];
  uint32_t vals[PARAM_MAX_VALS];
  uint8_t numvals;
  uint8_t defaultval;
} termparam_t;

const termparam_t p_baudrate = {
  "Baud rate",
  { "4800", "9600", "19200", "38400", "57600" },
  { 4800, 9600, 19200, 38400, 57600 },
  5,
  4
};

const termparam_t p_databits = {
  "Word Length",
  { "8", "9" },
  { USART_WordLength_8b, USART_WordLength_9b },
  2,
  0
};

const termparam_t p_parity = {
  "Parity",
  { "N", "E", "O" },
  { USART_Parity_No, USART_Parity_Even, USART_Parity_Odd },
  3,
  0
};

const termparam_t p_stopbits = {
  "Stop bits",
  { "1", "2" },
  { USART_StopBits_1, USART_StopBits_2 },
  2,
  0
};

const termparam_t p_enterchar = {
  "Enter sends",
  { "CR", "LF", "CRLF" },
  { 0b10, 0b01, 0b11 },
  3,
  0
};

const termparam_t p_localecho = {
  "Local echo",
  { "Off", "On" },
  { 0, 1 },
  2,
  0
};

const termparam_t p_escseqs = {
  "Escape sequences",
  { "Off", "On" },
  { 0, 1 },
  2,
  1
};

const termparam_t p_revvideo = {
  "Reverse video",
  { "Off", "On" },
  { 0, 1 },
  2,
  0
};

static const termparam_t *params[] = {
  &p_baudrate,
  &p_databits,
  &p_parity,
  &p_stopbits,
  &p_enterchar,
  &p_localecho,
  &p_escseqs,
  &p_revvideo
};

static uint32_t profile1[TC_NUM_PARAMS];
static uint32_t profile2[TC_NUM_PARAMS];
static uint32_t profile1temp[TC_NUM_PARAMS];
static uint32_t profile2temp[TC_NUM_PARAMS];
static uint32_t *config;
static uint8_t profilenumber;

const char* cfg_param_name(uint8_t param)
{
 return (params[param]->name);
}

uint32_t cfg_param_value(uint8_t param)
{
  uint32_t val = config[param];
  return (params[param]->vals[val]);
}

const char* cfg_param_value_str(uint8_t param)
{
  uint8_t val = config[param];
  return (params[param]->valnames[val]);
}

void cfg_set_defaults()
{
  uint8_t i;
	config = profile1;
  for (i = 0; i < TC_NUM_PARAMS; i++)
    profile1[i] = profile2[i] = (params[i]->defaultval);
}

void cfg_print_line(uint8_t linenum)
{
  video_putcxy(0, linenum, '1'+profilenumber);
  video_putcxy(1, linenum, ']');

  video_putsxy(3,  linenum, cfg_param_value_str(TC_BAUDRATE));
  video_putsxy(9,  linenum, cfg_param_value_str(TC_DATABITS));
  video_putsxy(10, linenum, cfg_param_value_str(TC_PARITY));
  video_putsxy(11, linenum, cfg_param_value_str(TC_STOPBITS));
  video_putsxy(13, linenum, cfg_param_value_str(TC_ENTERCHAR));

  if (cfg_param_value(TC_ESCSEQS))
    video_putsxy(18, linenum, "ES");
  
  if (cfg_param_value(TC_LOCALECHO))
    video_putsxy(21, linenum, "LE");

  video_putsxy(TILES_WIDE-22, linenum, "(press NumLock to set)");
}

/***** Setup screen *****/
static int8_t currparam;
static uint8_t currprof;

static void setup_print_line(int8_t param)
{
  uint8_t linenum = 4 + 2*param;
  video_gotoxy(0, linenum);
  video_clrline();

  if (param == TC_NUM_PARAMS)
    video_putsxy(3, linenum, "Save");
  else if (param == -1)
  {
    video_putsxy(3, linenum, "Profile");
    video_putcxy(3+PARAM_NAME_LEN+3, linenum, '1'+currprof);
    if (currprof == profilenumber)
      video_putsxy(3+PARAM_NAME_LEN+5, linenum, "(active)");
  }
  else
  {
    video_putsxy(3, linenum, cfg_param_name(param));
    video_putsxy(3+PARAM_NAME_LEN+3, linenum, cfg_param_value_str(param));
  }

  /* Highlight with inverse video if this parameter is selected */
  if (currparam == param)
    video_invert_range(2, linenum, TILES_WIDE-4);

  /* Redraw the border */
  video_putcxy(0, linenum, '\x19');
  video_putcxy(TILES_WIDE-1, linenum, '\x19');
}

void setup_redraw()
{
  video_clrscr();

  int8_t i;
  for (i = -1; i < TC_NUM_PARAMS+1; i++)
    setup_print_line(i);
  
  /* Print border */
  for (i = 1; i < TILES_WIDE-1; i++)
  {
    video_putcxy(i, 0, '\x12');
    video_putcxy(i, TILES_HIGH-1, '\x12');
  }
  for (i = 1; i < TILES_HIGH-1; i++)
  {
    video_putcxy(0, i, '\x19');
    video_putcxy(TILES_WIDE-1, i, '\x19');
  }
  video_putcxy(0, 0, '\x0D');
  video_putcxy(TILES_WIDE-1, 0, '\x0C');
  video_putcxy(0, TILES_HIGH-1, '\x0E');
  video_putcxy(TILES_WIDE-1, TILES_HIGH-1, '\x0B');

  video_putsxy(5, TILES_HIGH-2, "\x03\x04: select     Enter: change     Esc: quit");
}

void setup_start()
{
  video_hide_cursor();

  currparam = -1;

  /* copy the current settings into the temps */
  memcpy(profile1temp, profile1, sizeof(profile1temp));
  memcpy(profile2temp, profile2, sizeof(profile1temp));
  config = (profilenumber) ? profile2temp : profile1temp;
  currprof = profilenumber;

  setup_redraw();
}

uint8_t setup_handle_key(uint8_t key)
{
  uint8_t ret = 0;

  switch (key)
  {
    case K_UP:
    {
      int8_t oldparam = currparam;
      currparam--;
      if (currparam < -1)
        currparam = TC_NUM_PARAMS;
      setup_print_line(oldparam);
      setup_print_line(currparam);
      break;
    }
    case K_DOWN:
    {
      int8_t oldparam = currparam;
      currparam++;
      if (currparam > TC_NUM_PARAMS)
        currparam = -1;
      setup_print_line(oldparam);
      setup_print_line(currparam);
      break;
    }
    case '\n':
    {
      if (currparam == TC_NUM_PARAMS) /* save and quit */
      {
        /* copy the temp settings back */
        memcpy(profile1, profile1temp, sizeof(profile1));
        memcpy(profile2, profile2temp, sizeof(profile2));
        config = (profilenumber) ? profile2 : profile1;
        ret = SETUP_SAVE;
      }
      else if (currparam == -1) /* change profile */
      {
        currprof = !currprof;
        config = (currprof) ? profile2temp : profile1temp;
        setup_redraw();
      }
      else
      {
        uint8_t maxval = (params[currparam]->numvals);
        config[currparam]++;
        if (config[currparam] >= maxval)
          config[currparam] = 0;
       setup_print_line(currparam);
      }
      break;
    }
    case '\x1B': /* ESC */
    case K_NUMLK:
    {
      config = (profilenumber) ? profile2 : profile1;
      ret = SETUP_CANCEL;
      break;
    }
  }

  return ret;
}

void setup_leave()
{
  video_welcome();
  cfg_print_line(video_gety());
  video_lfwd();
  video_show_cursor();
}
