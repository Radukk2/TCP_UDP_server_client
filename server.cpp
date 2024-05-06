#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "helpers.hpp"
#include "common.hpp"

using namespace std;


void run_chat_multi_server(int listenfd) {

	struct pollfd poll_fds[MAX_CONNECTIONS];
	int num_clients = 2;
	int rc;
	vector<struct client> clients;

	struct chat_packet received_packet;
	rc = listen(listenfd, MAX_CONNECTIONS);
	DIE(rc < 0, "listen");
	poll_fds[0].fd = listenfd;
	poll_fds[0].events = POLLIN;
	poll_fds[1].fd = STDIN_FILENO;
    poll_fds[1].events = POLLIN;

	while (1) {
		rc = poll(poll_fds, num_clients, -1);
		DIE(rc < 0, "poll");
		if (poll_fds[1].revents & POLLIN) {
		 	char buf[100];
			scanf("%s", buf);
			if (strncmp(buf, "exit", 4) == 0) {
				close(listenfd);
				for (int j = 0; j < num_clients; j++)
					close(poll_fds[j].fd);
				return;
			} else {
				continue;
			}
		}
		for (int i = 0; i < num_clients; i++)  {
			if (poll_fds[i].revents & POLLIN) {
				if (poll_fds[i].fd == listenfd) {
					struct client new_client;
					struct sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					int newsockfd =
						accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
					DIE(newsockfd < 0, "accept");
					char id[1024];
					rc = recv(newsockfd, id, sizeof(id), 0);
					bool exists = false;
					for (int j = 0 ; j < clients.size(); j++) {
						if (strcmp(clients[j].client_id, id) == 0) {
							printf("Client %s already connected.\n", id);
							exists = true;
							break;
						}
					}
					if (exists) {
						close(newsockfd);
						continue;
					}
					poll_fds[num_clients].fd = newsockfd;
					poll_fds[num_clients].events = POLLIN;
					num_clients++;
					new_client.addr = cli_addr;
					new_client.fd = newsockfd;
					strcpy(new_client.client_id, id);
					printf("New client %s connected from 127.0.0.1:%d.\n",
							new_client.client_id, ntohs(cli_addr.sin_port));
					clients.push_back(new_client);
				} else {
					int rc = recv_all(poll_fds[i].fd, &received_packet,
									  sizeof(received_packet));
					DIE(rc < 0, "recv");

					if (rc == 0) {
						for (int j = 0 ; j < clients.size(); j++) {
							if (clients[j].fd == poll_fds[i].fd) {
								printf("Client %s disconnected.\n", clients[j].client_id);
								close(poll_fds[i].fd);
								for (int j = i; j < num_clients - 1; j++) {
									poll_fds[j] = poll_fds[j + 1];
								}
								num_clients--;
								clients.erase(clients.begin() + j);
								break;
							}
						}
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2)
		return 1;
	setvbuf(stdout, NULL, 2, BUFSIZ);
	uint16_t port;
	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(listenfd < 0, "socket");
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);
	int enable = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");
	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");
	rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "bind");
	run_chat_multi_server(listenfd);
	return 0;
}