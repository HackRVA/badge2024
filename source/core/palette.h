#ifndef PALETTE_H
#define PALETTE_H

#include <stdint.h>

#define PALETTE_SIZE 16

void palette_init(void);
uint16_t palette_get_color(uint8_t index);
uint32_t *palette_get_loaded_palette(void);
void palette_load(const uint32_t palette[16]);
void palette_reset(void);
uint16_t palette_color_from_index(uint8_t index);
uint32_t palette_color_from_hex(const char *hex_digit);
void palette_replace_color(uint8_t original_index, uint8_t replacement_index);
void palette_clear_replacement(uint8_t original_index);

void palette_draw_grid(int grid_x, int grid_y, int tile_size);

#endif // PALETTE_H
