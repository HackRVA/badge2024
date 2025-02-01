#ifndef PALETTE_H
#define PALETTE_H

#include <stdint.h>

#define PALETTE_SIZE 16

struct palette {
  uint32_t colors[PALETTE_SIZE];
  uint8_t replacements[PALETTE_SIZE];
};

uint16_t palette_get_color(struct palette, uint8_t index);
uint16_t palette_color_from_index(struct palette, uint8_t index);
uint32_t palette_color_from_hex(struct palette, const char *hex_digit);
void palette_replace_color(struct palette*, uint8_t original_index, uint8_t replacement_index);
void palette_clear_replacement(struct palette*, uint8_t original_index);
void palette_draw_grid(struct palette, int grid_x, int grid_y, int tile_size);

#endif // PALETTE_H
