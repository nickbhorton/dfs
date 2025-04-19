#ifndef DFS_TCP_HEADER
#define DFS_TCP_HEADER

#include <stdint.h>
#include <sys/socket.h>

#include "stringview.h"

#define TCP_MAX_CALLS 10
#define TCP_MAX_TIME_MS 1000

typedef struct {
    struct sockaddr_storage address;
    socklen_t address_length;
} Address;

typedef struct {
    int fd;
    Address address;
} Connection;

int tcp_bind(StringView node, int16_t port);
int tcp_connect(StringView node, int16_t port);
Connection tcp_accept(int listen_fd);
ssize_t tcp_recv(int fd, char* buffer, size_t size);
ssize_t tcp_send(int fd, const char* buffer, size_t size);

#endif
