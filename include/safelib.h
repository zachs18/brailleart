#include <stdlib.h>

void *smalloc(size_t size);
void *scalloc(size_t nmemb, size_t size);
void *srealloc(void *ptr, size_t size);
void *sreallocarray(void *ptr, size_t nmemb, size_t size);
void *s_mm_malloc(size_t size, size_t align);
