#include "protocol.h"
#include "tcp.h"

#define FILE_NAME_BUFFER_SIZE 256

ssize_t recv_request(int sock_fd, DfsRequest* request_o)
{
    static char file_name_buffer[FILE_NAME_BUFFER_SIZE];

    ssize_t total_bytes_recv = 0;
    // header
    ssize_t bytes_recv = tcp_recv(sock_fd, (char*)request_o, sizeof(DfsRequestHeader));
    if (bytes_recv < sizeof(DfsRequestHeader)) {
        return -1;
    }
    total_bytes_recv += bytes_recv;

    // file_name length
    bytes_recv = tcp_recv(sock_fd, (char*)&(request_o->file_name.length), sizeof(request_o->file_name.length));
    if (bytes_recv != sizeof(request_o->file_name.length)) {
        return -2;
    } else if (request_o->file_name.length >= FILE_NAME_BUFFER_SIZE) {
        return -4;
    }
    total_bytes_recv += bytes_recv;

    // file_name
    bytes_recv = tcp_recv(sock_fd, file_name_buffer, request_o->file_name.length);
    if (bytes_recv != request_o->file_name.length) {
        return -3;
    }
    file_name_buffer[request_o->file_name.length] = '\0';
    request_o->file_name.data = file_name_buffer;

    // printf("file_name recv: %s\n", request_o->file_name.data);
    return total_bytes_recv;
}

ssize_t send_request(int sock_fd, DfsRequest const* request)
{
    ssize_t total_bytes_sent = 0;
    // header
    ssize_t bytes_sent = tcp_send(sock_fd, (char*)request, sizeof(DfsRequestHeader));
    if (bytes_sent < sizeof(DfsRequestHeader)) {
        return -1;
    }
    total_bytes_sent += bytes_sent;

    // file_name length
    bytes_sent = tcp_send(sock_fd, (char const*)&(request->file_name.length), sizeof(request->file_name.length));
    if (bytes_sent != sizeof(request->file_name.length)) {
        return -2;
    }
    total_bytes_sent += bytes_sent;

    // file_name
    bytes_sent = tcp_send(sock_fd, (char const*)request->file_name.data, request->file_name.length);
    if (bytes_sent != request->file_name.length) {
        return -3;
    }
    total_bytes_sent += bytes_sent;
    return total_bytes_sent;
}
