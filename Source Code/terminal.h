/*
 * terminal.h
 * 
 * Serial terminal and ANSI/VT100 escape sequence handling
 *
 * Originally by Matt Sarnoff, 
 * hacked up by David Cranor to work with STM32F and thinnerclient
 * 8/30/10
 *
 * (c) Massachusetts Institute of Technology 2010
 * Permission granted for experimental and personal use;
 * license for commercial sale available from MIT.
 */

#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include <stdint.h>

void app_setup();
void app_handle_key(uint8_t key);
void receive_char(uint8_t c);

#endif
