#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

#include "filestuff.h"
#include "protocol.h"
#include "stringview.h"
#include "tcp.h"

// function does nto return unless args are valid
int validate_input(int argc, char** argv);

// returns number of heap sock_fd connections in connections_o
int read_conf(int** connections_o);

void dfc_put(int scc, int* scv, int bfnc, char** bfnv);
void dfc_ls(int scc, int* scv);

int main(int argc, char** argv)
{
    int function = validate_input(argc, argv);
    int* connections = NULL;
    int connection_count = 0;
    if ((connection_count = read_conf(&connections)) <= 0) {
        printf("read_conf\n");
        exit(1);
    }
    switch (function) {
    case REQUEST_LS:
        dfc_ls(connection_count, connections);
        break;
    case REQUEST_PUT:
        dfc_put(connection_count, connections, argc - 2, &argv[2]);
        break;
    default:
        break;
    }

    // shutdown all connections
    // shudown_connections:
    for (int i = 0; i < connection_count; i++) {
        // i < connection_count - 1 ? printf("%d/", connections[i]) : printf("%d", connections[i]);
        if (connections[i] > 0) {
            shutdown(connections[i], SHUT_RDWR);
        }
    }
    // printf("\n");
    free(connections);
}

int bfn_to_fn(const char* bfn, char* fn_o, size_t fnsz, int chunk_num, int chunk_count, int server_idx)
{
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    char const* path_removed = strrchr(bfn, '/');
    char const* fn_no_path = path_removed ? path_removed + 1 : bfn;
    int rv = snprintf(
        fn_o,
        fnsz,
        "%s.%ld.%09ld.%d.%d.%d",
        fn_no_path,
        current_time.tv_sec,
        current_time.tv_nsec,
        chunk_num,
        chunk_count,
        server_idx
    );
    return rv;
}

ssize_t send_chunk(
    int sock_fd,
    int file_fd,
    char const* bfn,
    ssize_t file_size,
    int chunk_number,
    int chunk_count,
    int server_idx
)
{
    static char fn[256];
    int rv = bfn_to_fn(bfn, fn, 256, chunk_number, chunk_count, server_idx);
    if (rv < 0) {
        return -1;
    }
    size_t offset;
    size_t chunk_size;
    if (chunk_number < chunk_count) {
        offset = (file_size / chunk_count) * (chunk_number - 1);
        chunk_size = file_size / chunk_count;
    } else {
        offset = (file_size / chunk_count) * (chunk_number - 1);
        chunk_size = file_size / chunk_count + file_size % chunk_count;
    }
    /*
    printf(
        "%s -> %s    fn_sz: %lu    cfile_sz: %zd    offset: %zd    chunk_size: %zd\n",
        bfn,
        fn,
        strlen(fn),
        file_size,
        offset,
        chunk_size
    );
    */

    // send request
    StringView fn_sv = {.data = fn, .length = rv};
    DfsRequest request = {.function = REQUEST_PUT, .file_size = chunk_size, .file_name = fn_sv};
    ssize_t request_bytes_sent = send_request(sock_fd, &request);
    if (request_bytes_sent < 0) {
        return -2;
    }
    ssize_t file_bytes_sent = sendfile(sock_fd, file_fd, (off_t*)&offset, chunk_size);
    if (file_bytes_sent < chunk_size) {
        return -2;
    }
    return request_bytes_sent + file_bytes_sent;
}

// bfn -> base file name
// sc  -> socket connection
void dfc_put(int scc, int* scv, int bfnc, char** bfnv)
{
    for (int i = 0; i < bfnc; i++) {
        ssize_t cfile_sz = get_file_size(bfnv[i]);
        if (cfile_sz <= 0) {
            continue;
        }
        FILE* cfile = fopen(bfnv[i], "r");
        if (cfile == NULL) {
            continue;
        }

        // determine x
        uint8_t bfn_hash[16];
        StringView bfn_sv = {.data = bfnv[i], .length = strlen(bfnv[i])};
        hash_file_name(bfn_sv, bfn_hash);
        int x = bfn_hash[15] % scc;

        int rv;
        for (int n = 0; n < scc; n++) {
            if (scv[n] < 0) {
                continue;
            }
            int no = n + x;
            int fst_chunk_idx = (no % scc) + 1;
            int snd_chunk_idx = ((no + 1) % scc) + 1;
            rv = send_chunk(scv[n], fileno(cfile), bfnv[i], cfile_sz, fst_chunk_idx, scc, n);
            if (rv < 0) {
                printf("%d/%d for server %d failed\n", fst_chunk_idx, scc, n + 1);
            }
            rv = send_chunk(scv[n], fileno(cfile), bfnv[i], cfile_sz, snd_chunk_idx, scc, n);
            if (rv < 0) {
                printf("%d/%d for server %d failed\n", snd_chunk_idx, scc, n + 1);
            }
        }
        fclose(cfile);
    }
}

int get_chunk_info(
    char* fn,
    char* base_file_name_o,
    struct timespec* put_time,
    int* chunk_number,
    int* chunk_count,
    int* server_number
)
{
    char* tokens[32];
    size_t token_idx = 0;
    char* ptr = strtok(fn, ".");
    while (ptr != NULL) {
        tokens[token_idx] = ptr;
        token_idx++;
        ptr = strtok(NULL, ".");
    }
    put_time->tv_sec = atoi(tokens[token_idx - 5]);
    put_time->tv_nsec = atoi(tokens[token_idx - 4]);
    *chunk_number = atoi(tokens[token_idx - 3]);
    *chunk_count = atoi(tokens[token_idx - 2]);
    *server_number = atoi(tokens[token_idx - 1]);

    size_t base_file_name_idx = 0;
    for (int i = 0; i < (int)token_idx - 5; i++) {
        memcpy(base_file_name_o + base_file_name_idx, tokens[i], strlen(tokens[i]));
        base_file_name_idx += strlen(tokens[i]);
        if (i != (int)token_idx - 6) {
            memcpy(base_file_name_o + base_file_name_idx, ".", 1);
            base_file_name_idx += 1;
        }
    }
    return 0;
}
int chunk_sort(void const* a, void const* b)
{
    char const* str1 = *(char const**)a;
    char const* str2 = *(char const**)b;
    struct timespec put_time1;
    struct timespec put_time2;
    int chunk_number1;
    int chunk_number2;
    int chunk_count1;
    int chunk_count2;
    int server_number1;
    int server_number2;
    char base_file_name1[256];
    char base_file_name2[256];

    char buffer[256];

    snprintf(buffer, 256, "%s", str1);
    get_chunk_info(buffer, base_file_name1, &put_time1, &chunk_number1, &chunk_count1, &server_number1);
    /*
    printf(
        "%s %ld %ld %d %d %d\n",
        base_file_name1,
        put_time1.tv_sec,
        put_time1.tv_nsec,
        chunk_number1,
        chunk_count1,
        server_number1
    );
    */

    snprintf(buffer, 256, "%s", str2);
    get_chunk_info(buffer, base_file_name2, &put_time2, &chunk_number2, &chunk_count2, &server_number2);
    /*
    printf(
        "%s %ld %ld %d %d %d\n",
        base_file_name2,
        put_time2.tv_sec,
        put_time2.tv_nsec,
        chunk_number2,
        chunk_count2,
        server_number2
    );
    */
    int rv = strcmp(base_file_name1, base_file_name2);
    if (rv != 0) {
        return rv;
    }
    return chunk_number1 > chunk_number2;
};

void dfc_ls(int scc, int* scv)
{
    char* file_array[1024];
    size_t file_count = 0;
    for (int n = 0; n < scc; n++) {
        if (scv[n] < 0) {
            continue;
        }

        DfsRequest request = {.function = REQUEST_LS};
        ssize_t request_bytes_sent = send_request(scv[n], &request);
        if (request_bytes_sent < 0) {
            printf("send_request failed for dfc_ls\n");
            continue;
        }
        ssize_t response_size = -1;
        int response_size_bytes = tcp_recv(scv[n], (char*)&response_size, sizeof(response_size));
        if (response_size_bytes != sizeof(response_size)) {
            printf("tcp_recv failed for dfc_ls %d\n", response_size_bytes);
            continue;
        }
        if (response_size > 0) {
            char* response = malloc(response_size + 1);
            response[response_size] = '\0';
            response_size_bytes = tcp_recv(scv[n], response, response_size);
            if (response_size_bytes != response_size) {
                printf("tcp_recv failed for dfc_ls %d\n", response_size_bytes);
                free(response);
                continue;
            }

            // tokenize response on \n
            char* file_ptr = strtok(response, "\n");
            while (file_ptr != NULL) {
                file_array[file_count] = malloc(strlen(file_ptr) + 1);
                memcpy(file_array[file_count], file_ptr, strlen(file_ptr));
                file_array[file_count][strlen(file_ptr)] = '\0';
                file_count++;
                file_ptr = strtok(NULL, "\n");
            }
            free(response);
        }
    }
    qsort(file_array, file_count, sizeof(file_array[0]), chunk_sort);
    for (size_t i = 0; i < file_count; i++) {
        printf("%s\n", file_array[i]);
        free(file_array[i]);
    }
}

int read_conf(int** connections_o)
{
    FILE* conf_file = fopen("dfc.conf", "r");
    if (conf_file == NULL) {
        return -1;
    }

    char line[512];
    int line_count = 0;
    while (fgets(line, sizeof(line), conf_file)) {
        line_count++;
    }

    rewind(conf_file);
    int* result = malloc(sizeof(int) * line_count);

    int i = 0;
    while (fgets(line, sizeof(line), conf_file)) {
        char* server_w = strtok(line, " ");
        if (server_w == NULL || strcmp(server_w, "server")) {
            fclose(conf_file);
            free(result);
            return -1;
        }

        char* dir = strtok(NULL, " ");
        if (dir == NULL) {
            fclose(conf_file);
            free(result);
            return -1;
        }
        // printf("dir: %s\n", dir);

        char* address = strtok(NULL, " ");
        if (address == NULL) {
            fclose(conf_file);
            free(result);
            return -1;
        }

        char* address_ip = strtok(address, ":");
        if (address_ip == NULL) {
            fclose(conf_file);
            free(result);
            return -1;
        }
        // printf("ip: %s\n", address_ip);

        char* address_port_str = strtok(NULL, ":");
        if (address_port_str == NULL) {
            fclose(conf_file);
            free(result);
            return -1;
        }
        int address_port = atoi(address_port_str);
        // printf("port: %d\n", address_port);

        // connect to the server
        StringView node = {.data = address_ip, .length = strlen(address_ip)};
        result[i] = tcp_connect(node, address_port);
        i++;
    }

    fclose(conf_file);

    *connections_o = result;
    return line_count;
}

void useage()
{
    printf("dfc <command> [filename] ... [filename]\n");
    exit(1);
}

void useage_get()
{
    printf("dfc get [filename] ... [filename]\n");
    exit(1);
}

void useage_put()
{
    printf("dfc put [filename] ... [filename]\n");
    exit(1);
}

void useage_cmd()
{
    printf("dfc <get|put|ls> [filename] ... [filename]\n");
    exit(1);
}

// taken from stack overflow (im lazy)
int strcicmp(char const* str1, char const* str2)
{
    for (;; str1++, str2++) {
        int diff = tolower((unsigned char)*str1) - tolower((unsigned char)*str2);
        if (diff != 0 || !*str1)
            return diff;
    }
}

int validate_input(int argc, char** argv)
{
    if (argc < 2) {
        useage();
    }
    if (strcicmp(argv[1], "get") == 0) {
        if (argc < 3) {
            useage_get();
        }
        return REQUEST_GET;
    } else if (strcicmp(argv[1], "put") == 0) {
        if (argc < 3) {
            useage_put();
        }
        return REQUEST_PUT;
    } else if (strcicmp(argv[1], "ls") == 0) {
        return REQUEST_LS;
    }
    useage_cmd();
    return 0;
}
