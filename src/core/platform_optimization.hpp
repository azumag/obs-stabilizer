#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>

#if defined(__APPLE__) && defined(__arm64__)
// Apple Silicon optimizations
#define PLATFORM_APPLE_SILICON 1
#define PLATFORM_HAS_NEON 1

#include <arm_neon.h>

// SIMD alignment for Apple Silicon
constexpr int SIMD_ALIGNMENT = 16;

#elif defined(_WIN32) || defined(_WIN64)
// Windows optimizations
#define PLATFORM_WINDOWS 1
#define PLATFORM_HAS_SSE4_2 1
#define PLATFORM_HAS_AVX2 1

#include <intrin.h>

// SIMD alignment for Windows
constexpr int SIMD_ALIGNMENT = 32;

#elif defined(__linux__)
// Linux optimizations
#define PLATFORM_LINUX 1
#define PLATFORM_HAS_SSE4_2 1

#include <x86intrin.h>

// SIMD alignment for Linux
constexpr int SIMD_ALIGNMENT = 32;

#else
// Generic platform - no SIMD optimizations
#define PLATFORM_GENERIC 1
#define PLATFORM_SIMD_ALIGNMENT 16

#endif

/**
 * @brief Platform detection utilities
 */
namespace PlatformOptimization {

enum class PlatformType {
    AppleSilicon,  // ARM64 with NEON
    Windows,       // x86_64 with SSE4.2/AVX2
    Linux,         // x86_64 with SSE4.2
    Generic        // No SIMD support
};

/**
 * @brief Get the current platform type
 */
inline PlatformType get_platform_type() {
#if defined(PLATFORM_APPLE_SILICON)
    return PlatformType::AppleSilicon;
#elif defined(PLATFORM_WINDOWS)
    return PlatformType::Windows;
#elif defined(PLATFORM_LINUX)
    return PlatformType::Linux;
#else
    return PlatformType::Generic;
#endif
}

/**
 * @brief Check if NEON SIMD is available
 */
inline bool has_neon() {
#if defined(PLATFORM_HAS_NEON)
    return true;
#else
    return false;
#endif
}

/**
 * @brief Check if SSE4.2 is available
 */
inline bool has_sse4_2() {
#if defined(PLATFORM_HAS_SSE4_2)
    return true;
#else
    return false;
#endif
}

/**
 * @brief Check if AVX2 is available
 */
inline bool has_avx2() {
#if defined(PLATFORM_HAS_AVX2)
    return true;
#else
    return false;
#endif
}

/**
 * @brief Get SIMD alignment requirement for the platform
 */
inline int get_simd_alignment() {
#if defined(SIMD_ALIGNMENT)
    return SIMD_ALIGNMENT;
#else
    return 16;
#endif
}

/**
 * @brief Get the number of CPU cores available
 * Helps users understand hardware capabilities for performance tuning
 */
int get_cpu_core_count();

/**
 * @brief Get system memory information in bytes
 * Useful for estimating available memory and adjusting buffer sizes
 */
size_t get_system_memory_size();

/**
 * @brief Get platform name as human-readable string
 * Useful for diagnostics and user feedback
 */
std::string get_platform_name();

/**
 * @brief Get SIMD capabilities as human-readable string
 * Lists all available SIMD instruction sets for the current platform
 */
std::string get_simd_capabilities();

/**
 * @brief Print platform information to log
 * Provides diagnostic information about current platform and optimizations
 */
void print_platform_info();

/**
 * @brief Check if running on Apple Silicon
 */
bool is_apple_silicon();

/**
 * @brief Check if running on Windows
 */
bool is_windows();

/**
 * @brief Check if running on Linux
 */
bool is_linux();

} // namespace PlatformOptimization
