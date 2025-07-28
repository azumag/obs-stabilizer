#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

// Basic memory allocation functions for OBS compatibility
#define bmalloc(size) malloc(size)
#define bcalloc(count, size) calloc(count, size)
#define brealloc(ptr, size) realloc(ptr, size)
#define bfree(ptr) free(ptr)

#ifdef __cplusplus
}
#endif