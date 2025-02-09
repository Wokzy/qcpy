
// #define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>

#include "misc.h"
#include "server.h"

#define PORT 55763

socklen_t addrlen = sizeof(struct sockaddr_in);


CTX *ctx_create()
{
	CTX *ctx = (CTX*)nc_calloc(1, sizeof(CTX));
	ctx->server_sock = -1;
	ctx->size = 0;
	ctx->capacity = 16;

	ctx->sockets = (SOCK_CTX*)nc_calloc(ctx->capacity, sizeof(SOCK_CTX));
	ctx->poll_fds = (struct pollfd*)nc_calloc(ctx->capacity, sizeof(struct pollfd));

	return ctx;
}

void ctx_add(CTX *ctx, int sock)
{
	if (ctx->size == ctx->capacity) {
		ctx->capacity <<= 1;
		ctx->sockets = nc_realloc(ctx->sockets, sizeof(SOCK_CTX) * ctx->capacity);
		ctx->poll_fds = nc_realloc(ctx->poll_fds,
								   sizeof(struct pollfd) * ctx->capacity);
	}

	ctx->sockets[ctx->size].fd = sock;
	ctx->sockets[ctx->size].inactivity_counter = 0;
	ctx->poll_fds[ctx->size].fd = sock;
	ctx->poll_fds[ctx->size].events = POLLIN | POLLOUT;
	ctx->size++;
}

void ctx_remove(CTX *ctx, size_t index)
{
	ctx->size--;

	memmove(ctx->sockets + index,
			ctx->sockets + (index + 1),
			sizeof(SOCK_CTX) * (ctx->capacity - index - 1));

	memmove(ctx->poll_fds + index,
			ctx->poll_fds + (index + 1),
			sizeof(struct pollfd) * (ctx->capacity - index - 1));
}

void ctx_destroy(CTX *ctx)
{

	// free(ctx->sockets);
	free(ctx->poll_fds);
	free(ctx);
}


void init_tcp_socket(CTX *ctx)
{

	int opt = 1;
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // SOCK_NONBLOCK

	if (sock == -1) {
		// perror();
		exit(1);
		// return -1;
	}

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	ctx->server_sock = sock;
}


int check_and_accept(CTX *ctx)
{
	// socklen_t addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in incoming_addr;

	struct pollfd poll_fd = {
		.fd = ctx->server_sock,
		.events = POLLIN,
		.revents = 0};

	poll(&poll_fd, 1, 0);
	if (poll_fd.revents & POLLIN) {
		// puts("rewrwe");
		int new_socket = accept(ctx->server_sock, (struct sockaddr*)&incoming_addr, &addrlen);
		ctx_add(ctx, new_socket);

		char *new_addr_str = inet_ntoa(incoming_addr.sin_addr);
		printf("new connection from: %s\n", new_addr_str);
		// printf("new connection\n");
		// free(new_addr_str);
	}
};

void client_disconenect(CTX *ctx, size_t index)
{
	puts("Client closed a connection");
	shutdown(ctx->poll_fds[index].fd, SHUT_RDWR);
	close(ctx->poll_fds[index].fd);

	ctx_remove(ctx, index);
}


int main()
{

	CTX *ctx = ctx_create();
	init_tcp_socket(ctx);

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PORT)
	};

	const char *server_addr = "0.0.0.0";

	inet_aton(server_addr, &addr.sin_addr);
	bind(ctx->server_sock, (struct sockaddr*)&addr, addrlen);

	listen(ctx->server_sock, 5);
	const char *response_str = "ack popopo";

	char buff[255] = {0};
	// struct pollfd poll_fd = {
	// 	.fd = new_socket,
	// 	.events = POLLIN,
	// 	.revents = 0};

	while (1) {
		check_and_accept(ctx);

		if (ctx->size == 0) {
			usleep(200000); // 100 ms
			continue;
		}

		poll(ctx->poll_fds, ctx->size, 0);
		// printf("%zu\n", ctx->size);
		for (size_t i = 0; i < ctx->size; i++) {
			if ((ctx->poll_fds[i].revents & POLLERR) || (ctx->poll_fds[i].revents & POLLHUP) || (ctx->poll_fds[i].revents & POLLNVAL)) {
				client_disconenect(ctx, i);
				i--;
				continue;
			}

			if (ctx->poll_fds[i].revents & POLLIN) {
				// puts("423423");
				ssize_t __total = recv(ctx->poll_fds[i].fd, buff, sizeof(buff), 0); // MSG_DONTWAIT
				// read(ctx->poll_fds[i].fd, buff, sizeof(buff));
				if (__total == -1) {
					client_disconenect(ctx, i);
					i--;
					continue;
				}
				if (__total == 0) {
					if (++ctx->sockets[i].inactivity_counter == 5) {
						client_disconenect(ctx, i);
						i--;
						continue;
					}
				} else {
					ctx->sockets[i].inactivity_counter = 0;
				}
				printf("new message (%zd): %s\n", __total, buff);
				memset(buff, 0, sizeof(buff));
			}

			if (ctx->poll_fds[i].revents & POLLOUT) {
				write(ctx->poll_fds[i].fd, response_str, 10); // 10 is hardcoded!
			}
			ctx->poll_fds[i].revents = 0;
		}

		usleep(200000);
	}

	shutdown(ctx->server_sock, SHUT_RDWR);
	close(ctx->server_sock);
	ctx_destroy(ctx);
	return 0;
}
