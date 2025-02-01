#ifndef UI_H
#define UI_H

struct ui_button {
	int x, y;
	int width, height;
	const char *text;
	unsigned short outline_size;
	unsigned short outline_color;
	unsigned short fill_color;
	unsigned short text_color;
};

struct ui_progress_bar {
	int x;
	int y;
	int width;
	int height;
	int outline_size;
	unsigned short fill_color;
	unsigned short empty_color;
	unsigned short outline_color;
	float fill_percentage;
};

struct ui_spinner {
	int x;
	int y;
	int width;
	int height;
	int outline_size;
	float percentage;
	unsigned short fill_color;
	unsigned short bg_color;
	unsigned short outline_color;
};

int ui_center_text_x(const char *text, int container_x, int container_width);
int ui_center_text_y(int container_y, int container_height);
/**
 * ui_button_draw is a stateless button element.
 * it's up to the caller to manage state of the button.
 *
 * ui_button_draw just calls ui_button_fill, ui_button_label, and
 * ui_button_outline (in that order).  You can build your own button with the
 * similar function calls.
 *
 * given that it has no internal state, it's effectively a rounded rect with an
 * outline and a label (i.e. text that tries to center itself).
 */
void ui_button_draw(struct ui_button button);
void ui_button_fill(struct ui_button button, unsigned short color);
void ui_button_dither_fill(struct ui_button button, unsigned short color1,
	unsigned short color2, int dither_size);
void ui_button_draw_label(struct ui_button button, unsigned short color);
void ui_button_draw_outline(struct ui_button button, unsigned short color);

void ui_button_draw_label(struct ui_button button, unsigned short color);
void ui_button_draw_outline(struct ui_button button, unsigned short color);
void ui_button_draw(struct ui_button button);
void ui_progress_bar_draw_fill(struct ui_progress_bar bar);
void ui_progress_bar_draw_outline(struct ui_progress_bar bar);
void ui_progress_bar_draw(struct ui_progress_bar bar);

void ui_spinner_draw(struct ui_spinner spinner);
void ui_draw_spinner_bars(struct ui_spinner spinner);
void ui_draw_spinner_outline(struct ui_spinner spinner);

#endif
