// SPDX-License-Identifier: MIT
#define _POSIX_C_SOURCE 200809L

#include "http_server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define REQ_BUF_SIZE  2048
#define BODY_BUF_SIZE 512

/* Tries to send the whole buffer in one go */
static int write_all(int fd, const char *buf, size_t len)
{
	size_t sent = 0;
	while (sent < len) {
		ssize_t n = send(fd, buf + sent, len - sent, 0);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		sent += (size_t)n;
	}
	return 0;
}

/* Serialize and send a full HTTP/1.1 response with a JSON body. */
static void send_response(int fd, const char *status, const char *body)
{
	char header[256];
	int n = snprintf(header, sizeof(header),
	                 "HTTP/1.1 %s\r\n"
	                 "Content-Type: application/json\r\n"
	                 "Content-Length: %zu\r\n"
	                 "Connection: close\r\n"
	                 "\r\n",
	                 status, strlen(body));
	if (n < 0 || (size_t)n >= sizeof(header))
		return;

	if (write_all(fd, header, (size_t)n) == 0)
		write_all(fd, body, strlen(body));
}

/* Format a time_t as ISO 8601 UTC */
static void format_timestamp(time_t t, char *out, size_t len)
{
	struct tm tm;
	gmtime_r(&t, &tm);
	strftime(out, len, "%Y-%m-%dT%H:%M:%SZ", &tm);
}

static void handle_current(int fd, struct shared_state *st)
{
	struct shared_state snap;

	pthread_mutex_lock(&st->lock);
	snap = *st;
	pthread_mutex_unlock(&st->lock);

	if (!snap.has_data) {
		send_response(fd, "503 Service Unavailable",
		              "{\"error\":\"no reading available yet\"}");
		return;
	}

	char ts[32];
	format_timestamp(snap.current.timestamp, ts, sizeof(ts));

	char body[BODY_BUF_SIZE];
	snprintf(body, sizeof(body),
	         "{\"temperature_c\":%.2f,\"pressure_hpa\":%.2f,\"timestamp\":\"%s\"}",
	         snap.current.temp_c, snap.current.pressure_kpa * 10.0, ts);

	send_response(fd, "200 OK", body);
}

static void handle_average(int fd, struct shared_state *st)
{
	struct shared_state snap;

	pthread_mutex_lock(&st->lock);
	snap = *st;
	pthread_mutex_unlock(&st->lock);

	if (!snap.has_data) {
		send_response(fd, "503 Service Unavailable",
		              "{\"error\":\"no reading available yet\"}");
		return;
	}

	const struct rolling_stats *s = &snap.stats;

	char body[BODY_BUF_SIZE];
	snprintf(body, sizeof(body),
	         "{\"temperature_c\":{\"mean\":%.2f,\"min\":%.2f,\"max\":%.2f},"
	         "\"pressure_hpa\":{\"mean\":%.2f,\"min\":%.2f,\"max\":%.2f},"
	         "\"samples\":%zu}",
	         s->temp.mean, s->temp.min, s->temp.max,
	         s->pressure.mean * 10.0, s->pressure.min * 10.0, s->pressure.max * 10.0,
	         s->sample_count);

	send_response(fd, "200 OK", body);
}

static void handle_alerts(int fd, struct shared_state *st)
{
	(void)st;
	/* Threshold logic lands in the next step; expose a stable shape now. */
	send_response(fd, "200 OK", "{\"alerts\":[]}");
}

static void handle_client(int fd, struct shared_state *st)
{
	char buf[REQ_BUF_SIZE];
	ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
	if (n <= 0)
		return;
	buf[n] = '\0';

	char method[8];
	char path[256];
	if (sscanf(buf, "%7s %255s", method, path) != 2) {
		send_response(fd, "400 Bad Request", "{\"error\":\"malformed request\"}");
		return;
	}

	if (strcmp(method, "GET") != 0) {
		send_response(fd, "405 Method Not Allowed", "{\"error\":\"only GET is supported\"}");
		return;
	}

	if (strcmp(path, "/api/v1/current") == 0) {
		handle_current(fd, st);
	} else if (strcmp(path, "/api/v1/average") == 0) {
		handle_average(fd, st);
	} else if (strcmp(path, "/api/v1/alerts") == 0) {
		handle_alerts(fd, st);
	} else {
		send_response(fd, "404 Not Found", "{\"error\":\"not found\"}");
	}
}

int http_server_run(int port, struct shared_state *st, volatile sig_atomic_t *running)
{
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		perror("socket");
		return -1;
	}

	int one = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons((uint16_t)port),
	};

	if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(listen_fd);
		return -1;
	}

	if (listen(listen_fd, 16) < 0) {
		perror("listen");
		close(listen_fd);
		return -1;
	}

	printf("HTTP server listening on port %d\n", port);

	while (*running) {
		int client_fd = accept(listen_fd, NULL, NULL);
		if (client_fd < 0) {
			if (errno == EINTR)
				break;
			continue;
		}
		handle_client(client_fd, st);
		close(client_fd);
	}

	close(listen_fd);
	return 0;
}
