#ifndef HEAP_H
#define HEAP_H

struct map {
	struct map_node *nodes;
	size_t cap, size;
};

enum mapping_type {
	MAPPING_FILE, /* serve a file */
	MAPPING_LINK, /* redirect to a link */
	MAPPING_TXT, /* serve the node value as text/plain */
	MAPPING_INVALID
};

struct map_node {
	enum mapping_type type;
	char *key, *val;
};

struct map *map_new(void);
void map_free(struct map *m);

int map_insert(struct map *m, enum mapping_type, char *key, char *val);
struct map_node *map_get(struct map *m, char *key);

#endif
