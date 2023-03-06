#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "a_star.h"

static char maze[] =
	"##########@################x#################\n"
	"#                  #                        #\n"
	"#                  #                        #\n"
	"#  ###########     ###################      #\n"
	"#  #        #      #                 #      #\n"
	"####        #      #                 #      #\n"
	"#     #            #   ########      #      #\n"
	"#     #    #########   #      #      #      #\n"
	"#     #            #          #             #\n"
	"#     #            #          #             #\n"
	"#     #            #####   ##################\n"
	"#     ##########   #                        #\n"
	"#     #        #   #                        #\n"
	"#              #########################    #\n"
	"#          #           #           #        #\n"
	"############           #     #     #        #\n"
	"#                  #         #              #\n"
	"#############################################\n";

static int maze_width(char *maze)
{
	char *n;

	n = strchr(maze, '\n');
	return 1 + n - maze;
}

static int xoff[] = { 0, 1, 0, -1 };
static int yoff[] = { 1, 0, -1, 0 };

static char *maze_node(char *maze, int x, int y)
{
	return maze + y * maze_width(maze) + x;
}

static void *nth_neighbor(void *context, void *p, int n)
{
	char *maze = context;
	char *c = p;
	int i, x, y, offset = c - maze;

	x = offset % maze_width(maze);
	y = offset / maze_width(maze);

	for (i = n; i < 4; i++) {
		int tx = x + xoff[i];
		int ty = y + yoff[i];
		if (tx < 0 || ty < 0)
			continue;
		if (ty * maze_width(maze) + tx > (int) strlen(maze))
			continue;
		c = maze_node(maze, x + xoff[i], y + yoff[i]);
		if (*c != '#')
			return c;
	}
	return NULL;
}

static int maze_cost(void *context, void *first, void *second)
{
	char *maze = context;
	char *f = first;
	char *s = second;
	int sx, sy, fx, fy;
	int d, dx, dy;

	int fp = f - maze;
	int sp = s - maze;

	sx = sp % maze_width(maze);
	sy = sp / maze_width(maze);
	fx = fp % maze_width(maze);
	fy = fp / maze_width(maze);

	dx = sx - fx;
	dy = sy - fy;
	d = (abs(dx) + abs(dy));
	return d;
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
	static int maxnodes, i;
	char *start, *goal;
	struct a_star_path *path;
	struct a_star_working_space ws;

	start = strchr(maze, '@');
	goal = strchr(maze, 'x');
	maxnodes = strlen(maze);

        unsigned char nodeset1[A_STAR_NODESET_SIZE(maxnodes)];
        unsigned char nodeset2[A_STAR_NODESET_SIZE(maxnodes)];
	unsigned char scoremap1[A_STAR_SCOREMAP_SIZE(maxnodes)];
	unsigned char scoremap2[A_STAR_SCOREMAP_SIZE(maxnodes)];
	unsigned char nodemap[A_STAR_NODEMAP_SIZE(maxnodes)];
	unsigned char a_star_path1[A_STAR_PATH_SIZE(maxnodes)];
	unsigned char a_star_path2[A_STAR_PATH_SIZE(maxnodes)];

	ws.nodeset[0] = (void *) nodeset1;
	ws.nodeset[1] = (void *) nodeset2;
	ws.nodemap = (void *) nodemap;
	ws.scoremap[0] = (void *) scoremap1;
	ws.scoremap[1] = (void *) scoremap2;
	ws.a_star_path[0] = (void *) a_star_path1;
	ws.a_star_path[1] = (void *) a_star_path2;

	path = a_star((void *) maze, &ws, start, goal, maxnodes, maze_cost, maze_cost, nth_neighbor);
	if (!path) {
		printf("a_star() failed to return a path.\n");
		return 0;
	}
	for (i = 0; i < path->node_count; i++) {
		char *p = path->path[i];
		*p = '.';
	}
	*goal = 'x';
	*start = '@';

	printf("%s\n", maze);

	return 0;
}
