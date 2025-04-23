#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    case REQUEST_PUT:
        dfc_put(connection_count, connections, argc - 2, &argv[2]);
    default:
        break;
    }

    // shutdown all connections
    // shudown_connections:
    for (int i = 0; i < connection_count; i++) {
        i < connection_count - 1 ? printf("%d/", connections[i]) : printf("%d", connections[i]);
        if (connections[i] > 0) {
            shutdown(connections[i], SHUT_RDWR);
        }
    }
    printf("\n");
    free(connections);
}

int bfn_to_fn(const char* bfn, char* fn_o, size_t fnsz, int chunk_num, int chunk_count)
{
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    char const* path_removed = strrchr(bfn, '/');
    char const* fn_no_path = path_removed ? path_removed + 1 : bfn;
    int rv = snprintf(
        fn_o,
        fnsz,
        "%s.%ld.%09ld.%d.%d",
        fn_no_path,
        current_time.tv_sec,
        current_time.tv_nsec,
        chunk_num,
        chunk_count
    );
    return rv;
}

// bfn -> base file name
// sc  -> socket connection
void dfc_put(int scc, int* scv, int bfnc, char** bfnv)
{
    for (int i = 0; i < bfnc; i++) {
        char fn[256];
        int rv = bfn_to_fn(bfnv[i], fn, 256, 1, 4);
        if (rv > 0) {
            printf("%s (%lu)\n", fn, strlen(fn));
        }
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
