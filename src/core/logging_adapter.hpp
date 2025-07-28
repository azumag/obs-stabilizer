/*
OBS Stabilizer Plugin - Logging Adapter for Standalone/OBS Builds
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#ifdef HAVE_OBS_HEADERS
    // OBS plugin mode - use obs_log
    #include <obs-module.h>
    #define STABILIZER_LOG_INFO(fmt, ...) obs_log(LOG_INFO, fmt, ##__VA_ARGS__)
    #define STABILIZER_LOG_WARNING(fmt, ...) obs_log(LOG_WARNING, fmt, ##__VA_ARGS__)
    #define STABILIZER_LOG_ERROR(fmt, ...) obs_log(LOG_ERROR, fmt, ##__VA_ARGS__)
    #define STABILIZER_LOG_DEBUG(fmt, ...) obs_log(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
    // Standalone mode - use printf/fprintf
    #include <cstdio>
    #include <cstdarg>

    namespace obs_stabilizer {
        inline void stabilizer_log(const char* level, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            printf("[%s] OBS-Stabilizer: ", level);
            vprintf(fmt, args);
            printf("\n");
            va_end(args);
        }
    }

    #define STABILIZER_LOG_INFO(fmt, ...) obs_stabilizer::stabilizer_log("INFO", fmt, ##__VA_ARGS__)
    #define STABILIZER_LOG_WARNING(fmt, ...) obs_stabilizer::stabilizer_log("WARNING", fmt, ##__VA_ARGS__)
    #define STABILIZER_LOG_ERROR(fmt, ...) obs_stabilizer::stabilizer_log("ERROR", fmt, ##__VA_ARGS__)
    #define STABILIZER_LOG_DEBUG(fmt, ...) obs_stabilizer::stabilizer_log("DEBUG", fmt, ##__VA_ARGS__)
#endif