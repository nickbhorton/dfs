#ifndef DFS_SERVER_HEADER
#define DFS_SERVER_HEADER

#include <stddef.h>
#include <stdint.h>

#define REQUEST_TEST 1
#define REQUEST_GET 2
#define REQUEST_PUT 3

// packed because struct will be read into over tcp connection
typedef struct __attribute__((packed)) {
    uint8_t function;
    uint8_t hash[16];
    size_t file_size;
} DfsRequest;

#endif
