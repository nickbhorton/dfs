#ifndef DFS_FILESTUFF_HEADER
#define DFS_FILESTUFF_HEADER

#include <stdint.h>
#include <stdio.h>

#include "stringview.h"

ssize_t get_file_size(StringView file_name);

void hash_file_name(StringView file_name, uint8_t hash_o[16]);
void hexify_hash(uint8_t hash[8], char hex_o[33]);

#endif
