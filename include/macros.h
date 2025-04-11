#ifndef DFS_MACROS_HEADER
#define DFS_MACROS_HEADER

#define Fatal(expr, perror_str)                                                                                        \
    if ((expr)) {                                                                                                      \
        if (perror_str != NULL) {                                                                                      \
            perror(perror_str);                                                                                        \
        } else {                                                                                                       \
            fprintf(stderr, "fatal error\n");                                                                          \
        }                                                                                                              \
        fflush(stdout);                                                                                                \
        fflush(stderr);                                                                                                \
        exit(EXIT_FAILURE);                                                                                            \
    }
#endif
