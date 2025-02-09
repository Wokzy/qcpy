
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>

#define SERVER_PORT 55763


int main()
{
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in serv_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(SERVER_PORT)
	};
	inet_aton("127.0.0.1", &serv_addr.sin_addr);

	connect(sock, (const struct sockaddr*)&serv_addr, sizeof(serv_addr));

	char msg[255];
	char buff[11];
	while (1) {
		printf("-> ");
		scanf("%s", msg);

		send(sock, msg, strlen(msg), 0);
		recv(sock, buff, 10, 0);
		printf("recieved: %s\n", buff);
		memset(msg, 0, 255);
		memset(buff, 0, 11);
	}

	return 0;
}