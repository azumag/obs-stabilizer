#pragma once

#include <cstdarg>
#include <cstdio>
#include <iostream>

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
