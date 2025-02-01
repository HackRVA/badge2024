#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "colors.h"
#include "framebuffer.h"
#include "palette.h"

static const uint32_t default_palette[16] = {
	PACKRGB888(0, 0, 0),
	PACKRGB888(127, 36, 84),
	PACKRGB888(28, 43, 83),
	PACKRGB888(0, 135, 81),
	PACKRGB888(171, 82, 54),
	PACKRGB888(96, 88, 79),
	PACKRGB888(195, 195, 198),
	PACKRGB888(255, 241, 233),
	PACKRGB888(237, 27, 81),
	PACKRGB888(250, 162, 27),
	PACKRGB888(247, 236, 47),
	PACKRGB888(93, 187, 77),
	PACKRGB888(81, 166, 220),
	PACKRGB888(131, 118, 156),
	PACKRGB888(241, 118, 166),
	PACKRGB888(252, 204, 171),
};

static uint32_t loaded_palette[16] = {0};
static uint8_t palette_replacements[16] = {0};

void palette_init(void)
{
	memcpy(loaded_palette, default_palette, sizeof(default_palette));
	for (int i = 0; i < 16; i++) {
		palette_replacements[i] = i;
	}
}

uint32_t *palette_get_loaded_palette(void)
{
	return loaded_palette;
}

uint16_t palette_get_color(uint8_t index)
{
	uint8_t replaced_index = palette_replacements[index];
	return palette_color_from_index(replaced_index);
}

void palette_load(const uint32_t palette[16])
{
	for (int i = 0; i < 16; i++) {
		loaded_palette[i] = palette[i];
	}
}

void palette_reset(void)
{
	memcpy(loaded_palette, default_palette, sizeof(default_palette));
	for (int i = 0; i < 16; i++) {
		palette_replacements[i] = i;
	}
}

uint16_t palette_color_from_index(uint8_t index)
{
	if (index >= 16) {
		return 0;
	}
	uint32_t rgb888 = loaded_palette[index];
	return rgb888;
}

uint32_t palette_color_from_hex(const char *hex_digit)
{
	if (strlen(hex_digit) != 1) {
		return 0;
	}

	char hex = hex_digit[0];
	int index;

	if (hex >= '0' && hex <= '9') {
		index = hex - '0';
	} else if (hex >= 'A' && hex <= 'F') {
		index = hex - 'A' + 10;
	} else if (hex >= 'a' && hex <= 'f') {
		index = hex - 'a' + 10;
	} else {
		return 0;
	}

	if (index >= 0 && index < 16) {
		return loaded_palette[index];
	} else {
		return 0;
	}
}

void palette_draw_grid(int grid_x, int grid_y, int tile_size)
{
	int rect_width = tile_size * 2;
	int rect_height = tile_size * 2;
	int margin = 0;
	char hex_digit[2];

	for (int i = 0; i < 16; i++) {
		int x = grid_x + (i % 4) * (rect_width + margin);
		int y = grid_y + (i / 4) * (rect_height + margin);

		FbColor(loaded_palette[i]);
		FbMove(x, y);
		FbFilledRectangle(rect_width, rect_height);

		sprintf(hex_digit, "%X", i);
		int text_x = x + rect_width / 2 - 3;
		int text_y = y + rect_height / 2 - 4;
		FbColor(WHITE);
		FbMove(text_x, text_y);
		FbWriteString(hex_digit);
	}
}

void palette_replace_color(uint8_t original_index, uint8_t replacement_index)
{
	if (original_index < 16 && replacement_index < 16) {
		palette_replacements[original_index] = replacement_index;
	}
}

void palette_clear_replacement(uint8_t original_index)
{
	if (original_index < 16) {
		palette_replacements[original_index] = original_index;
	}
}
