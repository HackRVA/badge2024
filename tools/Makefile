all:	x11-colors-to-badge-colors rotate-font feed-asset-converter

x11-colors-to-badge-colors:	x11-colors-to-badge-colors.c
	gcc -Wall -Wextra -o x11-colors-to-badge-colors x11-colors-to-badge-colors.c

rotate-font:	rotate-font.c ../source/display/assets/font8x8.xbm.h
	gcc -Wall -Wextra -o rotate-font rotate-font.c

feed-asset-converter:	feed-asset-converter.c
	gcc -Wall -Wextra -g3 -fsanitize=address,undefined \
		-o feed-asset-converter feed-asset-converter.c

clean:
	rm -f x11-colors-to-badge-colors rotate-font.c feed-asset-converter

