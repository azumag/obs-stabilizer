/******************************************************************************
    Minimal Memory Management Definitions for OBS Plugin Development
    This file provides memory management definitions for standalone compilation
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>

/* Use standard C memory functions */
#define bmalloc(size) malloc(size)
#define bzalloc(size) calloc(1, size)
#define bfree(ptr) free(ptr)
#define brealloc(ptr, size) realloc(ptr, size)

#define bstrdup(str) strdup(str)
#define bstrdup_n(str, len) strndup(str, len)

#ifdef __cplusplus
}
#endif
