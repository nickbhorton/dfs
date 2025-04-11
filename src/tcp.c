#include "tcp.h"

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "stringview.h"

Connection tcp_accept(int sockfd)
{
    Connection result = {};
    result.address.address_length = sizeof(result.address.address);
    int rv;
    while (1) {
        rv = accept(sockfd, (struct sockaddr*)&result.address.address, &result.address.address_length);
        if (rv < 0) {
            if (errno == ECONNABORTED || errno == EINTR) {
                continue;
            }
        }
        break;
    }
    result.fd = rv;
    return result;
}

static int tcp_address_info(const char* node, const char* service, struct addrinfo** ai_list)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (node == NULL) {
        hints.ai_flags = AI_PASSIVE;
    }
    int rv = getaddrinfo(node, service, &hints, ai_list);
    return rv;
}

static int tcp_generic(StringView node, int16_t port, bool bind_node)
{
    struct addrinfo* ai_list = NULL;

    static char node_cstr[512];
    if (node.data != NULL || node.length != 0) {
        snprintf(node_cstr, 512, "%.*s", (int)node.length, node.data);
    }

    static char port_cstr[128];
    snprintf(port_cstr, 128, "%d", port);

    int rv = tcp_address_info((node.data == NULL || node.length == 0) ? NULL : node_cstr, port_cstr, &ai_list);
    if (rv != 0) {
        fprintf(stderr, "tcp_connect: %s\n", gai_strerror(rv));
        return -1;
    }

    int socket_fd;
    struct addrinfo* ai_ptr;
    for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next) {
        socket_fd = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
        if (socket_fd < 0) {
            continue;
        }

        if (bind_node) {
            static const int yes = 1;
            if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
                perror("setsockopt");
                freeaddrinfo(ai_list);
                return -1;
            }
        }

        rv = bind_node ? bind(socket_fd, ai_ptr->ai_addr, ai_ptr->ai_addrlen)
                       : connect(socket_fd, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
        if (rv < 0) {
            continue;
        }
        break;
    }

    if (ai_ptr == NULL) {
        fprintf(stderr, "tcp_connect: %s\n", "failed to find server");
        freeaddrinfo(ai_list);
        return -1;
    }

    freeaddrinfo(ai_list);
    return socket_fd;
}

int tcp_bind(StringView node, int16_t port) { return tcp_generic(node, port, true); }
int tcp_conect(StringView node, int16_t port) { return tcp_generic(node, port, false); }
