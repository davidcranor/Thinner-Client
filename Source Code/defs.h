/*
 * defs.h
 * 
 * Global definitions
 *
 * Originally by Matt Sarnoff, 
 * hacked up by David Cranor and Max Lobovsky to work with STM32F and thinnerclient
 * 8/30/10
 *
 * (c) Massachusetts Institute of Technology 2010
 * Permission granted for experimental and personal use;
 * license for commercial sale available from MIT.
 */

#ifndef _DEFS_H_
#define _DEFS_H_

/* binary logarithm for compile-time constants */
#define LOG2(v)       (8 - 90/(((v)/4+14)|1) - 2/((v)/2+1))

#define PASTE(x,y)    x ## y

#define TILE_WIDTH    6   /* must be 8 or less! */
#define TILE_HEIGHT   8   /* must be a power of two! */
#define TILE_HBIT     LOG2(TILE_HEIGHT)
#define TILES_HIGH 25
#define TILES_WIDE 80
#define NUM_TILES     (TILES_WIDE*TILES_HIGH)
#define PIXELS_WIDE   (TILE_WIDTH*TILES_WIDE)
#define PIXELS_HIGH   (TILE_HEIGHT*TILES_HIGH)
#define NUM_LINES     PIXELS_HIGH

#define LEFT_MARGIN 3
#define TOP_MARGIN 2

#define FONT_6x8
#define FONT_HEIGHT 8

#endif
