#include <string.h>

#include "framebuffer.h"
#include "ui.h"

#define PI 3.14159265358979323846

static void draw_button_fill(struct ui_button button, unsigned short color)
{
	FbColor(color);
	FbMove(button.x + button.outline_size, button.y + button.outline_size);
	FbFilledRectangle(button.width - button.outline_size * 2,
		button.height - button.outline_size * 2);
}

static void draw_button_dither_fill(struct ui_button button,
	unsigned short color1, unsigned short color2, int dither_size)
{
	if (dither_size < 1) {
		dither_size = 1;
	}

	for (int y = 0; y < button.height - button.outline_size * 2;
		y += dither_size) {
		for (int x = 0; x < button.width - button.outline_size * 2;
			x += dither_size) {
			unsigned short current_color =
				(((x / dither_size) + (y / dither_size)) % 2 ==
					0)
				? color1
				: color2;

			FbMove(button.x + button.outline_size + x,
				button.y + button.outline_size + y);
			FbColor(current_color);
			FbFilledRectangle(dither_size, dither_size);
		}
	}
}

int ui_center_text_x(const char *text, int container_x, int container_width)
{
	int text_length = strlen(text);
	return container_x + (container_width / 2) - (text_length * 4);
}

int ui_center_text_y(int container_y, int container_height)
{
	return container_y + (container_height / 2) - 4;
}

static void draw_button_label(struct ui_button button, unsigned short color)
{
	int text_length = strlen(button.text);
	int text_x = ui_center_text_x(button.text, button.x, button.width);
	int text_y = ui_center_text_y(button.y, button.height);

	if (text_length == 1) {
		text_x = text_x + 1;
	}

	FbMove(text_x, text_y);
	FbColor(color);
  FbTransparentIndex(0);
	FbWriteString(button.text);
}

static void draw_button_outline(struct ui_button button, unsigned short color)
{
	FbColor(color);
	FbMove(button.x, button.y);
	FbRoundedRect(button.width, button.height, button.outline_size);
}

void ui_button_fill(struct ui_button button, unsigned short color)
{
	draw_button_fill(button, color);
}

void ui_button_dither_fill(struct ui_button button, unsigned short color1,
	unsigned short color2, int dither_size)
{
	draw_button_dither_fill(button, color1, color2, dither_size);
}

void ui_button_draw_label(struct ui_button button, unsigned short color)
{
	draw_button_label(button, color);
}

void ui_button_draw_outline(struct ui_button button, unsigned short color)
{
	draw_button_outline(button, color);
}

void ui_button_draw(struct ui_button button)
{
	draw_button_fill(button, button.fill_color);
	draw_button_outline(button, button.outline_color);
	draw_button_label(button, button.text_color);
}

static void draw_progress_bar_fill(struct ui_progress_bar bar)
{
	if (bar.fill_percentage < 0) {
		bar.fill_percentage = 0;
	}
	if (bar.fill_percentage > 1.0) {
		bar.fill_percentage = 1.0;
	}
	int fill_width =
		(int)((bar.width - bar.outline_size * 2) * bar.fill_percentage);

	FbColor(bar.fill_color);
	FbMove(bar.x + bar.outline_size, bar.y + bar.outline_size);
	FbFilledRectangle(fill_width, bar.height - bar.outline_size * 2);

	if (fill_width < bar.width - bar.outline_size * 2) {
		FbColor(bar.empty_color);
		FbMove(bar.x + bar.outline_size + fill_width,
			bar.y + bar.outline_size);
		FbFilledRectangle(bar.width - bar.outline_size * 2 - fill_width,
			bar.height - bar.outline_size * 2);
	}
}

static void draw_progress_bar_outline(struct ui_progress_bar bar)
{
	FbColor(bar.outline_color);
	FbMove(bar.x, bar.y);
	FbRoundedRect(bar.width, bar.height, bar.outline_size);
}

void ui_progress_bar_draw_fill(struct ui_progress_bar bar)
{
	draw_progress_bar_fill(bar);
}

void ui_progress_bar_draw_outline(struct ui_progress_bar bar)
{
	draw_progress_bar_outline(bar);
}

void ui_progress_bar_draw(struct ui_progress_bar bar)
{
	draw_progress_bar_fill(bar);
	draw_progress_bar_outline(bar);
}

void ui_draw_spinner_outline(struct ui_spinner spinner)
{
	FbColor(spinner.outline_color);
	FbMove(spinner.x - spinner.width / 2, spinner.y - spinner.height / 2);
	FbRoundedRect(spinner.width, spinner.height, spinner.outline_size);
}

static void draw_spinner_bar(int x, int y, int bar_width, int bar_height,
	unsigned short fill_color, unsigned short empty_color, double start_pct,
	double end_pct, double spinner_pct, int reverse)
{
	double effective_pct = spinner_pct < start_pct ? 0.0
		: spinner_pct > end_pct
		? 1.0
		: (spinner_pct - start_pct) / (end_pct - start_pct);

	int fill_length = (int)(effective_pct *
		(bar_width > bar_height ? bar_width : bar_height));

	FbColor(fill_color);
	if (bar_width > bar_height) {
		if (reverse) {
			FbMove(x + bar_width - fill_length, y);
			FbFilledRectangle(fill_length, bar_height);
		} else {
			FbMove(x, y);
			FbFilledRectangle(fill_length, bar_height);
		}
	} else {
		if (reverse) {
			FbMove(x, y);
			FbFilledRectangle(bar_width, fill_length);
		} else {
			FbMove(x, y + bar_height - fill_length);
			FbFilledRectangle(bar_width, fill_length);
		}
	}

	if (fill_length < (bar_width > bar_height ? bar_width : bar_height)) {
		FbColor(empty_color);
		if (bar_width > bar_height) {
			if (reverse) {
				FbMove(x, y);
				FbFilledRectangle(
					bar_width - fill_length, bar_height);
			} else {
				FbMove(x + fill_length, y);
				FbFilledRectangle(
					bar_width - fill_length, bar_height);
			}
		} else {
			if (reverse) {
				FbMove(x, y + fill_length);
				FbFilledRectangle(
					bar_width, bar_height - fill_length);
			} else {
				FbMove(x, y);
				FbFilledRectangle(
					bar_width, bar_height - fill_length);
			}
		}
	}
}

void ui_draw_spinner_bars(struct ui_spinner spinner)
{
	int half_width = spinner.width / 2;
	int half_height = spinner.height / 2;
	int bar_thickness = spinner.outline_size + 2;
	int offset = spinner.outline_size / 2 + bar_thickness / 2 - 2;

	draw_spinner_bar(spinner.x - half_width + offset,
		spinner.y - half_height + offset, spinner.width - 2 * offset,
		bar_thickness, spinner.fill_color, spinner.bg_color, 0.0, 0.25,
		spinner.percentage, 0);
	draw_spinner_bar(spinner.x + half_width - offset - bar_thickness,
		spinner.y - half_height + offset, bar_thickness,
		spinner.height - 2 * offset, spinner.fill_color,
		spinner.bg_color, 0.25, 0.50, spinner.percentage, 1);
	draw_spinner_bar(spinner.x - half_width + offset,
		spinner.y + half_height - offset - bar_thickness,
		spinner.width - 2 * offset, bar_thickness, spinner.fill_color,
		spinner.bg_color, 0.50, 0.75, spinner.percentage, 1);
	draw_spinner_bar(spinner.x - half_width + offset,
		spinner.y - half_height + offset, bar_thickness,
		spinner.height - 2 * offset, spinner.fill_color,
		spinner.bg_color, 0.75, 1.0, spinner.percentage, 0);
}

void ui_spinner_draw(struct ui_spinner spinner)
{
	ui_draw_spinner_bars(spinner);
	ui_draw_spinner_outline(spinner);
}
