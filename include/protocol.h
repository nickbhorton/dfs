#ifndef DFS_SERVER_HEADER
#define DFS_SERVER_HEADER

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "stringview.h"

#define REQUEST_TEST 1
#define REQUEST_GET 2
#define REQUEST_PUT 3
#define REQUEST_LS 4

#define LS_BUFFER_SIZE 4096

// packed because struct will be read into over tcp connection
typedef struct __attribute__((packed)) {
    uint8_t function;
    uint8_t hash[16];
    size_t file_size;
} DfsRequestHeader;

typedef struct __attribute__((packed)) {
    uint8_t function;
    uint8_t hash[16];
    size_t file_size;
    StringView file_name;
} DfsRequest;

ssize_t recv_request(int sock_fd, DfsRequest* request_o);
ssize_t send_request(int sock_fd, DfsRequest const* request);

#endif
