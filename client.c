
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

	char msg[255] = {0};
	char buff[256] = {0};
	sprintf(msg + 1, "%s", "./test_1.txt");
	send(sock, msg, strlen(msg + 1) + 1, 0);

	recv(sock, buff, 1, 0);
	if (buff[0] != 1) {
		puts("Error on server side");
		return 0;
	}

	FILE *res_file = fopen("recieved.txt", "w");

	while (1) {
		// printf("-> ");
		// scanf("%s", msg);

		// send(sock, msg, strlen(msg), 0);
		ssize_t total = recv(sock, buff, 255, 0);

		printf("recieved: %s\n", buff);
		fwrite(buff, strlen(buff), 1, res_file);
		if (strlen(buff) < 255){
			fclose(res_file);
			break;
		}

		// memset(msg, 0, 255);
		memset(buff, 0, 255);
	}

	return 0;
}