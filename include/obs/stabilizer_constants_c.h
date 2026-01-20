/*
 * OBS Stabilizer Plugin - C-compatible Constants
 * Magic numbers for C code that can't use C++ templates
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Magic numbers replaced with named constants for maintainability
#define DATA_PLANES_COUNT 8              // Number of data planes in OBS frame
#define MEMORY_GROWTH_FACTOR 2           // Buffer shrink threshold multiplier

#ifdef __cplusplus
}
#endif