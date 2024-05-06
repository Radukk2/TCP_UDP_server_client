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

#define MAX_CONNECTIONS 32

#define MSG_MAXSIZE 1024

struct chat_packet
{
    uint16_t len;
    char message[MSG_MAXSIZE + 1];
};

int recv_all(int sockfd, void *buffer, size_t len)
{
    size_t bytes_received = 0;
    size_t bytes_remaining = len;
    char *buff = (char *)buffer;
    while (bytes_remaining)
    {
        size_t rec = recv(sockfd, buff, bytes_remaining, 0);
        if (rec <= 0)
            return rec;
        bytes_remaining -= rec;
        bytes_received += rec;
        buff += rec;
    }
    return bytes_received;
}

int send_all(int sockfd, void *buffer, size_t len)
{
    size_t bytes_sent = 0;
    size_t bytes_remaining = len;
    char *buff = (char *)buffer;
    while (bytes_remaining)
    {
        size_t rec = send(sockfd, buff, bytes_remaining, 0);
        if (rec <= 0)
            return rec;
        bytes_remaining -= rec;
        bytes_sent += rec;
        buff += rec;
    }
    return bytes_sent;
}