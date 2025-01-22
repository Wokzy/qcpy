
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>

#define PORT 55763

int init_tcp_socket()
{

	int opt = 1;
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // SOCK_NONBLOCK

	if (sock == -1)
		return -1;

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	return sock;
}


int main()
{

	int sockfd = init_tcp_socket();

	if (sockfd == -1)
		return 1;

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PORT)
	};
	socklen_t addrlen = sizeof(struct sockaddr_in);

	const char *server_addr = "0.0.0.0";

	inet_aton(server_addr, &addr.sin_addr);
	bind(sockfd, (struct sockaddr*)&addr, addrlen);

	listen(sockfd, 5);
	struct sockaddr_in incoming_addr;
	int new_socket = accept(sockfd, (struct sockaddr*)&incoming_addr, &addrlen);

	char *new_addr_str = inet_ntoa(incoming_addr.sin_addr);
	char *response_str = "ack popopo";

	char buff[255];
	struct pollfd poll_fd = {
		.fd = new_socket,
		.events = POLLIN,
		.revents = 0};

	while (1) {
		poll(&poll_fd, 1, 200);
		if ((poll_fd.revents & POLLERR) || (poll_fd.revents & POLLHUP)) {
			shutdown(new_socket, SHUT_RDWR);
			close(new_socket);

			shutdown(sockfd, SHUT_RDWR);
			close(sockfd);

			puts("Client closed a connection");
			return 0;
		}

		if (poll_fd.revents & POLLIN) {
			recv(new_socket, buff, sizeof(buff), MSG_DONTWAIT);
			// read(new_socket, buff, sizeof(buff));
			printf("new message from (%s): %s\n", new_addr_str, buff);
			memset(buff, 0, sizeof(buff));

			// this may block the socket!!!
			write(new_socket, response_str, 10); // 10 is hardcoded!
		}
	}

	free(new_addr_str);
	return 0;
}
