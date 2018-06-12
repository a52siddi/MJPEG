/*************************************************************************************
 * vim :set ts=8:
 ******************************************************************************/
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdint.h"
#include "unistd.h"
#include "SDL/SDL.h"

//Define debug to print info when a block is copied out of bound
#define DEBUG

static SDL_Surface *screen;
static SDL_Rect ClipRect;
// According to when the quit event is detected, the player can get stuck
// the "initialized" parameter prevents this
static int initialized;
static uint32_t global_framerate;

// Frame rate management, stores previous counter value
static int old_time;


void screen_init(uint32_t width, uint32_t height, uint32_t framerate)
{

	int width_int = width , height_int = height;
	/* Initialize defaults, Video and Audio */
	if ((SDL_Init(SDL_INIT_VIDEO) == -1)) {
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		exit(-1);
	}

	/* Initialize the SDL library */
	if (SDL_Init(SDL_INIT_VIDEO) < 0)  {
		fprintf(stderr,
				"Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	/* Clean up on exit */
	atexit(SDL_Quit);
	if (framerate == 0) {
		global_framerate = 25;
	} else {
		global_framerate = framerate;
	}

	screen = SDL_SetVideoMode(width_int, height_int, 32, SDL_SWSURFACE);
	if (screen == NULL) {
		fprintf(stderr,
				"Couldn't set %dx%dx32 video mode for screen: %s\n",
				width, height, SDL_GetError());
		exit(1);
	}
	ClipRect.x = 0;
	ClipRect.y = 0;
	ClipRect.w = width;
	ClipRect.h = height;
	SDL_SetClipRect(screen, &ClipRect);

	old_time = SDL_GetTicks();
	initialized	= 1;
}

int screen_exit()
{
	/* Shutdown all subsystems */
	SDL_Event event;
	while(initialized) {
		if (SDL_PollEvent(&event)) {
			if ((event.type == SDL_QUIT )) {
				printf("\n");
				SDL_Quit();
				return 1;
			}
		}
		/*
		 * Something cleaner than polling would be nice
		 * but waiting for a better solution, don't
		 * waste all CPU time.
		 */
		sleep(0);
	}
	return 0;
}

void screen_cpyrect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, void *ptr)
{
	void *dest_ptr;
	void *src_ptr;
	uint32_t line;
	uint32_t w_internal = w, h_internal = h;
	int w_length;

	SDL_LockSurface(screen);
	w_length = screen->pitch / (screen->format->BitsPerPixel / 8 );

#ifdef DEBUG
	if ((y) > screen->h) {
		printf("[%s] : block can't be copied, "
				"not in the screen (too low)\n", __func__);
		exit(1);
	}
	if ((x) > screen->w) {
		printf("[%s] : block can't be copied, "
				"not in the screen (right border)\n", __func__);
		exit(1);
	}
#endif
	if ((x+w) > screen->w) {
		w_internal = screen->w -x;
	}
	if ((y+h) > screen->h) {
		h_internal = screen->h -y;
	}
	for(line = 0; line < h_internal ; line++)
	{
		// Positionning src and dest pointers
		// 	_ src : must be placed at the beginning of line "line"
		// 	_ dest : must be placed at the beginning
		// 	        of the corresponding block :
		//(line offset + current line + position on the current line)
		// We assume that RGB is 4 bytes

		dest_ptr = (void*)((uint32_t *)(screen->pixels) +
				((y+line)*w_length) + x);
		src_ptr = (void*)((uint32_t *)ptr + ((line * w)));
		memcpy(dest_ptr,src_ptr,w_internal * sizeof(uint32_t));
	}

	SDL_UnlockSurface(screen);
}

int screen_refresh()
{
  int delay;
	SDL_Rect offset;
	uint64_t finish_time;
	SDL_Event event;
	offset.x = 0;
	offset.y = 0;
	finish_time = SDL_GetTicks();

  //	while (SDL_GetTicks() - old_time < 1000 / global_framerate)
  delay = (1000/global_framerate) + old_time - SDL_GetTicks();
  if (delay > 0 )
    SDL_Delay(delay);

	if (SDL_Flip(screen) == -1) {
		printf("Could not refresh screen: %s\n.", SDL_GetError() );
	}
	//\r is required to avoid latency due to screening on terminal
    printf("\r[screen] : framerate is %0.2ffps, "
			"computed one image in %0.2fms",
			1000.00f / (SDL_GetTicks() - old_time),
			(finish_time - old_time) * 1.00f);
	old_time = SDL_GetTicks();
	// Check wether close event was detected, otherwise SDL freezes
	if(SDL_PollEvent(&event)) {
    if (event.type == SDL_KEYDOWN) {
      if (event.key.keysym.sym == SDLK_q) {
        printf("bye bye\n");
        initialized = 0;
        SDL_Quit();
        return 1;
      }
    }
		if (event.type == SDL_QUIT) {
			printf("\n");
			initialized = 0;
			SDL_Quit();
			return 1;
		}
	}
	return 0;
}



