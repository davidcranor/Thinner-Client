/*
 * video.h
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


#ifndef _VIDEO_H_
#define _VIDEO_H_

#include "defs.h"
#include "thinnerclient.h"
#include <stdint.h>


extern uint8_t tileMap[TILES_HIGH][TILES_WIDE];

void video_setup();

/****** Output routines ******/

//Actually fill the framebuffer with the tiles referenced in the tilemap.
void updateFrameBuffer(uint8_t map[][TILES_WIDE], uint8_t font[][FONT_HEIGHT]);

/*Fills the tilemap with stuff, useful for debugging.*/
void filltileMap(void);

/* Clears the screen and prints the welcome message. */
void video_welcome();

void puthex(uint8_t n);

void sprinthex(char *str, uint8_t n);

/* Set the top and bottom margins. The cursor is moved to the first column
 * of the top margin. */
void video_set_margins(int8_t top, int8_t bottom);

/* Resets the top margin to the top line of the screen and the bottom margin
 * to the bottom line of the screen. */
void video_reset_margins();

/* Returns the line number of the top margin. */
int8_t video_top_margin();

/* Returns the line number of the bottom margin. */
int8_t video_bottom_margin();

/* Sets whether or not the screen should be displayed in reverse video. */
void video_set_reverse(uint32_t val);

/* Clears the screen, returns the cursor to (0,0), and resets the margins
 * to the full size of the screen. */
void video_clrscr();

/* Clears the current line and returns the cursor to the start of the line. */
void video_clrline();

/* Clears the rest of the line from the cursor position to the end of the line
 * without moving the cursor.*/
void video_clreol();

/* erasemode = 0: erase from the cursor (inclusive) to the end of the screen.
 * erasemode = 1: erase from the start of the screen to the cursor (inclusive).
 * erasemode = 2: erase the entire screen.
 * The cursor does not move.
 * This call corresponds to the ANSI "Erase in Display" escape sequence. */
void video_erase(uint8_t erasemode);

/* erasemode = 0: erase from the cursor (inclusive) to the end of the line.
 * erasemode = 1: erase from the start of the line to the cursor (inclusive).
 * erasemode = 2: erase the entire line.
 * The cursor does not move.
 * This call corresponds to the ANSI "Erase in Line" escape sequence. */
void video_eraseline(uint8_t erasemode);

/* Overwrites the character at the cursor position without moving it. */
void video_setc(char c);

/* Prints a character at the cursor position and advances the cursor.
 * Carriage returns and newlines are interpreted. */
void video_putc(char c);

/* Prints a character at the cursor position and advances the cursor.
 * Carriage returns and newlines are not interpreted. */
void video_putc_raw(char c);

/* Prints a string at the cursor position and advances the cursor.
 * The screen will be scrolled if necessary. */
void video_puts(char *str);

/* Prints a character at the specified position. The cursor is not advanced. */
void video_putcxy(int8_t x, int8_t y, char c);

/* Prints a string at the specified position. Escape characters are not
 * interpreted. The cursor is not advanced and the screen is not scrolled. */
void video_putsxy(int8_t x, int8_t y, const char *str);

/* Prints a string on the specified line. The previous contents of the line
 * are erased. Escape characters are not interpreted. The cursor is not
 * advanced and the screen is not scrolled. */
void video_putline(int8_t y, char *str);

/* Moves the cursor to the specified coordinates. 
 * This function can be used to move the cursor outside of the margins. */
void video_gotoxy(int8_t x, int8_t y);

/* Moves the cursor left/right by the specified number of columns. */
void video_movex(int8_t dx);

/* Moves the cursor up/down the specified number of lines. 
 * The cursor does not move beyond the top/bottom margins. */
void video_movey(int8_t dy);

/* Moves the cursor to the start of the current line. */
void video_movesol();

/* Sets the horizontal position of the cursor. */
void video_setx(int8_t x);

/* Advances the cursor one character to the right, advancing to a new line if
 * necessary. */
void video_cfwd();

/* Advances the cursor one line down and moves it to the start of the new line.
 * The screen is scrolled if the bottom margin is exceeded. */
void video_lfwd();

/* Advances the cursor one line down but does not return the cursor to the start
 * of the new line. The screen is scrolled if the bottom margin is exceeded. */
void video_lf();

/* Moves the cursor one character back, moving to the end of the previous line
 * if necessary. */
void video_cback();

/* Moves the cursor to the end of the previous line, or to the first column
 * of the top margin if the top margin is exceeded. */
void video_lback();

/* Scrolls the region between the top and bottom margins up one line.
 * A blank line is added at the bottom. The cursor is not moved. */
void video_scrollup();

/* Scrolls the region between the top and bottom margins down one line.
 * A blank lines is added at the top. The cursor is not moved. */
void video_scrolldown();

/* Inserts a blank line at cursor position by scrolling everything below the cursor down a line.
 * The cursor is not moved. */
void video_insertline();

/* Deletes line at cursor position by scrolling everything below the cursor up a line and
 * erasing the bottom line of the screen.  The cursor is not moved. */
void video_deleteline();

/* Deletes character at cursor position by scrolling everything to the right of it to the left one place.
 * The cursor is not moved. */
void video_deletechar();

/* Inserts a place for a character at cursor position by scrolling to the right of it to the right one place.
 * The cursor is not moved. */
void video_insertchar();

/* Returns the x coordinate of the cursor. */
int8_t video_getx();

/* Returns the y coordinate of the cursor. */
int8_t video_gety();

/* Returns the character at the specified position. */
char video_charat(int8_t x, int8_t y);

/* Shows the cursor. Off by default. */
void video_show_cursor();

/* Hides the cursor. */
void video_hide_cursor();

/* Returns 1 if the cursor is visible, 0 if it is hidden. */
uint8_t video_cursor_visible();

/* Set inverse video for the character range specified. */
void video_invert_range(int8_t x, int8_t y, uint8_t rangelen);
#endif
