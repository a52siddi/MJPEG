/******************************************************************************
 * vim :set ts=8:
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <malloc.h>

#include "screen.h"
#include <SDL/SDL.h>


#if (SDL_BYTEORDER != SDL_BIG_ENDIAN)
#define WHITE  0x00ffffff
#define BLUE  0x000000ff
#define GREEN  0x0000ff00
#define RED  0x00ff0000
#define BLACK  0x00000000
#else
#define WHITE  0xffffff00
#define BLUE  0xff000000
#define GREEN  0x00ff0000
#define RED  0x0000ff00
#define BLACK  0x00000000
#endif


int main(int argc, char *argv[])
{
	uint32_t colorblock[60000];
	uint32_t i = 0;
	/* On veut remplir une mire avec 7 bandes de couleur de 100 pixels de
	 * large chacune, et de 600 pixels de haut
	 *  On initialise donc un écran de largeur 700, et de hauteur 600
	 */
	screen_init(700, 600, 25);

	/* On veut ensuite copier les bandes de couleur, à la bonne place dans
	 * l'image.
	 * Pour cela, on utilise la fonction screen_cpyrect. On initialise
	 * également le bloc à copier
	 */
	for (i = 0 ; i < 60000 ; i++) {
		colorblock[i] = WHITE;
	}
	screen_cpyrect(0,0,100,600, colorblock);
	for (i = 0 ; i < 60000 ; i++) {
		colorblock[i] = RED;
	}
	screen_cpyrect(100,0,100,600, colorblock);
	for (i = 0 ; i < 60000 ; i++) {
		colorblock[i] = RED | BLUE;
	}
	screen_cpyrect(200,0,100,600, colorblock);
	for (i = 0 ; i < 60000 ; i++) {
		colorblock[i] = BLUE;
	}
	screen_cpyrect (300,0,100,600, colorblock);
	for (i = 0 ; i < 60000 ; i++) {
		colorblock[i] = BLUE | GREEN;
	}
	screen_cpyrect (400,0,100,600, colorblock);
	for (i = 0 ; i < 60000 ; i++) {
		colorblock[i] = GREEN;
	}
	screen_cpyrect (500,0,100,600, colorblock);
	for (i = 0 ; i < 60000 ; i++) {
		colorblock[i] = BLACK;
	}
	screen_cpyrect (600,0,100,600, colorblock);
	// Les 7 bandes ont été copiés, on peut maintenant afficher l'image
	screen_refresh();
	/* Une fois rafraichit, on ferme l'écran. Cette fonction attend en fait
	 * que l'on ferme explicitement la fenetre
	 */
	screen_exit();
	return 0;
}

