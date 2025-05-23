#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/poll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "filestuff.h"
#include "macros.h"
#include "protocol.h"
#include "stringview.h"
#include "tcp.h"

#define OVERWRITE 1

#define TEST_RESPONSE_EXIST 'y'
#define TEST_RESPONSE_NEXIST 'n'

int listen_fd;

void handle_sigint(int signal);
void attempt_mkdir(const char* dir_name);
void filename_from_request(DfsRequest const* request, const char* dir, char* file_name_o, size_t size);

int main(int argc, char** argv)
{
    if (argc != 3) {
        printf("dfs <directory> <port>\n");
        exit(EXIT_FAILURE);
    }

    // port
    long int port = strtol(argv[2], NULL, 10);
    if (port < 1024 || port > 65535) {
        printf("invalid port\n");
        exit(EXIT_FAILURE);
    }

    // directory
    const char* dir = argv[1];
    attempt_mkdir(dir);

    // handle signals
    Fatal(signal(SIGCHLD, SIG_IGN) == SIG_ERR, "signal");
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    Fatal(sigemptyset(&sa.sa_mask) < 0, "sigemptyset");
    Fatal(sigaction(SIGINT, &sa, NULL) < 0, "sigaction");

    // bind server address
    StringView server_address_sv = {.data = NULL, .length = 0};
    Fatal((listen_fd = tcp_bind(server_address_sv, port)) < 0, "tcp_bind");
    Fatal(listen(listen_fd, 128) < 0, "listen");

    while (1) {
        Connection client_connection = tcp_accept(listen_fd);
        if (!fork()) {
            while (1) {
                DfsRequest request = {};
                ssize_t bytes_recv = recv_request(client_connection.fd, &request);
                if (bytes_recv < 0) {
                    // assume that client closed connection
                    shutdown(client_connection.fd, 2);
                    exit(0);
                }

                // make filename from request
                char file_name[256];
                if (request.file_name.length > 0) {
                    filename_from_request(&request, dir, file_name, sizeof(file_name));
                    printf("%s\n", file_name);
                }

                switch (request.function) {
                case REQUEST_TEST: {
                    int fd = open(file_name, O_RDONLY, 0);
                    char file_exist;
                    if (fd >= 0) {
                        file_exist = TEST_RESPONSE_EXIST;
                        close(fd);
                    } else {
                        file_exist = TEST_RESPONSE_NEXIST;
                    }
                    if (send(client_connection.fd, &file_exist, 1, 0) != 1) {
                        printf("send failed during TEST\n");
                    }
                    break;
                }
                case REQUEST_GET: {
                    int fd = open(file_name, O_RDONLY, 0);
                    ssize_t file_size = 0;
                    if (fd >= 0) {
                        file_size = get_file_size(file_name);
                    }
                    if (file_size < 0) {
                        printf("get_file_size failed during GET\n");
                        break;
                    }
                    // send the file_size as bytes
                    ssize_t size_sent = send(client_connection.fd, (char*)&file_size, sizeof(file_size), 0);
                    if (size_sent != sizeof(file_size)) {
                        printf("send failed during send file_size for GET (%zi/%zu)\n", size_sent, sizeof(file_size));
                    }
                    // if there is no file dont send file
                    if (!file_size) {
                        break;
                    }
                    flock(fd, LOCK_SH);
                    ssize_t bytes_sent = sendfile(client_connection.fd, fd, 0, file_size);
                    flock(fd, LOCK_UN);
                    close(fd);
                    if (bytes_sent != file_size) {
                        printf("sendfile failed during GET (%zi/%zu)\n", bytes_sent, file_size);
                    }
                    break;
                }
                case REQUEST_PUT: {
                    int fd;
                    if (OVERWRITE)
                        fd = open(file_name, O_CREAT | O_WRONLY, S_IRWXU);
                    else
                        fd = open(file_name, O_CREAT | O_EXCL | O_WRONLY, S_IRWXU);
                    if (fd < 0) {
                        printf("put (%zu) failed to open\n", request.file_size);
                        break;
                    }
                    flock(fd, LOCK_EX);
                    char buffer[4096];
                    bytes_recv = 0;
                    while (bytes_recv < request.file_size) {
                        size_t size_to_recv =
                            (request.file_size - bytes_recv) < 4096 ? (request.file_size - bytes_recv) : 4096;
                        ssize_t rv = recv(client_connection.fd, buffer, size_to_recv, 0);
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
                    flock(fd, LOCK_UN);
                    close(fd);

                    // this may create multithreaded issues
                    if (bytes_recv != request.file_size) {
                        unlink(file_name);
                    }
                    break;
                }
                case REQUEST_LS: {
                    char ls_resp[LS_BUFFER_SIZE];
                    int bytes_written = snprintf(ls_resp, sizeof(ls_resp), "/bin/ls %s", dir);
                    FILE* fp = popen(ls_resp, "r");
                    memset(ls_resp, 0, bytes_written);
                    if (fp == NULL) {
                        printf("Failed to run command\n");
                        exit(1);
                    }
                    size_t i = 0;
                    while ((ls_resp[i] = fgetc(fp)) != EOF) {
                        i++;
                    }
                    pclose(fp);

                    // send the size as bytes
                    ssize_t size_sent = tcp_send(client_connection.fd, (char*)&i, sizeof(i));
                    if (size_sent != sizeof(i)) {
                        printf("send failed during send size for LS (%zi/%zu)\n", size_sent, sizeof(i));
                    }

                    // if there is a list, send the list
                    if (i > 0) {
                        size_sent = tcp_send(client_connection.fd, ls_resp, i);
                        if (size_sent != i) {
                            printf("send failed during send file list for LS (%zi/%zu)\n", size_sent, i);
                        }
                    }
                    break;
                }
                default:
                    fprintf(stderr, "unknown request function %d\n", request.function);
                }
            }
        } else {
            close(client_connection.fd);
        }
    }
}

void handle_sigint(int signal)
{
    shutdown(listen_fd, SHUT_RDWR);
    fflush(stdout);
    fflush(stderr);
    exit(0);
}

void attempt_mkdir(const char* dir_name)
{
    struct stat st = {};
    if (stat(dir_name, &st) < 0) {
        Fatal(mkdir(dir_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0, "mkdir");
    }
}

void filename_from_request(DfsRequest const* request, const char* dir, char* file_name_o, size_t size)
{
    snprintf(file_name_o, size, "%s/%.*s", dir, (int)request->file_name.length, request->file_name.data);
}
