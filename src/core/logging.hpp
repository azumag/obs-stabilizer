#pragma once

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

// OpenCV exception support (only include when available)
// CMake find_package(OpenCV) ensures OpenCV headers are available for the build
#include <opencv2/core.hpp>

#ifdef BUILD_STANDALONE

static inline void core_log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::cerr << "[ERROR] ";
    vfprintf(stderr, fmt, args);
    std::cerr << std::endl;
    va_end(args);
}

static inline void core_log_warning(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::cerr << "[WARNING] ";
    vfprintf(stderr, fmt, args);
    std::cerr << std::endl;
    va_end(args);
}

static inline void core_log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::cout << "[INFO] ";
    vfprintf(stdout, fmt, args);
    std::cout << std::endl;
    va_end(args);
}

static inline void core_log_debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::cout << "[DEBUG] ";
    vfprintf(stdout, fmt, args);
    std::cout << std::endl;
    va_end(args);
}

#define CORE_LOG_ERROR(...) core_log_error(__VA_ARGS__)
#define CORE_LOG_WARNING(...) core_log_warning(__VA_ARGS__)
#define CORE_LOG_INFO(...) core_log_info(__VA_ARGS__)
#define CORE_LOG_DEBUG(...) core_log_debug(__VA_ARGS__)

#else

#include <obs-module.h>

#define CORE_LOG_ERROR(...) blog(LOG_ERROR, "[obs-stabilizer] " __VA_ARGS__)
#define CORE_LOG_WARNING(...) blog(LOG_WARNING, "[obs-stabilizer] " __VA_ARGS__)
#define CORE_LOG_INFO(...) blog(LOG_INFO, "[obs-stabilizer] " __VA_ARGS__)
#define CORE_LOG_DEBUG(...) blog(LOG_DEBUG, "[obs-stabilizer] " __VA_ARGS__)

#endif

// ============================================================================
// Exception Handling Helpers
// ============================================================================

namespace StabilizerLogging {

/**
 * Log exception details with consistent formatting
 * @param location Location where the exception occurred (e.g., function name)
 * @param e The exception that was caught
 */
inline void log_exception(const char* location, const std::exception& e) {
    CORE_LOG_ERROR("Exception in %s: %s", location, e.what());
}

/**
 * Log unknown exception details
 * @param location Location where the exception occurred (e.g., function name)
 */
inline void log_unknown_exception(const char* location) {
    CORE_LOG_ERROR("Unknown exception in %s", location);
}

/**
 * Log OpenCV exception with additional details
 * @param location Location where the exception occurred
 * @param e The OpenCV exception that was caught
 */
inline void log_opencv_exception(const char* location, const cv::Exception& e) {
    CORE_LOG_ERROR("OpenCV exception in %s: %s (code: %d, func: %s, line: %d)",
                   location, e.what(), e.code, e.func.c_str(), e.line);
}

/**
 * Helper to wrap a function call with exception handling and logging
 * @param func The function to call
 * @param location Location for logging
 * @param default_value The default value to return on exception
 * @return The result of the function call, or default value on exception
 *
 * Note: This function is compatible with C++17 (no auto template parameters).
 * The return type is deduced from decltype(func()) for flexibility.
 */
template<typename Func, typename T>
auto safe_call(Func func, const char* location, T default_value) -> decltype(func()) {
    try {
        return func();
    } catch (const cv::Exception& e) {
        log_opencv_exception(location, e);
        return default_value;
    } catch (const std::exception& e) {
        log_exception(location, e);
        return default_value;
    } catch (...) {
        log_unknown_exception(location);
        return default_value;
    }
}

} // namespace StabilizerLogging

