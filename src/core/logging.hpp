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

// Log level enumeration for dynamic log filtering
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    NONE = 4
};

// Global log level variable (default: INFO for production use)
static inline LogLevel& get_global_log_level() {
    static LogLevel current_level = LogLevel::INFO;
    return current_level;
}

// Set global log level
static inline void set_log_level(LogLevel level) {
    get_global_log_level() = level;
}

// Get current global log level
static inline LogLevel get_log_level() {
    return get_global_log_level();
}

// Log function with level filtering
static inline void core_log_with_level(LogLevel level, const char* level_str, const char* fmt, ...) {
    // Skip log if current level is higher than message level
    if (level < get_global_log_level()) {
        return;
    }

    va_list args;
    va_start(args, fmt);

    // Output to stderr for ERROR and WARNING, stdout for INFO and DEBUG
    FILE* output = (level >= LogLevel::WARNING) ? stderr : stdout;
    fprintf(output, "[%s] ", level_str);
    vfprintf(output, fmt, args);
    fprintf(output, "\n");

    va_end(args);
}

static inline void core_log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    core_log_with_level(LogLevel::ERROR, "ERROR", fmt, args);
    va_end(args);
}

static inline void core_log_warning(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    core_log_with_level(LogLevel::WARNING, "WARNING", fmt, args);
    va_end(args);
}

static inline void core_log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    core_log_with_level(LogLevel::INFO, "INFO", fmt, args);
    va_end(args);
}

static inline void core_log_debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    core_log_with_level(LogLevel::DEBUG, "DEBUG", fmt, args);
    va_end(args);
}

#define CORE_LOG_ERROR(...) core_log_error(__VA_ARGS__)
#define CORE_LOG_WARNING(...) core_log_warning(__VA_ARGS__)
#define CORE_LOG_INFO(...) core_log_info(__VA_ARGS__)
#define CORE_LOG_DEBUG(...) core_log_debug(__VA_ARGS__)

#else

#include <obs-module.h>

// Log level enumeration for dynamic log filtering (OBS mode)
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    NONE = 4
};

// Global log level variable (default: INFO for production use)
static inline LogLevel& get_global_log_level() {
    static LogLevel current_level = LogLevel::INFO;
    return current_level;
}

// Set global log level
static inline void set_log_level(LogLevel level) {
    get_global_log_level() = level;
}

// Get current global log level
static inline LogLevel get_log_level() {
    return get_global_log_level();
}

// Map LogLevel to OBS log level
static inline int log_level_to_obs(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return LOG_DEBUG;
        case LogLevel::INFO: return LOG_INFO;
        case LogLevel::WARNING: return LOG_WARNING;
        case LogLevel::ERROR: return LOG_ERROR;
        case LogLevel::NONE: return LOG_ERROR;  // NONE falls back to ERROR
        default: return LOG_INFO;
    }
}

// Log function with level filtering for OBS mode
static inline void core_log_with_level_obs(LogLevel level, const char* fmt, va_list args) {
    // Skip log if current level is higher than message level
    if (level < get_global_log_level()) {
        return;
    }

    int obs_level = log_level_to_obs(level);
    blog(obs_level, "[obs-stabilizer] %s", fmt);
}

static inline void core_log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    core_log_with_level_obs(LogLevel::ERROR, fmt, args);
    va_end(args);
}

static inline void core_log_warning(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    core_log_with_level_obs(LogLevel::WARNING, fmt, args);
    va_end(args);
}

static inline void core_log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    core_log_with_level_obs(LogLevel::INFO, fmt, args);
    va_end(args);
}

static inline void core_log_debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    core_log_with_level_obs(LogLevel::DEBUG, fmt, args);
    va_end(args);
}

#define CORE_LOG_ERROR(...) core_log_error(__VA_ARGS__)
#define CORE_LOG_WARNING(...) core_log_warning(__VA_ARGS__)
#define CORE_LOG_INFO(...) core_log_info(__VA_ARGS__)
#define CORE_LOG_DEBUG(...) core_log_debug(__VA_ARGS__)

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

