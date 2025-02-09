
#include <time.h>
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
	sprintf(msg + 1, "%s", "./test_2.txt");
	send(sock, msg, strlen(msg + 1) + 1, 0);

	recv(sock, buff, 1, 0);
	if (buff[0] != 1) {
		puts("Error on server side");
		return 0;
	}

	FILE *res_file = fopen("recieved.txt", "w");

	clock_t start_clock = clock();
	size_t counter = 0;
	float speed = 0.0;

	while (1) {
		// printf("-> ");
		// scanf("%s", msg);

		// send(sock, msg, strlen(msg), 0);
		ssize_t total = recv(sock, buff, 255, 0);
		counter++;

		// printf("\rrecieving: %s    speed: %f mb/s", msg + 1, speed);
		fwrite(buff, strlen(buff), 1, res_file);
		if (strlen(buff) < 255){
			fclose(res_file);
			break;
		}

		if (counter == 4096) {
			printf("\rrecieving: %s    speed: %9.2f mb/s", msg + 1, speed);
			clock_t end_clock = clock();
			speed = (1.0f / ((float) (end_clock - start_clock) / (float) (CLOCKS_PER_SEC)));
			start_clock = end_clock;
			counter = 0;

		}

		// memset(msg, 0, 255);
		memset(buff, 0, 255);
	}

	return 0;
}