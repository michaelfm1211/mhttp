#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include "map.h"
#include "client.h"

#define HTTP_OK "HTTP/1.1 200 OK\r\n"
#define HTTP_FOUND "HTTP/1.1 302 Found\r\n"
#define HTTP_NOT_FOUND "HTTP/1.1 404 Not Found\r\n"
#define HTTP_NOT_IMPLEMENTED "HTTP/1.1 501 Not Implemented\r\n"
#define HTTP_SERVER_ERROR "HTTP/1.1 500 Internal Server Error\r\n"

#define MIME_TEXT_PLAIN "Content-Type: text/plain\r\n"

void plog(FILE *file, const char *addr, const char *fmt, ...)
{
	va_list args;
	time_t now;
	struct tm nowtm;
	char nowstr[64];

	va_start(args, fmt);
	now = time(NULL);
	gmtime_r(&now, &nowtm);
	strftime(nowstr, sizeof(nowstr), "%Y-%m-%d %H:%M:%S", &nowtm);
	fprintf(file, "[%s] %s: ", nowstr, addr);
	vfprintf(file, fmt, args);
	fputc('\n', file);
	va_end(args);
}

void plogerr(const char *addr, const char *msg)
{
	char err[64];
	strerror_r(errno, err, sizeof(err));
	plog(stderr, addr, "%s: %s", msg, err);
}

void serve_error(int sock)
{
	write(sock, HTTP_SERVER_ERROR, sizeof(HTTP_SERVER_ERROR) - 1);
	write(sock, "\r\n", 2);
}

void serve_file(int sock, const char *addr, const char *param)
{
	char *filename, *mime_type;
	FILE *file;
	char buf[1024];
	size_t nread;

	filename = strdup(param);
	mime_type = strchr(filename, ' ');
	if (mime_type == NULL) {
		plog(stdout, addr, "Invalid FILE syntax in map file.");
		serve_error(sock);
		return;
	}
	*mime_type = '\0';
	mime_type += 1;

	file = fopen(filename, "r");
	if (file == NULL) {
		plogerr(addr, filename);
		serve_error(sock);
		return;
	}

	write(sock, HTTP_OK, sizeof(HTTP_OK)-1);
	snprintf(buf, 1024, "Content-Type: %s\r\n\r\n", mime_type);
	write(sock, buf, strlen(buf));

	while ((nread = fread(buf, 1, sizeof(buf), file)) > 0) {
		write(sock, buf, nread);
	}
	if (ferror(file)) {
		plogerr(addr, "fread()");
		return;
	}

	free(filename);
	fclose(file);
}

void serve_link(int sock, const char *link)
{
	char *buf;
	size_t buflen;

	buflen = strlen(link) + sizeof("Location: \r\n\r\n");
	buf = malloc(buflen);
	snprintf(buf, buflen, "Location: %s\r\n\r\n", link);

	write(sock, HTTP_FOUND, sizeof(HTTP_FOUND)-1);
	write(sock, buf, buflen);
	free(buf);
}

void serve_text(int sock, const char *text)
{
	write(sock, HTTP_OK, sizeof(HTTP_OK)-1);
	write(sock, MIME_TEXT_PLAIN, sizeof(MIME_TEXT_PLAIN)-1);
	write(sock, "\r\n", 2);
	write(sock, text, strlen(text));
}

void serve_not_found(int sock)
{
	write(sock, HTTP_NOT_FOUND, sizeof(HTTP_NOT_FOUND) - 1);
	write(sock, "\r\n", 2);
}

void serve_not_implemented(int sock)
{
	write(sock, HTTP_NOT_IMPLEMENTED,
			sizeof(HTTP_NOT_IMPLEMENTED) - 1);
	write(sock, "\r\n", 2);
}

char *read_query(int sock, const char *addr)
{
	char *buf, *query, *tmp;
	size_t buflen;
	ssize_t nread;

	buflen = 64;
	buf = malloc(64);
	if (buf == NULL) {
		plogerr(addr, "malloc()");
		return NULL;
	}
	nread = read(sock, buf, 64);
	if (nread < 0) {
		plogerr(addr, "read()");
		goto err;
	}
	while (memchr(buf, '\r', buflen) == NULL) {
		char *oldbuf;
		oldbuf = buf;
		buflen += 64;
		buf = realloc(buf, buflen);
		if (buf == NULL) {
			plogerr(addr, "realloc()");
			free(oldbuf);
			return NULL;
		}
		nread = read(sock, buf, 64);
		if (nread < 0) {
			plogerr(addr, "read()");
			goto err;
		}
	}

	query = memchr(buf, ' ', buflen);
	if (query == NULL) {
		plog(stderr, addr, "Malformed query.");
		goto err;
	}
	*query = '\0';
	if (strcmp(buf, "GET")) {
		plog(stderr, addr, "Non-GET method.");
		serve_not_implemented(sock);
		goto err;
	}

	query += 1;
	tmp = memchr(query, ' ', buflen - (query - buf));
	if (tmp == NULL) {
		plog(stderr, addr, "Malformed query.");
		goto err;
	}
	*tmp = '\0';

	query = strdup(query);
	free(buf);
	return query;
err:
	free(buf);
	return NULL;
}

void handle_client(int sock, const char *addr, struct map *map)
{
	char *query;
	struct map_node *mapping;

	plog(stdout, addr, "Connection opened.");

	query = read_query(sock, addr);
	if (query == NULL) {
		goto end;
	}
	plog(stdout, addr, "%s", query);

	mapping = map_get(map, query);
	free(query);
	if (mapping == NULL) {
		serve_not_found(sock);
		goto end;
	} 

	switch (mapping->type) {
	case MAPPING_FILE:
		serve_file(sock, addr, mapping->val);
		break;
	case MAPPING_LINK:
		serve_link(sock, mapping->val);
		break;
	case MAPPING_TXT:
		serve_text(sock, mapping->val);
		break;
	default:
		serve_error(sock);
		break;
	}

end:
	plog(stdout, addr, "Connection closed.");
	shutdown(sock, SHUT_RDWR);
	close(sock);
}
