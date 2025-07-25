/*
Mock implementations for testing
Provides mock OBS functions and structures for unit testing
*/

#pragma once

#include <cstdio>
#include <cstdarg>

// Mock OBS logging levels
#define LOG_ERROR   100
#define LOG_WARNING 200  
#define LOG_INFO    300
#define LOG_DEBUG   400

// Mock OBS logging function
inline void obs_log(int log_level, const char* format, ...) {
    const char* level_str = "UNKNOWN";
    switch (log_level) {
        case LOG_ERROR:   level_str = "ERROR"; break;
        case LOG_WARNING: level_str = "WARN"; break;
        case LOG_INFO:    level_str = "INFO"; break;
        case LOG_DEBUG:   level_str = "DEBUG"; break;
    }
    
    printf("[%s] ", level_str);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}

// Mock video format definitions
#define VIDEO_FORMAT_NV12  1
#define VIDEO_FORMAT_I420  2

// Mock OBS frame structure
struct obs_source_frame {
    uint32_t width;
    uint32_t height;
    int format;
    uint8_t* data[4];
    uint32_t linesize[4];
};