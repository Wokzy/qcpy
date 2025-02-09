#pragma once

#define MEGABYTE 1024*1024
#define TRANSMIT_BUFF_SIZE 2*MEGABYTE

void *nc_malloc(size_t n);
void *nc_calloc(size_t nmemb, size_t size);
void *nc_realloc(void *ptr, size_t new_size);
