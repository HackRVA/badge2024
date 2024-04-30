#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL.h>

#include "sim_slider_input.h"

struct sim_slider_input {
	float x, y, length, thickness;
	int vertical;
	float *value;
	uint8_t r, g, b, a;
	uint8_t or, og, ob, oa;
	void (*callback)(struct sim_slider_input *s, float v);
};

/* Create a new slider input at x, y, with dimensions length, thickness, color,
   controlling value of *value, possibly vertical. */
struct sim_slider_input *slider_input_create(float x, float y, float length, float thickness,
						float *value, int vertical)
{
	struct sim_slider_input *s = malloc(sizeof(*s));
	s->x = x;
	s->y = y;
	s->length = length;
	s->thickness = thickness;
	s->value = value;
	if (*s->value > 1.0)
		*s->value = 1.0;
	if (*s->value < 0.0)
		*s->value = 0.0;
	s->vertical = vertical;

	/* default color is white */
	s->r = 255;
	s->g = 255;
	s->b = 255;
	s->a = 255;
	s->callback = NULL;

	return s;
}

static void slider_get_dimensions(SDL_Window *window, struct sim_slider_input *s,
				int *x, int *y, int *w, int *h)
{
	int width, height;

	SDL_GetWindowSize(window, &width, &height);

	float length = s->length * height;
	float thickness = s->thickness * width;

	if (s->vertical) {
		*x = (int) (s->x * width);
		*y = (int) (s->y * height);
		*w = (int) thickness;
		*h = (int) length;
	} else {
		*x = (int) (s->x * width);
		*y = (int) (s->y * height);
		*w = (int) length;
		*h = (int) thickness;
	}
}

/* Draw the given slider */
void slider_input_draw(SDL_Window *w, SDL_Renderer *r, struct sim_slider_input *s)
{
	int width, height;
	SDL_Rect rect, inner_rect;

	float v = *s->value;
	if (v > 1.0)
		v = 1.0;
	if (v < 0.0)
		v = 0.0;

	SDL_GetWindowSize(w, &width, &height);
	if (s->vertical) {
		slider_get_dimensions(w, s, &rect.x, &rect.y, &rect.w, &rect.h);
		inner_rect.x = rect.x;
		inner_rect.y = rect.y;
		inner_rect.w = rect.w;
		inner_rect.h = (int) (v * rect.h);
	} else {
		slider_get_dimensions(w, s, &rect.x, &rect.y, &rect.w, &rect.h);
		inner_rect.x = rect.x;
		inner_rect.y = rect.y;
		inner_rect.w = (int) (v * rect.w);
		inner_rect.h = rect.h;
	}
	SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
	SDL_RenderFillRect(r, &rect);
	SDL_SetRenderDrawColor(r, s->r, s->g, s->b, s->a);
	SDL_RenderFillRect(r, &inner_rect);
	SDL_SetRenderDrawColor(r, s->or, s->og, s->ob, s->oa);
	SDL_RenderDrawRect(r, &rect);
}

/* When mouse button is pressed at (mousex, mousey) possibly influence slider value
 * returns 1 if mouse coords are within slider bounds, 0 otherwise
 */
int slider_input_button_press(SDL_Window *window, struct sim_slider_input *s, int mousex, int mousey)
{
	int x, y, w, h;
	float newv;

	slider_get_dimensions(window, s, &x, &y, &w, &h);
	if (mousex < x)
		return 0;
	if (mousey < y)
		return 0;
	if (mousex > x + w)
		return 0;
	if (mousey > y + h)
		return 0;

	if (s->vertical)
		newv = (float) (mousey - y) / (float) h;
	else
		newv = (float) (mousex - x) / (float) w;
	*s->value = newv;
	if (s->callback)
		s->callback(s, newv);
	return 1;
}

void slider_input_set_color(struct sim_slider_input *s, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	s->r = r;
	s->g = g;
	s->b = b;
	s->a = a;
}

void slider_input_set_outline_color(struct sim_slider_input *s, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	s->or = r;
	s->og = g;
	s->ob = b;
	s->oa = a;
}


void slider_set_button_press_callback(struct sim_slider_input *s,
	void (*callback)(struct sim_slider_input *s, float value))
{
	s->callback = callback;
}

