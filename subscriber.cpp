#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "helpers.hpp"
#include "common.hpp"

void run_client(int sockfd) {
	char buf[MSG_MAXSIZE + 1];
	memset(buf, 0, MSG_MAXSIZE + 1);

	struct chat_packet sent_packet;
	struct chat_packet recv_packet;
	struct pollfd fds[2];
	fds[1].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	fds[0].fd = sockfd;
	fds[1].events = POLLIN;
	while (1) {
		poll(fds, 2, -1);
		if ((fds[0].revents & POLLIN)) {
			int rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
			if (rc == 0) {
				close(sockfd);
				return;
			}
			printf("%s", recv_packet.message);
		}
		if (fds[1].revents & POLLIN) {
				scanf("%s", buf);
				strcpy(sent_packet.message, buf);
				sent_packet.len = strlen(buf) + 1;
				//exit
				if (strncmp("exit", buf, 4) == 0) {
					close(sockfd);
					return;
				}
				//subscribe
				if (strncmp(buf, "subscribe", 9) == 0) {
					char topic[100];
					scanf("%s", topic);
					printf("Subscribed to topic %s\n", topic);
					strcat(sent_packet.message, topic);
					sent_packet.len += strlen(topic);
				}
				if (strncmp(buf, "unsubscribe", 11) == 0) {
					char topic[100];
					printf("Unsubscribed from topic %s\n", topic);
					strcat(sent_packet.message, topic);
					sent_packet.len += strlen(topic);
				}
				send_all(sockfd, &sent_packet, sizeof(sent_packet));
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		printf("\n Usage: %s <ip> <port>\n", argv[0]);
		return 1;
	}
	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");
	const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);
	setvbuf(stdout, NULL, 2, BUFSIZ);
	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");
	rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	rc = send(sockfd, argv[1], sizeof(argv[1]), 0);
	DIE(rc < 0, "connect");
	int flag = 1;
	int result = setsockopt(sockfd, IPPROTO_TCP, 1, (char *)&flag, sizeof(int));
	run_client(sockfd);
	return 0;
}
