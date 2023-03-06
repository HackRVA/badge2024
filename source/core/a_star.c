#include "a_star.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

struct nodeset {
	int nmembers;
	int maxmembers;
	__extension__ void *node[0];
};

static int nodeset_empty(struct nodeset *n)
{
	return (n->nmembers == 0);
}

static void nodeset_add_node(struct nodeset *n, void *node)
{
	int i;

	for (i = 0; i < n->nmembers; i++) {
		if (n->node[i] == node)
			return;
	}
	assert(n->nmembers < n->maxmembers);
	n->node[n->nmembers] = node;
	n->nmembers++;
}

static void nodeset_remove_node(struct nodeset *n, void *node)
{
	int i;

	for (i = 0; i < n->nmembers; i++) {
		if (n->node[i] != node)
			continue;
		if (i == n->nmembers - 1) {
			n->node[i] = NULL;
			n->nmembers--;
			return;
		}
		n->node[i] = n->node[n->nmembers - 1];
		n->nmembers--;
		n->node[n->nmembers] = NULL;
		return;
	}
}

static int nodeset_contains_node(struct nodeset *n, void *node)
{
	int i;

	for (i = 0; i < n->nmembers; i++)
		if (n->node[i] == node)
			return 1;
	return 0;
}

static struct nodeset *nodeset_new(int maxnodes, void *address)
{
	struct nodeset *n;

	n = address;
	memset(n, 0, sizeof(*n) + maxnodes * sizeof(void *));
	n->maxmembers = maxnodes;
	return n;
}

struct node_pair {
	void *from, *to;
};

struct node_map {
	int nelements;
	__extension__ struct node_pair p[0];
};

struct score_entry {
	void *node;
	int score;
};

struct score_map {
	int nelements;
	__extension__ struct score_entry s[0];
};

static int score_map_get_score(struct score_map *m, void *node)
{
	int i;

	for (i = 0; i < m->nelements; i++)
		if (m->s[i].node == node)
			return m->s[i].score;
	assert(0);
	return 0x7fffffff;
}

static void *lowest_score(struct nodeset *candidates, struct score_map *s)
{

	int i;
	int score, lowest_score;
	void *lowest = NULL;

	for (i = 0; i < candidates->nmembers; i++) {
		score = score_map_get_score(s, candidates->node[i]);
		if (lowest != NULL && score > lowest_score)
			continue;
		lowest = candidates->node[i];
		lowest_score = score;
	}
	return lowest;
}

static struct score_map *score_map_new(int maxnodes, void *address)
{
	struct score_map *s;

	s = address;
	memset(s, 0, sizeof(*s) + sizeof(struct score_entry) * maxnodes);
	s->nelements = maxnodes;
	return s;
}

static void score_map_add_score(struct score_map *s, void *node, int score)
{
	int i;

	for (i = 0; i < s->nelements; i++) {
		if (s->s[i].node != node)
			continue;
		s->s[i].score = score;
		return;
	}
	for (i = 0; i < s->nelements; i++) {
		if (s->s[i].node != NULL)
			continue;
		s->s[i].node = node;
		s->s[i].score = score;
		return;
	}
	assert(0);
}

static struct node_map *node_map_new(int maxnodes, void *address)
{
	struct node_map *n;

	n = address;
	memset(n, 0, sizeof(*n) + sizeof(struct node_pair) * maxnodes);
	n->nelements = maxnodes;
	return n;
}

static void node_map_set_from(struct node_map *n, void *to, void *from)
{
	int i;

	for (i = 0; i < n->nelements; i++) {
		if (n->p[i].to != to)
			continue;
		n->p[i].from = from;
		return;
	}
	/* didn't find it, pick a NULL entry */
	for (i = 0; i < n->nelements; i++) {
		if (n->p[i].to != NULL)
			continue;
		n->p[i].to = to;
		n->p[i].from = from;
		return;
	}
	assert(0); /* should never get here */
}

static void *node_map_get_from(struct node_map *n, void *to)
{
	int i;

	for (i = 0; i < n->nelements; i++)
		if (n->p[i].to == to)
			return n->p[i].from;
	return NULL;
}

static void reconstruct_path(struct node_map *came_from, void *current, void ***path, int *nodecount, int maxnodes, void *address)
{
	int i;
	void **p = address;
	memset(p, 0, sizeof(*p) * maxnodes);

	for (i = 0; i < came_from->nelements; i++)
		if (came_from->p[i].to == NULL)
			break;
	p[0] = current;
	i = 1;
	while ((current = node_map_get_from(came_from, current))) {
		p[i] = current;
		i++;
	}
	*nodecount = i;
	*path = p;
}

struct a_star_path *a_star(void *context, struct a_star_working_space *working_space,
				void *start, void *goal,
				int maxnodes,
				a_star_node_cost_fn distance,
				a_star_node_cost_fn cost_estimate,
				a_star_neighbor_iterator_fn nth_neighbor)
{
	struct nodeset *openset, *closedset;
	struct node_map *came_from;
	struct score_map *gscore, *fscore;
	void *neighbor, *current;
	int tentative_gscore;
	int i, n;
	void **answer = NULL;
	int answer_count = 0;
	struct a_star_path *return_value;

	closedset = nodeset_new(maxnodes, working_space->nodeset[0]);
	openset = nodeset_new(maxnodes, working_space->nodeset[1]);
	came_from = node_map_new(maxnodes, working_space->nodemap);
	gscore = score_map_new(maxnodes, working_space->scoremap[0]);
	fscore = score_map_new(maxnodes, working_space->scoremap[1]);

	nodeset_add_node(openset, start);
	score_map_add_score(gscore, start, 0.0);
	score_map_add_score(fscore, start, cost_estimate(context, start, goal));

	while (!nodeset_empty(openset)) {
		current = lowest_score(openset, fscore);
		if (current == goal) {
			reconstruct_path(came_from, current, &answer, &answer_count, maxnodes, working_space->a_star_path[0]);
			break;
		}
		nodeset_remove_node(openset, current);
		nodeset_add_node(closedset, current);
		n = 0;
		while ((neighbor = nth_neighbor(context, current, n))) {
			n++;
			if (nodeset_contains_node(closedset, neighbor))
				continue;
			tentative_gscore = score_map_get_score(gscore, current) + distance(context, current, neighbor);
			if (!nodeset_contains_node(openset, neighbor))
				nodeset_add_node(openset, neighbor);
			else if (tentative_gscore >= score_map_get_score(gscore, neighbor))
				continue;
			node_map_set_from(came_from, neighbor, current);
			score_map_add_score(gscore, neighbor, tentative_gscore);
			score_map_add_score(fscore, neighbor,
					score_map_get_score(gscore, neighbor) +
						cost_estimate(context, neighbor, goal));
		}
	}
	if (answer_count == 0) {
		return_value = NULL;
	} else {
		return_value = working_space->a_star_path[1];
		return_value->node_count = answer_count;
		for (i = 0; i < answer_count; i++) {
			return_value->path[answer_count - i - 1] = answer[i];
		}
	}
	return return_value;
}
