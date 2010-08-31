#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <SDL.h>

// Read at most one frame in one sitting.
#define MAX_READ_IN_ONE_SITTING (14400)

int WIDTH, HEIGHT;

#define BPP 4
#define DEPTH 32

char keys_held[1024] = {0};

int can_read( int fd ) {
	fd_set sready;
	struct timeval nowait;

	FD_ZERO(&sready);
	FD_SET((unsigned int)fd,&sready);
	memset((char *)&nowait,0,sizeof(nowait));

	select(fd+1,&sready,NULL,NULL,&nowait);

	if( FD_ISSET(fd,&sready) ) {
		return 1;
	}
	else {
		return 0;
	}
}

inline void setpixel(SDL_Surface *screen, int x, int y, Uint32 colour) {
	Uint32 *pixmem32;

	if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;

	y = y*screen->pitch/BPP;

	pixmem32 = (Uint32*) screen->pixels + y + x;
	*pixmem32 = colour;
}

void DrawScreen(SDL_Surface* screen) {

	const Uint32 white = SDL_MapRGB( screen->format, 255, 255, 255 );
	const Uint32 black = SDL_MapRGB( screen->format, 0, 0, 0 );

	if(SDL_MUSTLOCK(screen)) 
	{
		if(SDL_LockSurface(screen) < 0) return;
	}

	//SDL_FillRect(screen, NULL, black);

	static int xx = 0;
	static int yy = 0;

	unsigned char monochrome_packed_byte;
	int read_more = MAX_READ_IN_ONE_SITTING;
	while (can_read(0) && (read_more--)) {
		read(0, &monochrome_packed_byte, 1);

		int ii;
		for (ii=0; ii<8; ii++) {
			if (monochrome_packed_byte & 0x80) {
				setpixel(screen, xx, yy, white);
			} else {
				setpixel(screen, xx, yy, black);
			}
			monochrome_packed_byte <<= 1;
			xx++;
		}
		if (xx >= WIDTH) {
			xx = 0;
			yy++;
		}
		if (yy >= HEIGHT) {
			yy = 0;
		}
	}

	if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);

	SDL_Flip(screen); 
}

SDL_Surface *load_image( const char *path ) {
	SDL_Surface *image;
	SDL_Surface *temp;

	temp = SDL_LoadBMP(path);
	if (temp == NULL) {
		printf("Unable to load bitmap: %s\n", SDL_GetError());
		return NULL;
	}

	image = SDL_DisplayFormat(temp);
	if (image == NULL) {
		printf("Unable to convert bitmap to current display format: %s\n", SDL_GetError());
		return NULL;
	}
	SDL_FreeSurface(temp);

	return image;
}

int main(int argc, char **argv) {
	SDL_Surface *screen;
	SDL_Event event;
	const SDL_VideoInfo *info;
	int continue_running = 1;

	if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;

	info   = SDL_GetVideoInfo();
	if (info == NULL) {
		printf("Unable to get video info: %s\n", SDL_GetError());
		return 1;
	}
	WIDTH  = info->current_w;
	HEIGHT = info->current_h;

	WIDTH  = 480;
	HEIGHT = 240;

	if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_HWSURFACE)))
	{
		SDL_Quit();
		return 1;
	}

	while(continue_running) 
	{
		DrawScreen(screen);
		while(SDL_PollEvent(&event)) 
		{	  
			switch (event.type) 
			{
				case SDL_QUIT:
					continue_running = 0;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if( event.button.button == SDL_BUTTON_LEFT ) {
					}
					break;
				case SDL_KEYDOWN:
					if (event.key.keysym.sym >= 0 && event.key.keysym.sym < 1024)
						keys_held[event.key.keysym.sym] = 1;
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						continue_running = 0;
					}
					break;
				case SDL_KEYUP:
					if (event.key.keysym.sym >= 0 && event.key.keysym.sym < 1024)
						keys_held[event.key.keysym.sym] = 0;
					break;
			}
		}
	}

	SDL_Quit();

	return 0;
}

