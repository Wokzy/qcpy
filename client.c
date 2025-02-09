
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <libgen.h>
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
	// sprintf(msg + 1, "%s", "/home/wokzy/films/Dexter.s4.BDRip.1080p/Dexter.s04e01.Living.the.Dream.mkv");
	sprintf(msg + 1, "%s", "./test_*.txt");
	send(sock, msg, strlen(msg + 1) + 1, 0);

	recv(sock, buff, 1 + 2*sizeof(int), 0);
	if (buff[0] != 1) {
		puts("Error on server side");
		return 0;
	}

	int total_files = *(int*)(buff + 1);

	printf("total files: %d\n", total_files);

	msg[0] = 1;
	for (int i = 0; i < total_files; i++) {
		send(sock, msg, 1, 0);
		char fname[256] = {0};
		fname[0] = 'r';
		fname[1] = '_';
		memset(buff, 0, 255);
		recv(sock, fname + 2, 253, 0);

		send(sock, msg, 1, 0);
		recv(sock, buff, sizeof(size_t), 0);

		size_t file_size = *(size_t*)buff;
		send(sock, msg, 1, 0);

		// printf("%s %zu\n", fname, file_size);

		FILE *res_file = fopen(fname, "w");

		clock_t start_clock = clock();
		size_t counter = 0;
		size_t counter_2 = 0;
		float speed = 0.0;

		size_t total_recieved = 0;

		memset(buff, 0, 255);
		while (1) {
			ssize_t total = recv(sock, buff, 255, 0);
			total_recieved += total;
			counter++;

			fwrite(buff, total, 1, res_file);
			if (total_recieved == file_size){
				fclose(res_file);
				printf("\33[2K\r");
				printf("\r%s    speed: %9.2f mb/s    100%%", fname, speed);
				break;
			}

			if (counter == 4096) {
				clock_t end_clock = clock();
				speed = (1.0 / ((float) (end_clock - start_clock) / (float) (CLOCKS_PER_SEC))) / 3;
				// counter_2++;
				// printf("\33[2K\r");
				printf("\rrecieving: %s    speed: %9.2f mb/s    %zu%%", fname, speed, (100*total_recieved) / file_size);

				start_clock = end_clock;
				counter = 0;
			}

			// memset(msg, 0, 255);
			memset(buff, 0, 255);
		}
		puts("");
	}
	send(sock, msg, 1, 0);
	close(sock);

	return 0;
}