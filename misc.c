
#include <stdlib.h>

#include "misc.h"

void *nc_malloc(size_t n)
{
	void *res = malloc(n);
	if (res == NULL)
		exit(1);

	return res;
}

void *nc_calloc(size_t nmemb, size_t size)
{
	void *res = calloc(nmemb, size);

	if (res == NULL)
		exit(1);

	return res;
}

void *nc_realloc(void *ptr, size_t new_size)
{
	void *new_ptr = realloc(ptr, new_size);

	if (new_ptr == NULL)
		exit(1);

	return new_ptr;
}
