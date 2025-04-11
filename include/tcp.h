#ifndef DFS_TCP_HEADER
#define DFS_TCP_HEADER

#include <stdint.h>
#include <sys/socket.h>

#include "stringview.h"

typedef struct {
    struct sockaddr_storage address;
    socklen_t address_length;
} Address;

typedef struct {
    int fd;
    Address address;
} Connection;

int tcp_bind(StringView node, int16_t port);
int tcp_conect(StringView node, int16_t port);
Connection tcp_accept(int listen_fd);

#endif
