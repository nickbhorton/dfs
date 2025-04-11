#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "macros.h"
#include "stringview.h"
#include "tcp.h"

int listen_fd;

void handle_sigint(int signal)
{
    shutdown(listen_fd, SHUT_RDWR);
    fflush(stdout);
    fflush(stderr);
    exit(0);
}

int main()
{
    Fatal(signal(SIGCHLD, SIG_IGN) == SIG_ERR, "signal");

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    Fatal(sigemptyset(&sa.sa_mask) < 0, "sigemptyset");
    Fatal(sigaction(SIGINT, &sa, NULL) < 0, "sigaction");

    const char* server_address = "127.0.0.1";
    StringView server_address_sv = {.data = server_address, .length = strlen(server_address)};
    int16_t port = 10001;
    Fatal((listen_fd = tcp_bind(server_address_sv, port)) < 0, "tcp_bind");
    Fatal(listen(listen_fd, 128) < 0, "listen");

    while (1) {
        Connection client_connection = tcp_accept(listen_fd);
        if (client_connection.fd < 0) {
            printf("connection failed\n");
            continue;
        }
        if (!fork()) {
            // clean up connection
            shutdown(client_connection.fd, 2);
            exit(0);
        } else {
            close(client_connection.fd);
        }
    }
}
