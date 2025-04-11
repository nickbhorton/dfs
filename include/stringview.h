#ifndef DFS_STRINGVIEW_HEADER
#define DFS_STRINGVIEW_HEADER

#include <stddef.h>

typedef struct {
    const char* data;
    size_t length;
} StringView;

#endif
