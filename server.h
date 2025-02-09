#pragma once

#include <stdlib.h>
#include <poll.h>

#define MAX_MSG_LENGTH 65536

enum client_state {
	INIT,
	INIT_TRANSMITTING,
	INIT_RECIEVING,
	RECIEVING,
	TRANSMITTING,
	FINISHING,
	ERROR
};

typedef struct
{
	int fd;
	int inactivity_counter; // if this field reaches 5, connection will be dropped.
	enum client_state state;
	size_t total_files;
	size_t files_sent; // in case of transmitting
	char **files_list;
	int file_descriptor;
	char *destination; // in case of recieving
} SOCK_CTX;

typedef struct
{
	int server_sock;
	SOCK_CTX *sockets;
	struct pollfd *poll_fds;
	size_t size;
	size_t capacity;
} CTX;

void init_tcp_socket(CTX *ctx);
int check_and_accept(CTX *ctx);
void client_disconenect(CTX *ctx, size_t index);

CTX *ctx_create();
void ctx_add(CTX *ctx, int sock);
void ctx_remove(CTX *ctx, size_t index);
void ctx_destroy(CTX *ctx);

int read_dir_content(SOCK_CTX *, const char *);
