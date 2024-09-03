#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"

struct map *map_new(void)
{
	struct map *m;

	m = malloc(sizeof(struct map));
	m->nodes = calloc(16, sizeof(struct map_node));
	m->cap = 16;
	m->size = 0;
	return m;
}

void map_free(struct map *m)
{
	size_t i;

	for (i = 0; i < m->size; i++) {
		free(m->nodes[i].key);
		free(m->nodes[i].val);
	}
	free(m->nodes);
	free(m);
}

int map_insert(struct map *m, enum mapping_type type, char *key, char *val)
{
	if (m->size == m->cap) {
		struct map_node *oldnodes;

		oldnodes = m->nodes;
		m->cap = m->cap * 2;
		m->nodes = realloc(m->nodes, sizeof(struct map_node) * m->cap);
		if (m->nodes == NULL) {
			free(oldnodes);
			perror("realloc()");
			return -1;
		}
		bzero(m->nodes + m->size, m->size);
	}

	m->nodes[m->size].type = type;
	m->nodes[m->size].key = key;
	m->nodes[m->size].val = val;

	m->size += 1;
	return 0;
}

struct map_node *map_get(struct map *m, char *key)
{
	size_t ptr;

	ptr = 0;
	while (ptr < m->size) {
		if (!strcmp(m->nodes[ptr].key, key)) {
			return &m->nodes[ptr];
		}
		ptr++;
	}
	return NULL;
}
