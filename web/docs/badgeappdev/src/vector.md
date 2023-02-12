# Vector Assets

Vector assets can be created using a tool created by @smcameron [https://github.com/smcameron/vectordraw](https://github.com/smcameron/vectordraw).


## Examples

```C
static const struct point blocking_square_points[] =
{
	{-18, 18},
	{18, -18},
	{-128, -128},
	{-18, -18},
	{18, 18},
};

FbDrawObject(blocking_square_points, sizeof(blocking_square_points) / sizeof blocking_square_points[0]), RED, 10, 10, 480);
```
