#ifndef CLIENT_H
#define CLIENT_H

#include "map.h"

char *read_query(int sock, const char *addr);
void handle_client(int sock, const char *addr, struct map *map);

#endif
