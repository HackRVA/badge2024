#ifndef A_STAR_H__
#define A_STAR_H__

/* Author: Stephen M. Cameron
 *
 * This is an implementation of the "A*" (A star) path-finding algorithm.
 * See: https://en.wikipedia.org/wiki/A*_search_algorithm
 */

struct a_star_working_space {
	void *nodeset[2];
	void *nodemap;
	void *scoremap[2];
	void *a_star_path[2];
};

#define A_STAR_NODESET_SIZE(maxnodes) (2 * sizeof(int) + (maxnodes) * sizeof(void *))
#define A_STAR_NODEMAP_SIZE(maxnodes) (8 + 16 * (maxnodes))
#define A_STAR_SCOREMAP_SIZE(maxnodes) (16 * ((maxnodes) + 1))
#define A_STAR_PATH_SIZE(maxnodes) (sizeof(int) + (maxnodes) * sizeof(void *))

struct a_star_path {
	int node_count;
	__extension__ void *path[0];
};

typedef int (*a_star_node_cost_fn)(void *context, void *first, void *second);
typedef void *(*a_star_neighbor_iterator_fn)(void *context, void *node, int neighbor);

/**
 *
 * a_star - runs the 'A*' algorithm to find a path from the start to the goal.
 *
 * @context: an opaque pointer which the algorithm does not use, but passes
 *           along intact to the callback functions described below.
 * @working_space: The algorithm needs some memory, but does not allocate, so
 *		   you need to provide it with memory. It needs 7 chunks of memory
 *		   in the 4 void pointers within struct a_star_working_space. The
 *		   size of each of these chunks is returned by the macros:
 *		   A_STAR_NODESET_SIZE(maxnodes)
 *		   A_STAR_NODEMAP_SIZE(maxnodes)
 *		   A_STAR_SCOREMAP_SIZE(maxnodes)
 *		   A_STAR_PATH_SIZE(maxnodes)
 *                 where maxnodes is the maximum possible path length.
 *
 * @start: an opaque pointer to the start node
 * @goal:  an opaque pointer to the goal node
 *
 * @maxnodes: the maximum number of nodes the algorithm will encounter (ie.
 *	      the number of possible locations in a maze you are traversing.)
 *
 * @distance: a function which you provide which returns the distance (however you
 *            choose to define that) between two arbitrary nodes
 *
 * @cost_estimate: a function you provide which provides a heuristic cost estimate
 *	           for traversing between two nodes
 *
 * @nth_neighbor: a function you provide which returns the nth neighbor of a given
 *                node, or NULL if n is greater than the number of neighbors - 1.
 *
 * Returns:
 *   The return value is a pointer to struct a_star_path (it will points within the provided
 *   struct a_star_working_space.)  the node_count is the number of nodes in the path, and
 *   the path array is a pointer to an array of node_count nodes in order from start to goal.
 *
 * See ./test_a_star.c example of how to use this.
 *
 */

struct a_star_path *a_star(void *context,
		struct a_star_working_space *working_space,
		void *start,
		void *goal,
		int maxnodes,
		a_star_node_cost_fn distance,
		a_star_node_cost_fn cost_estimate,
		a_star_neighbor_iterator_fn nth_neighbor);
#endif
