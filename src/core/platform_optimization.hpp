#pragma once

/**
 * @brief Platform-specific optimizations header
 *
 * This header provides platform-specific optimization declarations and
 * configuration for the stabilizer. On unsupported platforms, it provides
 * stub implementations that fall back to generic OpenCV functions.
 */

#include <cstdint>
#include <cstdlib>

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
 * @brief Align a pointer to SIMD boundary
 */
inline void* align_simd(void* ptr) {
    uintptr_t aligned = reinterpret_cast<uintptr_t>(ptr);
    int alignment = get_simd_alignment();
    aligned = (aligned + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<void*>(aligned);
}

/**
 * @brief Allocate SIMD-aligned memory
 */
inline void* allocate_simd_aligned(size_t size) {
#if defined(_WIN32)
    return _aligned_malloc(size, get_simd_alignment());
#else
    void* ptr = nullptr;
    posix_memalign(&ptr, get_simd_alignment(), size);
    return ptr;
#endif
}

/**
 * @brief Free SIMD-aligned memory
 */
inline void free_simd_aligned(void* ptr) {
#if defined(_WIN32)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

} // namespace PlatformOptimization
