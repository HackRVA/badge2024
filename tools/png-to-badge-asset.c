#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "../source/hal/png_utils.h"

#define LCD_XSIZE 128
#define LCD_YSIZE 160

#define TRANSPARENT_INDEX 255
#define ALPHA_THRESHOLD 128

static int errors = 0;
static int warnings = 0;
static uint16_t colormap[256] = { 0 };
static int ncolors = 0;
#define MAX_OVERFLOW_COLORS (0x0ffff)
static uint16_t overflow_colormap[MAX_OVERFLOW_COLORS];
static int noverflow_colors;
static int maxcolors = 256;
static uint8_t *pixdata = NULL;
static uint16_t *pixdata16 = NULL;
static int pixdata_size = 0;
static int pixels_per_byte = 1;
static int pixel_count = 0;
static int bits_per_pixel = 8;
static int bits_per_pixel_mask = 0xff;
static int color_count = 0;

static void __attribute__((noreturn)) usage(char *program_name)
{
	fprintf(stderr, "%s: usage: %s bits-per-pixel png-file(s)\n", program_name, program_name);
	fprintf(stderr, "      bits-per-pixel may be 16, 8, 4, 2, or 1\n");
	exit(1);
}

/* Returns the index into the colormap of the pixel color, possibly adding
 * the color to the colormap.  If there is no room left in the colormap,
 * -1 is returned.  Colors are stored in the colormap in 16 bit quantites,
 * with 5 bits each for R, G, and B with R in bits 11 - 15, G in bits 6-10,
 * and B in bits 0-4.  Bit 5 is unused.
 */
static int add_to_colormap(unsigned char *pixel)
{
	/* Compute the 16-bit version of the color */
	uint16_t color;
	uint8_t r, g, b;

	r = pixel[0];
	g = pixel[1];
	b = pixel[2];

	color = ((((r >> 3) & 0b11111) << 11)
		| (((g >> 3) & 0b11111) <<  6)
		| (((b >> 3) & 0b11111)));

	/* Check if we've already stored this color in cmap */
	for (int i = 0; i < ncolors; i++) {
		if (colormap[i] == color)
			return i;
	}

	/* Not found, add the new color */
	if (ncolors < maxcolors) {
		colormap[ncolors] = color;
		ncolors++;
		color_count++;
		return (ncolors - 1);
	}

	/* No room left in colormap, add to overflow_colormap.
	 * This just so we can accurately count and report the actual
	 * number of colors in the image data.  We don't actually do
	 * anything with the overflow colors.
	 */
	for (int i = 0; i < noverflow_colors; i++) {
		if (overflow_colormap[i] == color)
			return -1;
	}

	if (noverflow_colors < MAX_OVERFLOW_COLORS) {
		overflow_colormap[noverflow_colors] = color;
		noverflow_colors++;
		color_count++; 
	}
	
	return -1;
}

/* Calculate bytes per row for decoded png data. If alpha channel is present, it's easy
 * just 4 bytes * pixels per row.  If no alpha channel, it's 3 bytes * pixels per row,
 * plus possible a few more bytes to make things evenly divisible by 4.
 */
static int calculate_bytes_per_row(int width, int hasalpha)
{
	int answer;

	if (hasalpha)
		return width * 4;
	answer = width * 3;
	while (answer & 0x03)
		answer++;
	return answer;
}

static void free_pixdata(void)
{
	if (pixdata)
		free(pixdata);
	if (pixdata16)
		free(pixdata16);
	pixdata = NULL;
	pixdata16 = NULL;
}

/* Generate the color map (list of RGB colors used in the image, packed into 16 bit quantities)
 * and the pixel data (one value per pixel, indexing into the color map.)  The pixel data can
 * be packed into bytes in quanties of 1, 2, 4, or 8 pixels per byte, depending on pixels_per_byte
 */
static int generate_colormap_and_pixdata(char *image, int width, int height, int hasalpha)
{
	unsigned char *img = (unsigned char *) image;
	unsigned char *pixel;
	unsigned char alpha;
	int bytes_per_pixel = hasalpha ? 4 : 3;
	int bytes_per_row = calculate_bytes_per_row(width, hasalpha);

	color_count = 0;
	pixdata_size = (width * height) / pixels_per_byte;
	pixdata = malloc(pixdata_size);
	if (!pixdata) {
		fprintf(stderr, "  Error: Failed to allocate %d bytes\n", pixdata_size);
		return -1;
	}
	memset(pixdata, 0, pixdata_size);
	pixel_count = 0;

	for (int y = 0; y < height; y++) {
		unsigned char *start_of_row = img + (y * bytes_per_row);
		for (int x = 0; x < width; x++) {
			int index;
			pixel = start_of_row + x * bytes_per_pixel;
			if (hasalpha && bits_per_pixel != 16 && bits_per_pixel != 1)
				alpha = pixel[3];
			else
				alpha = 255;
			if (alpha < 128) {
				index = TRANSPARENT_INDEX & bits_per_pixel_mask;
			} else {
				index = add_to_colormap(pixel);
			}
			if (index < 0)
				continue;
			int byte = pixel_count / pixels_per_byte;
			int shift = pixels_per_byte - (pixel_count % pixels_per_byte) - 1;
			pixdata[byte] |= (index << (shift * bits_per_pixel));
			pixel_count++;
		}
	}
	if (color_count > maxcolors) {
		fprintf(stderr, "  Error: Too many colors (%d) in image (max = %d)\n",
			color_count, maxcolors);
		fprintf(stderr, "  You should posterize the image to reduce colors.\n");
		return -1;
	}
	return 0;
}

/* Generate the 16-bit pixel data (one 16-bit value per pixel color data stored in pixdata16)
 */
static int generate_pixdata16(char *image, int width, int height, int hasalpha)
{
	unsigned char *img = (unsigned char *) image;
	unsigned char *pixel;
	int bytes_per_pixel = hasalpha ? 4 : 3;
	int bytes_per_row = calculate_bytes_per_row(width, hasalpha);
	pixdata_size = 2 * width * height;
	pixdata16 = malloc(pixdata_size);
	if (!pixdata16) {
		fprintf(stderr, "  Error: Failed to allocate %d bytes\n", pixdata_size);
		return -1;
	}
	memset(pixdata16, 0, pixdata_size);
	pixel_count = 0;

	for (int y = 0; y < height; y++) {
		unsigned char *start_of_row = img + (y * bytes_per_row);
		for (int x = 0; x < width; x++) {
			pixel = start_of_row + x * bytes_per_pixel;
			/* Compute the 16-bit version of the color */
			uint16_t color;
			uint8_t r, g, b;

			r = pixel[0];
			g = pixel[1];
			b = pixel[2];

			color = ((((r >> 3) & 0b11111) << 11)
				| (((g >> 3) & 0b11111) <<  6)
				| (((b >> 3) & 0b11111)));

			pixdata16[pixel_count] = color;
			pixel_count++;
		}
	}
	return 0;
}

/* Generate the 1-bit pixel data (8 pixels per byte, stored in pixdata, no color map indexing)
 */
static int generate_pixdata1(char *image, int width, int height, int hasalpha)
{
	unsigned char *img = (unsigned char *) image;
	unsigned char *pixel;
	int bytes_per_pixel = hasalpha ? 4 : 3;
	int bytes_per_row = calculate_bytes_per_row(width, hasalpha);
	int output_bytes_per_row = width / 8;
	if (8 * output_bytes_per_row < width) {
		output_bytes_per_row++;
	}
	pixdata_size = (output_bytes_per_row * height);
	pixdata = malloc(pixdata_size);
	if (!pixdata) {
		fprintf(stderr, "  Error: Failed to allocate %d bytes\n", pixdata_size);
		return -1;
	}
	memset(pixdata, 0, pixdata_size);
	pixel_count = 0;
	int byte = 0;
	int bit = 0;
	for (int y = 0; y < height; y++) {
		unsigned char *start_of_row = img + (y * bytes_per_row);
		byte = y * output_bytes_per_row;
		bit = 0;
		for (int x = 0; x < width; x++) {
			pixel = start_of_row + x * bytes_per_pixel;
			/* Compute the 1-bit version of the color */
			uint16_t color;
			uint8_t r, g, b;

			r = pixel[0];
			g = pixel[1];
			b = pixel[2];

			color = r + g + b;
			if (color > 127 * 3)
				color = 1;
			else
				color = 0;

			pixdata[byte] |= (color << bit);
			pixel_count++;
			bit++;
			if (bit > 7) {
				bit = 0;
				byte++;
			}
		}
	}
	return 0;
}


static char *after_last_slash(char *name)
{
	char *last_slash = name;
	for (int i = 0; name[i] != '\0'; i++) {
		if (name[i] == '\\' || name[i] == '/') {
			if (name[i + 1] != '\\' && name [i + 1] != '/')
				last_slash = &name[i + 1];
		}
	}
	return last_slash;
}

/* Generate C code for the color map, pixel data, and asset structures needed
 * by the badge drawing routines.
 */
static int generate_c_code(char *filename, char *prefix, int width, int height)
{
	char *tmp_image_name = strdup(filename);
	char *dot = strstr(tmp_image_name, ".png");
	if (!dot)
		dot = strstr(tmp_image_name, ".PNG");
	if (dot)
		*dot = '\0'; /* strip off the ".png" or ".PNG" from the name */

	/* if there are leading directory names, get rid of them */
	char *image_name = after_last_slash(tmp_image_name);

	/* convert dashes to underscores */
	for (char *i = image_name; *i; i++)
		if (*i == '-')
			*i = '_';


	time_t lt = time(NULL);
	struct tm* now = localtime(&lt);
	char *when = strdup(asctime(now));

	/* asctime puts a newline on the end of the time for some reason.  Cut it off. */
	char *newline = strstr(when, "\n");
	if (newline)
		*newline = '\0';

	printf("/* Begin code generated by png-to-badge-asset from %s %s. */\n", filename, when);
	if (bits_per_pixel != 16 && bits_per_pixel != 1) {
		printf("static const uint16_t %s%s_colormap[%d] = {\n", prefix, image_name, ncolors);
		printf("\t");
		for (int i = 0; i < ncolors; i++) {
			if ((i % 12) == 11)
				printf("0x%04hx,\n\t", colormap[i]);
			else
				printf("0x%04hx,%s", colormap[i], i == ncolors - 1 ? "" : " ");
		}
		printf("\n}; /* %d values */\n", ncolors);
		printf("\n");
		printf("static const uint8_t %s%s_data[%d] = {\n", prefix, image_name, pixel_count);
		printf("\t");
		int datacount = pixel_count;
		switch (bits_per_pixel) {
		case 8: datacount = pixel_count;
			break;
		case 4: datacount = pixel_count / 2;
			break;
		case 2: datacount = pixel_count / 4;
			break;
		default: /* should not be reachable */
			fprintf(stderr, "BUG at %s:%d -- hit unreachable code\n",
				__FILE__, __LINE__);
			exit(1);
			break;
		}
		for (int i = 0; i < datacount; i++) {
			if ((i % 20) == 19)
				printf("%hhu,\n\t", pixdata[i]);
			else
				printf("%hhu,%s", pixdata[i], i == pixel_count - 1 ? "" : " ");
		}
		printf("\n}; /* %d values */\n", datacount);
	} else if (bits_per_pixel == 16) {
		printf("static const uint16_t %s%s_data[%d] = {\n", prefix, image_name, pixel_count);
		printf("\t");
		for (int i = 0; i < pixel_count; i++) {
			if ((i % 12) == 11)
				printf("0x%04hx,\n\t", pixdata16[i]);
			else
				printf("0x%04hx,%s", pixdata16[i], i == pixel_count - 1 ? "" : " ");
		}
		printf("\n}; /* %d values */\n", pixel_count);
	} else if (bits_per_pixel == 1) {
		int bytes_per_row = width / 8;
		if (bytes_per_row * 8 < width)
			bytes_per_row++;
		int datacount = bytes_per_row * height;
		if ((datacount * 8) < pixel_count)
			datacount++;
		printf("static const uint8_t %s%s_data[%d] = {\n", prefix, image_name, datacount);
		printf("\t");
		for (int i = 0; i < datacount; i++) {
			if ((i % 16) == 15)
				printf("0x%02hhx,\n\t", pixdata[i]);
			else
				printf("0x%02hhx,%s", pixdata[i], i == datacount - 1 ? "" : " ");
		}
		printf("\n}; /* %d values */\n", datacount);
	}
	printf("\n");
	printf("static const struct asset2 %s%s = {\n", prefix, image_name);
	printf("\t.type = PICTURE%dBIT,\n", bits_per_pixel);
	printf("\t.seqNum = 1,\n");
	printf("\t.x = %d,\n", width);
	printf("\t.y = %d,\n", height);
	if (bits_per_pixel != 16 && bits_per_pixel != 1) {
		printf("\t.colormap = (const uint16_t *) %s%s_colormap,\n", prefix, image_name);
		printf("\t.pixel = (const unsigned char *) %s%s_data,\n", prefix, image_name);
	} else if (bits_per_pixel == 16) {
		printf("\t.colormap = NULL, /* not used by PICTURE16BIT or PICTURE1BIT */\n");
		printf("\t.pixel16 = (const uint16_t *) %s%s_data,\n", prefix, image_name);
	} else if (bits_per_pixel == 1) {
		printf("\t.colormap = NULL, /* not used by PICTURE16BIT or PICTURE1BIT */\n");
		printf("\t.pixel = (const uint8_t *) %s%s_data,\n", prefix, image_name);
	}
	printf("};\n");
	printf("/* End of code generated by png-to-badge-asset from %s %s */\n", filename, when);

	free(when);
	free(tmp_image_name);

	return 0;
}

/* Process a png image turning it into a badge asset implemented in C code */
static int process_image(char *filename, char *prefix)
{
	char *image;
	int width, height, hasalpha;
	char whynot[1000];

	fprintf(stderr, "Processing %s\n", filename);

	width = 0;
	height = 0;
	hasalpha = 0;
	ncolors = 0;
	image = png_utils_read_png_image(filename, 0, 0, 0,
				&width, &height, &hasalpha, whynot, sizeof(whynot));

	if (!image) {
		fprintf(stderr, "  Error processing %s: %s\n",
			filename, whynot);
		return 1;
	}
	if (width > LCD_XSIZE) {
		fprintf(stderr, "  Warning: %s x dimension %d > %d\n", filename, width, LCD_XSIZE);
		warnings++;
	}
	if (height > LCD_YSIZE) {
		fprintf(stderr, "  Warning: %s y dimension %d > %d\n", filename, height, LCD_YSIZE);
		warnings++;
	}
	if (hasalpha && (bits_per_pixel == 16 || bits_per_pixel == 1)) {
		fprintf(stderr, "  Warning: %s has an alpha channel which will be ignored\n", filename);
		warnings++;
	}

	if (bits_per_pixel == 16) {
		int rc = generate_pixdata16(image, width, height, hasalpha);
		if (rc < 0)
			goto error_out;
	} else if (bits_per_pixel == 1) {
		int rc = generate_pixdata1(image, width, height, hasalpha);
		if (rc < 0)
			goto error_out;
	} else {
		int rc = generate_colormap_and_pixdata(image, width, height, hasalpha);
		if (rc < 0)
			goto error_out;
	}
	generate_c_code(filename, prefix, width, height);

	free_pixdata();	
	free(image);
	return 0;

error_out:
	free_pixdata();	
	free(image);
	return 1;
}

int main(int argc, char *argv[])
{

	if (argc < 3) {
		usage(argv[0]);
		__builtin_unreachable();
	}

	int rc = sscanf(argv[1], "%d", &bits_per_pixel);
	if (rc != 1) {
		usage(argv[0]);
		__builtin_unreachable();
	}

	switch (bits_per_pixel) {
	case 1: maxcolors = 2; /* transparency can't be supported */
		pixels_per_byte = 8;
		bits_per_pixel_mask = 0x01;
		break;
	case 2: maxcolors = 3; /* leave one for transparency */
		pixels_per_byte = 4;
		bits_per_pixel_mask = 0x03;
		break;
	case 4: maxcolors = 15; /* leave one for transparency */
		pixels_per_byte = 2;
		bits_per_pixel_mask = 0x0f;
		break;
	case 8: maxcolors = 255; /* save one for transparent */
		pixels_per_byte = 1;
		bits_per_pixel_mask = 0xff;
		break;
	case 16:maxcolors = 0x0ffff;
		pixels_per_byte = 1; /* unused */
		bits_per_pixel_mask = 0x0ffff;
		break;
	default:
		usage(argv[0]);
		__builtin_unreachable();
		break;
	}

	errors = 0;
	warnings = 0;
	for (int i = 2; i < argc; i++)
		errors += process_image(argv[i], "");

	fprintf(stderr, "Processed %d file(s) with %d error(s) and %d warning(s).\n",
			argc - 2, errors, warnings);
	free_pixdata();
	return 0;
}
