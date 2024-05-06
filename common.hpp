#ifndef COMMON_H
#define COMMON_H

#define MAX_CONNECTIONS 32

#define MSG_MAXSIZE 1024

struct chat_packet
{
    uint16_t len;
    char message[MSG_MAXSIZE + 1];
};

struct client
{
    char client_id[MSG_MAXSIZE];
    struct sockaddr_in addr;
    int fd;
};


int recv_all(int sockfd, void *buffer, size_t len);

int send_all(int sockfd, void *buffer, size_t len);

#endif