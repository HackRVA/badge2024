#include <stdio.h>
#include <stdint.h>

#include "../source/display/assets/font8x8.xbm.h"

static unsigned char rotfont[sizeof(font8x8_bits)] = { 0 };

unsigned char get_unrotated_bit(int c, int row, int col)
{
	unsigned char *b = (unsigned char *) &font8x8_bits[c * 8 + row];
	return !!(*b & (1u << col));
}

void set_rotated_bit(int c, int row, int col, unsigned char v)
{
	int rotated_r, rotated_c;

	rotated_r = col;
	rotated_c = 7 - row;
	unsigned char *b = &rotfont[c * 8 + rotated_r];
	*b |= (v << rotated_c);
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
	/* Rotate the font */
	for (int c = 0; c < (int) sizeof(rotfont) / 8; c++) {
		for (int row = 0; row < 8; row++) {
			for (int col = 0; col < 8; col++) {
				unsigned char i = get_unrotated_bit(c, row, col);
				set_rotated_bit(c, row, col, i);
			}
		}
	}

	/* print the rotated font */
	printf("/* Rotated font 8x8 1-bit font */\n");
	printf("/* This file created by ../tools/rotate-font.c. Do not edit. */\n");
	printf("const unsigned char font8x8_rotated_bits[] = {\n");
	for (int i = 0; i < (int) sizeof(rotfont); i++) {
		if ((i % 12) == 0) {
			printf("	");
		}
		printf("0x%02x,%s", rotfont[i], (i % 12) == 11 ? "\n" : " ");
	}
	printf("\n};\n");
	return 0;
}
