/*
 * OBS Stabilizer - Platform-specific optimizations implementation
 *
 * This file provides platform-specific optimization utilities for the stabilizer.
 * The actual SIMD optimization work is handled by OpenCV's optimized code paths,
 * which are enabled via cv::useOptimized(true) in StabilizerCore::initialize().
 * This module focuses on platform detection and optimization status reporting.
 *
 * Note: Inline functions in platform_optimization.hpp handle basic platform detection.
 * This file provides additional utility functions that require non-trivial implementation.
 */

#include "platform_optimization.hpp"
#include "logging.hpp"
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <sys/sysctl.h>
#endif

#ifdef __linux__
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

namespace PlatformOptimization {

/**
 * Get the number of CPU cores available
 * This helps users understand their hardware capabilities and can be used
 * for optimizing performance parameters like feature count or smoothing radius
 */
int get_cpu_core_count() {
#if defined(_WIN32)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return static_cast<int>(sysinfo.dwNumberOfProcessors);
#elif defined(__APPLE__)
    int num_cores;
    size_t len = sizeof(num_cores);
    sysctlbyname("hw.physicalcpu", &num_cores, &len, NULL, 0);
    return num_cores;
#elif defined(__linux__)
    return sysconf(_SC_NPROCESSORS_ONLN);
#else
    return 1;  // Default to 1 if unknown
#endif
}

/**
 * Get system memory information in bytes
 * Useful for estimating available memory and adjusting buffer sizes
 */
size_t get_system_memory_size() {
#if defined(_WIN32)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return static_cast<size_t>(status.ullTotalPhys);
#elif defined(__APPLE__)
    int mib[2];
    int64_t physical_memory;
    size_t length;

    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    length = sizeof(int64_t);
    sysctl(mib, 2, &physical_memory, &length, NULL, 0);

    return static_cast<size_t>(physical_memory);
#elif defined(__linux__)
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return static_cast<size_t>(info.totalram) * static_cast<size_t>(info.mem_unit);
    }
    return 0;
#else
    return 0;  // Unknown
#endif
}

/**
 * Get platform name as human-readable string
 * Useful for diagnostics and user feedback
 */
std::string get_platform_name() {
    PlatformType type = get_platform_type();
    switch (type) {
        case PlatformType::AppleSilicon:
            return "Apple Silicon (ARM64)";
        case PlatformType::Windows:
            return "Windows (x86_64)";
        case PlatformType::Linux:
            return "Linux (x86_64)";
        case PlatformType::Generic:
        default:
            return "Generic (Unknown)";
    }
}

/**
 * Get SIMD capabilities as human-readable string
 * Lists all available SIMD instruction sets for the current platform
 * This helps users understand what optimizations are active
 */
std::string get_simd_capabilities() {
    std::ostringstream oss;

    bool first = true;
    if (has_neon()) {
        oss << (first ? "" : ", ") << "NEON";
        first = false;
    }
    if (has_sse4_2()) {
        oss << (first ? "" : ", ") << "SSE4.2";
        first = false;
    }
    if (has_avx2()) {
        oss << (first ? "" : ", ") << "AVX2";
        first = false;
    }

    if (first) {
        oss << "None";
    }

    return oss.str();
}

/**
 * Print platform information to log
 * Provides diagnostic information about the current platform and optimizations
 * Useful for troubleshooting and performance tuning
 */
void print_platform_info() {
    CORE_LOG_INFO("Platform: %s", get_platform_name().c_str());
    CORE_LOG_INFO("CPU Cores: %d", get_cpu_core_count());
    CORE_LOG_INFO("System Memory: %.2f GB",
                  static_cast<double>(get_system_memory_size()) / (1024.0 * 1024.0 * 1024.0));
    CORE_LOG_INFO("SIMD Capabilities: %s", get_simd_capabilities().c_str());
    CORE_LOG_INFO("SIMD Alignment: %d bytes", get_simd_alignment());
}

/**
 * Check if running on Apple Silicon
 * Convenience function for platform-specific logic
 */
bool is_apple_silicon() {
    return get_platform_type() == PlatformType::AppleSilicon;
}

/**
 * Check if running on Windows
 * Convenience function for platform-specific logic
 */
bool is_windows() {
    return get_platform_type() == PlatformType::Windows;
}

/**
 * Check if running on Linux
 * Convenience function for platform-specific logic
 */
bool is_linux() {
    return get_platform_type() == PlatformType::Linux;
}

} // namespace PlatformOptimization
