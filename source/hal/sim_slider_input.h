#ifndef SIM_SLIDER_INPUT_H__
#define SIM_SLIDER_INPUT_H__

#include <stdint.h>
#include <SDL.h>

struct sim_slider_input;

/* Create a new slider input at x, y, with dimensions length, thickness, color,
   controlling value of *value (between 0 and 1), possibly vertical.
   The returned sim_slider_input is allocated with malloc and it is the
   responsibility of the caller to eventually free it.

   x, y, length and thickness are given as floats between 0 and 1 representing
   a fraction of the window's dimensions.
 */
struct sim_slider_input *slider_input_create(float x, float y, float length,
				float thickness, float *value, int vertical);

/* Draw the given slider */
void slider_input_draw(SDL_Window *w, SDL_Renderer *r, struct sim_slider_input *s);

/* When mouse button is pressed at (mousex, mousey) possibly influence slider value
 * returns 1 if mouse coords are within slider bounds, 0 otherwise
 */
int slider_input_button_press(SDL_Window *w, struct sim_slider_input *s, int mousex, int mousey);

void slider_input_set_color(struct sim_slider_input *s, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void slider_input_set_outline_color(struct sim_slider_input *s, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void slider_set_button_press_callback(struct sim_slider_input *s,
	void (*callback)(struct sim_slider_input *s, float value));

#endif
