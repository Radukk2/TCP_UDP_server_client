#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <iostream>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <set>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <regex>
#include "helpers.hpp"
#include "common.hpp"

using namespace std;

struct Packet {
	char topic[50];
	char type;
	char content[1501];
};

int receive_and_send(int connfd1, int connfd2, size_t len) {
	int bytes_received;
	char buffer[len];
	bytes_received = recv_all(connfd1, buffer, len);
	if (bytes_received == 0) {
		return 0;
	}
	DIE(bytes_received < 0, "recv");
	int rc = send_all(connfd2, buffer, len);
	if (rc <= 0) {
		perror("send_all");
		return -1;
	}

	return bytes_received;
}


void run_chat_multi_server(int listenfd, int udpfd) {

	struct pollfd poll_fds[MAX_CONNECTIONS];
	int num_clients = 3;
	int rc;
	vector<struct client> clients;

	struct chat_packet received_packet;
	rc = listen(listenfd, MAX_CONNECTIONS);
	DIE(rc < 0, "listen");
	poll_fds[0].fd = listenfd;
	poll_fds[0].events = POLLIN;
	poll_fds[1].fd = STDIN_FILENO;
    poll_fds[1].events = POLLIN;
	poll_fds[2].fd = udpfd;
	poll_fds[2].events = POLLIN;

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
							if (clients[j].isOn) {
								printf("Client %s already connected.\n", id);
								close(newsockfd);
							} else {
								clients[j].isOn = true;
								printf("New client %s connected from 127.0.0.1:%d.\n",
									id, ntohs(cli_addr.sin_port));
							}
							exists = true;
							break;
						}
					}
					if (exists)
						continue;
					poll_fds[num_clients].fd = newsockfd;
					poll_fds[num_clients].events = POLLIN;
					num_clients++;
					new_client.addr = cli_addr;
					new_client.fd = newsockfd;
					new_client.isOn = true;
					strcpy(new_client.client_id, id);
					printf("New client %s connected from 127.0.0.1:%d.\n",
							new_client.client_id, ntohs(cli_addr.sin_port));
					clients.push_back(new_client);
				} else if (poll_fds[i].fd == udpfd) {
					struct Packet packet;
					struct sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					int rc = recvfrom(udpfd, &packet, sizeof(struct Packet), 0, (struct sockaddr *)&cli_addr, &cli_len);
					for (int j = 0; j < num_clients; j++) {
						if (j > 2 )
							for (int it = 0; it < clients.size(); it++) {
								 if (clients[it].fd == poll_fds[j].fd && clients[it].isOn) {
									// auto top = clients[it].topics.find(packet.topic);
									// if (top != clients[it].topics.end())
									// 	send_all(poll_fds[j].fd, &packet, sizeof(packet));
									for (auto iter = clients[it].topics.begin(); iter != clients[it].topics.end(); ++iter) {
										const char* topic = (const char *)packet.topic;
										if (string((const char *)packet.topic) == *iter) {
											send_all(poll_fds[j].fd, &packet, sizeof(packet));
											continue;
										}
										if (iter->find('*') != string::npos && iter->find('+') != string::npos) {
											string topic  = string((const char *)packet.topic);
											string regexPrep = *iter;
											regexPrep = regex_replace(regexPrep, regex("\\*"), ".*");
											regexPrep = regex_replace(regexPrep, regex("\\+"), "[^/]+");
											if (regex_match(topic, regex(regexPrep))) {
												send_all(poll_fds[j].fd, &packet, sizeof(packet));
												break;
											}
											continue;
										}
										if (iter->find('*') != string::npos) {
											string topic  = string((const char *)packet.topic);
											string regexPrep = *iter;
											regexPrep = regex_replace(regexPrep, regex("\\*"), ".*");
											if (regex_match(topic, regex(regexPrep))) {
												send_all(poll_fds[j].fd, &packet, sizeof(packet));
												break;
											}

										}
										if (iter->find('+') != string::npos) {
											string topic  = string((const char *)packet.topic);
											string regexPrep = *iter;
											regexPrep = regex_replace(regexPrep, regex("\\+"), "[^/]+");
											if (regex_match(topic, regex(regexPrep))) {
												send_all(poll_fds[j].fd, &packet, sizeof(packet));
												break;
											}
										}
									}
									
								}
							}
					}

				} else {
					int rc = recv_all(poll_fds[i].fd, &received_packet,
									  sizeof(received_packet));
					if (!strncmp("subscribe", received_packet.message, 9)) {
						for (int j = 0 ; j < clients.size(); j++) {
							if (clients[j].fd == poll_fds[i].fd && clients[j].isOn) {
								clients[j].topics.insert(string((const char *) received_packet.message + 9));
								// for (const auto& topic : clients[j].topics) {
								// 	cout<<topic<<" ";
								// }
								// cout<<"\n";
								break;
							}
						}
					}
					if (!strncmp("unsubscribe", received_packet.message, 11)) {
						for (int j = 0 ; j < clients.size(); j++) {
							if (clients[j].fd == poll_fds[i].fd) {
								clients[j].topics.erase(string((const char *) received_packet.message + 11));
							}
						}
					}
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
								clients[j].isOn = false;
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
	int enable = 1;
	rc = setsockopt(listenfd, IPPROTO_TCP, enable, (char *)&enable, sizeof(int));
        DIE(rc == -1, "Failed to set socket options");
	int udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udpfd < 0, "socket");
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);
	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");
	rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	rc = bind(udpfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "bind");
	run_chat_multi_server(listenfd, udpfd);
	return 0;
}
