#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "map.h"
#include "client.h"

enum mapping_type mapping_type_from_str(const char *str)
{
	if (!strcmp(str, "FILE")) {
		return MAPPING_FILE;
	} else if (!strcmp(str, "LINK")) {
		return MAPPING_LINK;
	} else if (!strcmp(str, "TXT")) {
		return MAPPING_TXT;
	} else {
		return MAPPING_INVALID;
	} 
}

struct map *parse_mapfile(const char *mapfile_path)
{
	FILE *mapfile;
	size_t buflen;
	char *buf;
	struct map *map;
	int lineno;
	char *line, *brkl, *brkw;

	mapfile = fopen(mapfile_path, "r");
	if (mapfile == NULL) {
		perror(mapfile_path);
		return NULL;
	}
	fseek(mapfile, 0, SEEK_END);;
	buflen = ftell(mapfile) + 1;
	rewind(mapfile);

	buf = malloc(buflen);
	if (buf == NULL) {
		perror("malloc()");
		fclose(mapfile);
		return NULL;
	}
	if (fread(buf, 1, buflen - 1, mapfile) != buflen - 1) {
		perror("fread()");
		free(buf);
		fclose(mapfile);
		return NULL;
	}
	fclose(mapfile);
	buf[buflen - 1] = '\0';

	map = map_new();
	if (map == NULL) {
		return NULL;
	}
	lineno = 0;
	for (line = strtok_r(buf, "\n", &brkl); line;
			line = strtok_r(NULL, "\n", &brkl)) {
		enum mapping_type type;
		char *typestr, *key, *val;

		typestr = strtok_r(line, " ", &brkw);
		if (typestr == NULL) {
			goto syntax_err;
		}
		type = mapping_type_from_str(typestr);
		if (type == MAPPING_INVALID) {
			fprintf(stderr, "%s:%d: Mapping type '%s' is"
					" unsupported.\n",
					mapfile_path, lineno, typestr);
			free(buf);
			map_free(map);
			return NULL;
		}

		key = strtok_r(NULL, " ", &brkw);
		if (key == NULL) {
			goto syntax_err;
		}
		key = strdup(key);
		if (key == NULL) {
			perror("strdup()");
			goto err;
		}

		if (brkw == NULL) {
			goto syntax_err;
		}
		val = strdup(brkw);
		if (val == NULL) {
			perror("strdup()");
			goto err;
		}

		if (map_insert(map, type, key, val) < 0) {
			goto err;
		}
		lineno++;
	}

	free(buf);
	return map;
err:
	free(buf);
	map_free(map);
	return NULL;
syntax_err:
	fprintf(stderr, "%s:%d: Invalid syntax.\n", mapfile_path,
			lineno);
	free(buf);
	map_free(map);
	return NULL;
}

int create_server(const char *addrstr, int port)
{
	int server_sock;
	int opt;
	struct sockaddr_in addr;

	server_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (server_sock < 0) {
		perror("socket()");
		return -1;
	}

	opt = 1;
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt,
				sizeof(opt))) {
		perror("setsockopt()");
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(addrstr);
	addr.sin_port = htons(port);
	if (bind(server_sock, (struct sockaddr *)&addr,
				sizeof(struct sockaddr_in)) < 0) {
		perror("bind()");
		return -1;
	}

	if (listen(server_sock, 16) < 0) {
		perror("listen()");
		return -1;
 	}
	printf("Listening on %s:%d\n", addrstr, port);
	return server_sock;
}

int main(int argc, char **argv)
{
	struct map *map;
	int server_sock, client_sock;
	int port;
	struct sockaddr_in addr;
	socklen_t addrlen;

	if (argc != 4) {
		fprintf(stderr, "usage: %s mapfile addr port\n", argv[0]);
		return 1;
	}

	map = parse_mapfile(argv[1]);
	if (map == NULL) {
		return 1;
	}

	port = atoi(argv[3]);
	server_sock = create_server(argv[2], port);
	if (server_sock < 0) {
		map_free(map);
		return 1;
	}

	while (1) {
		client_sock = accept(server_sock,
				(struct sockaddr *)&addr, &addrlen);
		if (client_sock < 0) {
			perror("accept");
			return 1;
		}
		handle_client(client_sock, inet_ntoa(addr.sin_addr), map);
	}

	map_free(map);
	return 0;
}
