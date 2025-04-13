#include "filestuff.h"

#include <string.h>
#include <sys/stat.h>

#include "md5.h"

static char file_name_buffer[4096];

void hash_file_name(StringView file_name, uint8_t hash[16])
{
    snprintf(file_name_buffer, 4096, "%.*s", (int)file_name.length, file_name.data);
    md5String(file_name_buffer, hash);
}

void hexify_hash(uint8_t hash[16], char* hex_o)
{
    snprintf(
        hex_o,
        33,
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        hash[0],
        hash[1],
        hash[2],
        hash[3],
        hash[4],
        hash[5],
        hash[6],
        hash[7],
        hash[8],
        hash[9],
        hash[10],
        hash[11],
        hash[12],
        hash[13],
        hash[14],
        hash[15]
    );
}

ssize_t get_file_size(const char* file_name)
{
    static struct stat st;
    int rv = stat(file_name, &st);
    if (rv < 0) {
        perror("stat");
        return -1;
    }
    return st.st_size;
}
