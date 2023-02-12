# Frame Buffer

The Frame Buffer is how you will draw something to the screen.

## Framebuffer API

```C
void FbMove(unsigned char x, unsigned char y)

void FbClear()

void FbSwapBuffers()

void FbColor(unsigned short color)

void FbWriteLine(const char *string)

void FbLine(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1)

void FbPoint(unsigned char x, unsigned char y)

void FbRectangle(unsigned char width, unsigned char height)
void FbFilledRectangle(unsigned char width, unsigned char height);

void FbDrawVectors(short points[][2],
                   unsigned char n_points,
                   short center_x,
                   short center_y,
                   unsigned char connect_last_to_first)

void FbPolygonFromPoints(short points[][2],
                         unsigned char n_points,
                         short center_x,
                         short center_y)

/* FbDrawObject() draws an object at x, y.  The coordinates of drawing[] should be centered at
 * (0, 0).  The coordinates in drawing[] are multiplied by scale, then divided by 1024 (via a shift)
 * so for 1:1 size, use scale of 1024.  Smaller values will scale the object down. This is different
 * than FbPolygonFromPoints() or FbDrawVectors() in that drawing[] contains signed chars, and the
 * polygons can be constructed via this program: https://github.com/smcameron/vectordraw
 * as well as allowing scaling.
 */
void FbDrawObject(const struct point drawing[], int npoints, int color, int x, int y, int scale)

```
see [framebuffer.h](https://github.com/HackRVA/badge2023/blob/main/source/display/framebuffer.h) for more

## Examples

#### print text to screen

```C
FbWriteLine("You WON!");
```

```C
static void draw_controls()
{
    FbColor(WHITE);
    FbMove(43,120);
    FbWriteLine("|down|");
    FbColor(GREEN);

    FbMove(5, 120);
    FbWriteLine("<Back");


    FbMove(90,120);
    FbWriteLine("desc>");
}
```
