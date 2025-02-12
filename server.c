
// #define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/ip.h>
#include <glob.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>

#include "misc.h"
#include "server.h"

#define PORT 55763
#define BUF_SIZE 8192

socklen_t addrlen = sizeof(struct sockaddr_in);

ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count)
{
	off_t orig;

	if (offset != NULL) {

		/* Save current file offset and set offset to value in '*offset' */

		orig = lseek(in_fd, 0, SEEK_CUR);
		if (orig == -1)
			return -1;
		if (lseek(in_fd, *offset, SEEK_SET) == -1)
			return -1;
	}

	size_t totSent = 0;

	while (count > 0) {
		size_t toRead = BUF_SIZE;
		if (count < toRead)
			toRead = count;

		char buf[BUF_SIZE];
		ssize_t numRead = read(in_fd, buf, toRead);
		if (numRead == -1)
			return -1;
		if (numRead == 0)
			break;                      /* EOF */

		ssize_t numSent = write(out_fd, buf, numRead);
		if (numSent == -1)
			return -1;
		// if (numSent == 0)               /* Should never happen */
		// 	fatal("sendfile: write() transferred 0 bytes");

		count -= numSent;
		totSent += numSent;
	}

	if (offset != NULL) {

		/* Return updated file offset in '*offset', and reset the file offset
		   to the value it had when we were called. */

		*offset = lseek(in_fd, 0, SEEK_CUR);
		if (*offset == -1)
			return -1;
		if (lseek(in_fd, orig, SEEK_SET) == -1)
			return -1;
	}

	return totSent;
}


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
	ctx->sockets[ctx->size].state = INIT;
	ctx->poll_fds[ctx->size].fd = sock;
	ctx->poll_fds[ctx->size].events = POLLIN | POLLOUT;
	ctx->size++;
}

void ctx_remove(CTX *ctx, size_t index)
{
	ctx->size--;

	if (ctx->sockets[index].files_list != NULL) {
		for (size_t i = 0; i < ctx->sockets[index].total_files; i++)
			free(ctx->sockets[index].files_list[i]);
		free(ctx->sockets[index].files_list);
	}

	if (ctx->sockets[index].file_descriptor > 0) {
		close(ctx->sockets[index].file_descriptor);
	}

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

void read_dir_content(SOCK_CTX *socket_ctx, const char *dir) {
	DIR *directory = opendir(dir);
	// if (directory == NULL)
	// 	return -1;

	glob_t globbuf;
	glob(dir, 0, NULL, &globbuf);
	socket_ctx->files_list = globbuf.gl_pathv;
	// puts(socket_ctx->files_list[0]);

	socket_ctx->total_files = (int)(globbuf.gl_pathc);
	// socket_ctx->files_list = nc_calloc(socket_ctx->total_files, sizeof(char*));
	// for (size_t i = 0; i < 1; i++)
	// 	socket_ctx->files_list[i] = nc_calloc(255, sizeof(char));

	// memcpy(socket_ctx->files_list[0], dir, strlen(dir));
}

// TEMPORARY
int init_file(SOCK_CTX *socket_ctx, const char *fname, int mode) {

	if (mode == 0) {
		if (access(fname, F_OK) != 0)
			return -1;

		struct stat info;
		stat(fname, &info);
		socket_ctx->file_size = (size_t)info.st_size;
		socket_ctx->file_descriptor = open(fname, O_RDONLY);
	}
	else {
		socket_ctx->file_descriptor = open(fname, O_WRONLY | O_CREAT);
		if (socket_ctx->file_descriptor == -1)
			return -1;
		return -1;
	}

	// puts("file initted");

	return 0;
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

	char buff[MAX_MSG_LENGTH] = {0};
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
				goto client_disconenection;
			}

			if (ctx->poll_fds[i].revents & POLLIN) {
				ssize_t __total = recv(ctx->poll_fds[i].fd, buff, sizeof(buff), 0); // MSG_DONTWAIT
				if (__total == -1) {
					goto client_disconenection;
				}
				if (__total == 0) {
					if (++ctx->sockets[i].inactivity_counter == 5)
						goto client_disconenection;
					continue;
				} else {
					ctx->sockets[i].inactivity_counter = 0;
				}

				// printf("new message (%zd): %s\n", __total, buff + 1);
				if (ctx->sockets[i].state == INIT) {
					if (buff[0] == 0) {
						read_dir_content(ctx->sockets + i, buff + 1);
						ctx->sockets[i].state = INIT_TRANSMITTING;
					} else {
						goto client_disconenection;
					}
				} else if (ctx->sockets[i].state == WAITING1) {
					ctx->sockets[i].state = INIT_TRANSMITTING2;
				} else if (ctx->sockets[i].state == WAITING2) {
					init_file(ctx->sockets + i, ctx->sockets[i].files_list[ctx->sockets[i].files_sent], 0);
					ctx->sockets[i].state = INIT_TRANSMITTING3;
				} else if (ctx->sockets[i].state == WAITING3) {
					ctx->sockets[i].state = TRANSMITTING;
				} else if (ctx->sockets[i].state == TRANSMITTING) {
					close(ctx->sockets[i].file_descriptor);
					if (ctx->sockets[i].files_sent++ == ctx->sockets[i].total_files) {
						goto client_disconenection;
					} else {
						ctx->sockets[i].state = INIT_TRANSMITTING2;
					}
				}
				memset(buff, 0, sizeof(buff));
			}

			if (ctx->poll_fds[i].revents & POLLOUT) {
				// printf("%d\n", ctx->sockets[i].state);
				char res[256] = {0};
				switch (ctx->sockets[i].state){
					case INIT_TRANSMITTING:
						res[0] = 1;
						*(int*)(res + 1) = ctx->sockets[i].total_files;
						// read_dir_content(ctx->sockets + i, );
						// struct stat info;
						// stat(argv[1], &info);

						write(ctx->poll_fds[i].fd, res, 1 + sizeof(int));
						ctx->sockets[i].state = WAITING1;
						break;
					case INIT_TRANSMITTING2:
						sprintf(res, "%s", basename(ctx->sockets[i].files_list[ctx->sockets[i].files_sent]));
						write(ctx->poll_fds[i].fd, res, 1 + strlen(res));
						ctx->sockets[i].state = WAITING2;
						break;
					case INIT_TRANSMITTING3:
						*(size_t*)(res) = ctx->sockets[i].file_size;
						write(ctx->poll_fds[i].fd, res, sizeof(size_t));
						ctx->sockets[i].state = WAITING3;
						break;
					case ERROR:
						res[0] = -1;
						write(ctx->poll_fds[i].fd, res, 1);
						goto client_disconenection;
					case FINISHING:
						res[0] = 0;
						write(ctx->poll_fds[i].fd, res, 1);
						// goto client_disconenection;
						break;
					case TRANSMITTING:
						sendfile(ctx->poll_fds[i].fd, ctx->sockets[i].file_descriptor, NULL, TRANSMIT_BUFF_SIZE);
						break;
				}
				// write(ctx->poll_fds[i].fd, response_str, 10); // 10 is hardcoded!
			}
			ctx->poll_fds[i].revents = 0;

			continue;
			client_disconenection:
				client_disconenect(ctx, i);
				i--;
		}

		// usleep(200000);
	}

	shutdown(ctx->server_sock, SHUT_RDWR);
	close(ctx->server_sock);
	ctx_destroy(ctx);
	return 0;
}
