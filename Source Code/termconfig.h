/*
 * termconfig.h
 * 
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


#ifndef _TERMCONFIG_H_
#define _TERMCONFIG_H_

#include <stdint.h>

enum
{
  TC_BAUDRATE,
  TC_DATABITS,
  TC_PARITY,
  TC_STOPBITS,
  TC_ENTERCHAR,
  TC_LOCALECHO,
  TC_ESCSEQS,
  TC_REVVIDEO,
  TC_NUM_PARAMS
};

#define SETUP_CANCEL  1
#define SETUP_SAVE    2

/* Get the name of the specified parameter */
const char* cfg_param_name(uint8_t param);

/* Get the value of the specified parameter in a configuration */
uint32_t cfg_param_value(uint8_t param);

/* Get the string representation of the value of the specified parameter */
const char* cfg_param_value_str(uint8_t param);

/* Set parameter values to their defaults */
void cfg_set_defaults();

/* Print the configuration summary at the specified line */
void cfg_print_line(uint8_t linenum);

/* Start the setup screen */
void setup_start();

/* Redraw the setup screen */
void setup_redraw(); 

/* Handle keystrokes on the setup screen */
uint8_t setup_handle_key(uint8_t key);

/* Leave the setup screen */
void setup_leave();
#endif
