#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "filestuff.h"
#include "macros.h"
#include "protocol.h"
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

    const char* directory = "./dfs1/";
    const char* server_address = "127.0.0.1";
    int16_t port = 10001;
    StringView server_address_sv = {.data = server_address, .length = strlen(server_address)};
    Fatal((listen_fd = tcp_bind(server_address_sv, port)) < 0, "tcp_bind");
    Fatal(listen(listen_fd, 128) < 0, "listen");

    while (1) {
        Connection client_connection = tcp_accept(listen_fd);
        if (client_connection.fd < 0) {
            printf("connection failed\n");
            continue;
        }
        if (!fork()) {
            DfsRequest request = {};
            ssize_t bytes_recv = recv(client_connection.fd, (char*)&request, sizeof(DfsRequest), 0);
            if (bytes_recv < sizeof(DfsRequest)) {
                fprintf(stderr, "recv did not fully fill out DfsRequest %zd\n", bytes_recv);
                shutdown(client_connection.fd, 2);
                exit(1);
            }

            char file_name[128];
            memcpy(file_name, directory, strlen(directory));
            hexify_hash(request.hash, file_name + strlen(directory));
            printf("%s\n", file_name);

            switch (request.function) {
            case REQUEST_TEST: {
                printf("test\n");
                break;
            }
            case REQUEST_GET: {
                printf("get\n");
                break;
            }
            case REQUEST_PUT: {
                // int fd = open(file_name, O_CREAT | O_EXCL | O_WRONLY, S_IRWXU); // this version ensures no overwrites
                int fd = open(file_name, O_CREAT | O_WRONLY, S_IRWXU);
                if (fd < 0) {
                    printf("put (%zu) failed to open\n", request.file_size);
                    break;
                }
                char buffer[4096];
                bytes_recv = 0;
                while (bytes_recv < request.file_size) {
                    ssize_t rv = recv(client_connection.fd, buffer, 4096, 0);
                    if (rv <= 0) {
                        printf("recv failed during put (%zi/%zu)\n", bytes_recv, request.file_size);
                        break;
                    }
                    if (write(fd, buffer, rv) != rv) {
                        printf("write failed during put (%zi/%zu)\n", bytes_recv, request.file_size);
                        break;
                    }
                    bytes_recv += rv;
                }
                close(fd);
                break;
            }
            default:
                fprintf(stderr, "unknown request function %d\n", request.function);
            }

            shutdown(client_connection.fd, 2);
            exit(0);
        } else {
            close(client_connection.fd);
        }
    }
}
