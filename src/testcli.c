#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <unistd.h>

#include "filestuff.h"
#include "protocol.h"
#include "stringview.h"
#include "tcp.h"

int main(int argc, char** argv)
{
    if (argc != 4) {
        printf("./testcli <port> <filename> [GET|PUT|TEST]\n");
        exit(EXIT_FAILURE);
    }
    long int port = strtol(argv[1], NULL, 10);
    if (port < 1024 || port > 65535) {
        printf("invalid port\n");
        exit(EXIT_FAILURE);
    }

    DfsRequest request = {};
    if (strncmp(argv[3], "GET", 3) == 0) {
        request.function = REQUEST_GET;
    } else if (strncmp(argv[3], "PUT", 3) == 0) {
        request.function = REQUEST_PUT;
    } else if (strncmp(argv[3], "TEST", 4) == 0) {
        request.function = REQUEST_TEST;
    } else if (strncmp(argv[3], "LS", 2) == 0) {
        request.function = REQUEST_LS;
    } else {
        printf("invalid function, [GET|PUT|TEST]\n");
        exit(EXIT_FAILURE);
    }

    StringView file_name_sv = {.data = argv[2], .length = strlen(argv[2])};
    hash_file_name(file_name_sv, request.hash);
    request.file_name = file_name_sv;

    StringView node = {.data = NULL, .length = 0};
    int fd = tcp_connect(node, port);
    if (fd < 0) {
        printf("failed to connect\n");
        exit(EXIT_FAILURE);
    }

    if (request.function == REQUEST_TEST) {
        ssize_t rv = send_request(fd, &request);
        if (rv < 0) {
            printf("send_request%zd\n", rv);
            exit(EXIT_FAILURE);
        }
        char ans;
        rv = tcp_recv(fd, &ans, 1);
        printf("%c", ans);
    } else if (request.function == REQUEST_PUT) {
        FILE* fptr = fopen(argv[2], "r");
        if (fptr == NULL) {
            printf("file does not exists\n");
            exit(EXIT_FAILURE);
        }
        ssize_t file_size = get_file_size(argv[2]);
        if (file_size < 0) {
            printf("get_file_size\n");
            exit(EXIT_FAILURE);
        }
        request.file_size = file_size;
        ssize_t rv = send_request(fd, &request);
        if (rv < 0) {
            printf("send_request %zd\n", rv);
            exit(EXIT_FAILURE);
        }
        ssize_t file_bytes_sent = sendfile(fd, fileno(fptr), 0, file_size);
        if (file_bytes_sent != file_size) {
            printf("sendfile %zd/%zd\n", file_bytes_sent, file_size);
            exit(EXIT_FAILURE);
        }
    } else if (request.function == REQUEST_GET) {
        ssize_t rv = send_request(fd, &request);
        if (rv < 0) {
            printf("send_request %zd\n", rv);
            exit(EXIT_FAILURE);
        }
        ssize_t file_size = -1;
        int file_size_bytes = tcp_recv(fd, (char*)&file_size, sizeof(file_size));
        if (file_size_bytes != sizeof(file_size)) {
            printf("tcp_recv %d\n", file_size_bytes);
            exit(EXIT_FAILURE);
        }
        if (!file_size) {
            printf("%s does not exist\n", argv[2]);
            exit(EXIT_SUCCESS);
        }
        char file_buffer[16384];
        ssize_t total_file_bytes_recv = 0;
        while (total_file_bytes_recv < file_size) {
            if (file_size - total_file_bytes_recv < 16384) {
                ssize_t file_bytes_recv = tcp_recv(fd, file_buffer, file_size - total_file_bytes_recv);
                if (file_bytes_recv <= 0) {
                    printf("tcp_recv\n");
                    exit(EXIT_FAILURE);
                }
                total_file_bytes_recv += file_bytes_recv;
                for (size_t i = 0; i < file_bytes_recv; i++) {
                    printf("%c", file_buffer[i]);
                }
            } else {
                ssize_t file_bytes_recv = tcp_recv(fd, file_buffer, 16384);
                if (file_bytes_recv <= 0) {
                    printf("tcp_recv\n");
                    exit(EXIT_FAILURE);
                }
                total_file_bytes_recv += file_bytes_recv;
                for (size_t i = 0; i < file_bytes_recv; i++) {
                    printf("%c", file_buffer[i]);
                }
            }
        }
    } else if (request.function == REQUEST_LS) {
        ssize_t rv = send_request(fd, &request);
        if (rv < 0) {
            printf("send_request %zd\n", rv);
            exit(EXIT_FAILURE);
        }
        ssize_t response_size = -1;
        int response_size_bytes = tcp_recv(fd, (char*)&response_size, sizeof(response_size));
        if (response_size_bytes != sizeof(response_size)) {
            printf("tcp_recv %d\n", response_size_bytes);
            exit(EXIT_FAILURE);
        }
        if (response_size > 0) {
            char response[LS_BUFFER_SIZE];
            response_size_bytes = tcp_recv(fd, response, response_size);
            if (response_size_bytes != response_size) {
                printf("tcp_recv %d\n", response_size_bytes);
                exit(EXIT_FAILURE);
            }
            printf("%.*s", (int)response_size_bytes, response);
        }
    }
    close(fd);
    return 0;
}
