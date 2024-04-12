all:	x11-colors-to-badge-colors rotate-font feed-asset-converter png-to-badge-asset

x11-colors-to-badge-colors:	x11-colors-to-badge-colors.c
	gcc -Wall -Wextra -o x11-colors-to-badge-colors x11-colors-to-badge-colors.c

rotate-font:	rotate-font.c ../source/display/assets/font8x8.xbm.h
	gcc -Wall -Wextra -o rotate-font rotate-font.c

feed-asset-converter:	feed-asset-converter.c
	gcc -Wall -Wextra -g3 -fsanitize=address,undefined \
		-o feed-asset-converter feed-asset-converter.c

png_utils.o:	../source/hal/png_utils.h ../source/hal/png_utils.c
	gcc -c -Wall -Wextra -g3 -fsanitize=address,undefined \
		../source/hal/png_utils.c -o png_utils.o

png-to-badge-asset:	png-to-badge-asset.c png_utils.o
	gcc -Wall -Wextra -g3 -fsanitize=address,undefined \
		-o png-to-badge-asset png-to-badge-asset.c png_utils.o -lpng

clean:
	rm -f x11-colors-to-badge-colors rotate-font.c feed-asset-converter png-to-badge-asset png_utils.o

