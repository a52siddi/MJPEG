/*************************************************************************************
 * vim :set ts=8:
 ******************************************************************************/
#ifndef __SCREEN_H__
#define __SCREEN_H__

#include<stdint.h>

/*  Screen_init initializes the screen for JPEG visualization
 *  Parameters are :
 *  	width : width of the screen (image) in pixels
 *  	height : height of the screen (image) in pixels
 *  	framerate : maximum framerate for the screen (for mjpeg). Framerate is in frame/seconds
 */

extern void screen_init(uint32_t width, uint32_t height, uint32_t framerate);



/* screen_exit cleanly close the screen
*/

extern int screen_exit();



/* screen_cpyrect copies part of an image to the screen. Once copied, this part
 * is not immediately visible on the screen (see screen_refresh)
 * Parameters are :
 * 		x : width offset in number of columns (pixels) of the copied
 * 		        rectangle in the screen
 * 		y : height offset in number of lines (pixels) of the copied
 * 		        rectangle in the screen
 * 		w : width of the part to be copied
 * 		h : height of the part to be copied
 * 		ptr : pointer on the pixels in ARGB format
 * 	If DEBUG is defined, the function tests wether the rectangle is inside
 * 	the screen borders
 */

extern void screen_cpyrect( uint32_t x, uint32_t y,
		uint32_t w, uint32_t h,
		void *ptr);



/* screen_refresh is used to refresh the screen. Since copied data are not
 * immediately visible, it is possible to wait until the whole image is copied
 * before refreshing, to smooth the apparition of images
 */

extern int screen_refresh();

#endif

