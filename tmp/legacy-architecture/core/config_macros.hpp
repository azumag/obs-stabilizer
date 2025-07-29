/*
OBS Stabilizer Plugin - Conditional Compilation Configuration
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include "logging_adapter.hpp"
#include <type_traits>

// Central configuration for conditional compilation
// This header unifies all ENABLE_STABILIZATION checks and provides
// consistent feature detection across the entire codebase

// Primary feature flag (defined by CMake)
#ifdef ENABLE_STABILIZATION
    #define STABILIZER_OPENCV_AVAILABLE 1
    #define STABILIZER_FULL_FEATURES 1
#else
    #define STABILIZER_OPENCV_AVAILABLE 0
    #define STABILIZER_FULL_FEATURES 0
#endif

// Feature-specific macros for cleaner conditional compilation
#define STABILIZER_IF_OPENCV_AVAILABLE(...) \
    do { \
        if constexpr (STABILIZER_OPENCV_AVAILABLE) { \
            __VA_ARGS__ \
        } \
    } while(0)

#define STABILIZER_IF_FEATURES_AVAILABLE(...) \
    do { \
        if constexpr (STABILIZER_FULL_FEATURES) { \
            __VA_ARGS__ \
        } \
    } while(0)

// Simplified conditional compilation helpers without complex templates
struct OpenCVGuard {
    template<typename Func, typename DefaultValue>
    static auto execute_or([[maybe_unused]] Func&& func, [[maybe_unused]] DefaultValue&& default_val) {
        #if STABILIZER_OPENCV_AVAILABLE
            return func();
        #else
            return default_val;
        #endif
    }
};

struct FeatureGuard {
    template<typename Func, typename DefaultValue>
    static auto execute_or([[maybe_unused]] Func&& func, [[maybe_unused]] DefaultValue&& default_val) {
        #if STABILIZER_FULL_FEATURES
            return func();
        #else
            return default_val;
        #endif
    }
};

// Conditional include macros with cross-platform warning suppression
#if STABILIZER_OPENCV_AVAILABLE
    // Cross-platform warning suppression
    #ifdef _MSC_VER
        #define STABILIZER_OPENCV_HEADERS \
            __pragma(warning(push)) \
            __pragma(warning(disable: 4100)) /* unused parameter */ \
            __pragma(warning(disable: 4996)) /* deprecated */ \
            __pragma(warning(disable: 4244)) /* conversion */ \
            __pragma(warning(disable: 4267)) /* size_t conversion */ \
            __INCLUDE_LEVEL__ \
            __pragma(warning(pop))
    #elif defined(__GNUC__) || defined(__clang__)
        #define STABILIZER_OPENCV_HEADERS \
            _Pragma("GCC diagnostic push") \
            _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
            _Pragma("GCC diagnostic ignored \"-Wdeprecated-enum-enum-conversion\"") \
            _Pragma("GCC diagnostic ignored \"-Wdeprecated-anon-enum-enum-conversion\"") \
            _Pragma("GCC diagnostic ignored \"-Wconversion\"") \
            __INCLUDE_LEVEL__ \
            _Pragma("GCC diagnostic pop")
    #else
        #define STABILIZER_OPENCV_HEADERS __INCLUDE_LEVEL__
    #endif

    // Actual includes (separate from warning suppression for clarity)
    #define STABILIZER_INCLUDE_OPENCV_HEADERS() \
        do { \
            STABILIZER_OPENCV_HEADERS; \
        } while(0)
#else
    #define STABILIZER_OPENCV_HEADERS
    #define STABILIZER_INCLUDE_OPENCV_HEADERS()
#endif

// Function decoration macros
#if STABILIZER_OPENCV_AVAILABLE
    #define STABILIZER_OPENCV_FUNCTION
    #define STABILIZER_STUB_FUNCTION [[maybe_unused]]
#else
    #define STABILIZER_OPENCV_FUNCTION [[maybe_unused]]
    #define STABILIZER_STUB_FUNCTION
#endif

// Declaration macros for conditional API
#define STABILIZER_DECLARE_OPENCV_METHOD(return_type, method_name, ...) \
    STABILIZER_OPENCV_FUNCTION return_type method_name(__VA_ARGS__)

#define STABILIZER_DECLARE_STUB_METHOD(return_type, method_name, ...) \
    STABILIZER_STUB_FUNCTION return_type method_name(__VA_ARGS__)

// Implementation helper macros
#define STABILIZER_OPENCV_IMPL_BEGIN \
    if constexpr (STABILIZER_OPENCV_AVAILABLE) {

#define STABILIZER_OPENCV_IMPL_END \
    }

#define STABILIZER_STUB_IMPL_BEGIN \
    if constexpr (!STABILIZER_OPENCV_AVAILABLE) {

#define STABILIZER_STUB_IMPL_END \
    }

// Logging helpers with feature context
#define STABILIZER_LOG_FEATURE_STATUS() \
    do { \
        if constexpr (STABILIZER_OPENCV_AVAILABLE) { \
            STABILIZER_LOG_INFO( "OBS Stabilizer: OpenCV features enabled"); \
        } else { \
            STABILIZER_LOG_INFO( "OBS Stabilizer: Running in stub mode (OpenCV unavailable)"); \
        } \
    } while(0)

// Parameter validation helpers
#define STABILIZER_VALIDATE_OPENCV_AVAILABLE(return_value) \
    do { \
        if constexpr (!STABILIZER_OPENCV_AVAILABLE) { \
            STABILIZER_LOG_WARNING( "OpenCV feature requested but not available"); \
            return return_value; \
        } \
    } while(0)

// Unused parameter suppression for stub implementations
#define STABILIZER_UNUSED_WHEN_STUB(...) \
    do { \
        if constexpr (!STABILIZER_OPENCV_AVAILABLE) { \
            (void)(__VA_ARGS__); \
        } \
    } while(0)

// Version and capability information
namespace obs_stabilizer {
namespace config {
    constexpr bool has_opencv = STABILIZER_OPENCV_AVAILABLE;
    constexpr bool has_full_features = STABILIZER_FULL_FEATURES;

    // Runtime capability check
    inline bool is_opencv_available() {
        return has_opencv;
    }

    inline bool are_features_available() {
        return has_full_features;
    }

    // Feature description
    inline const char* get_feature_description() {
        if constexpr (has_opencv) {
            return "Full OpenCV-based stabilization";
        } else {
            return "Stub mode - no stabilization features";
        }
    }
}
} // namespace obs_stabilizer

