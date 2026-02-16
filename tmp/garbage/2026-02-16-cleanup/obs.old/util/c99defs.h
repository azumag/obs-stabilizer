/******************************************************************************
    Minimal C99 Definitions for OBS Plugin Development
    This file provides common C99 definitions for standalone compilation
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Define export attribute for shared libraries */
#if defined(_WIN32) || defined(_WIN64)
    #ifdef BUILDING_PLUGIN
        #define EXPORT __declspec(dllexport)
    #else
        #define EXPORT __declspec(dllimport)
    #endif
#else
    #if defined(__GNUC__) || defined(__clang__)
        #define EXPORT __attribute__((visibility("default")))
    #else
        #define EXPORT
    #endif
#endif

/* Common macros */
#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x) (void)(x)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/* Alignment macros */
#ifndef ALIGN
#ifdef _MSC_VER
#define ALIGN(x) __declspec(align(x))
#else
#define ALIGN(x) __attribute__((aligned(x)))
#endif
#endif

/* String macros */
#define STR(x) #x
#define STRINGIFY(x) STR(x)

#ifdef __cplusplus
}
#endif
