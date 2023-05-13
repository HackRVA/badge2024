all:	x11-colors-to-badge-colors rotate-font

x11-colors-to-badge-colors:	x11-colors-to-badge-colors.c
	gcc -Wall -Wextra -o x11-colors-to-badge-colors x11-colors-to-badge-colors.c

rotate-font:	rotate-font.c ../source/display/assets/font8x8.xbm.h
	gcc -Wall -Wextra -o rotate-font rotate-font.c

clean:
	rm -f x11-colors-to-badge-colors rotate-font.c

