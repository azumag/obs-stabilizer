/*
OBS Stabilizer Plugin - Unified Error Handling Implementation
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "error_handler.hpp"
#include "logging_adapter.hpp"

namespace obs_stabilizer {

#ifdef ENABLE_STABILIZATION
void ErrorHandler::handle_opencv_error(const cv::Exception& e, ErrorCategory category,
                                      const char* operation_name) {
    const char* category_name = get_category_name(category);
    STABILIZER_LOG_ERROR( "[%s] OpenCV error in %s: %s (code: %d, file: %s, line: %d)",
            category_name, operation_name, e.what(), e.code, e.file.c_str(), e.line);
}
#endif

void ErrorHandler::handle_standard_error(const std::exception& e, ErrorCategory category,
                                        const char* operation_name) {
    const char* category_name = get_category_name(category);
    STABILIZER_LOG_ERROR( "[%s] Standard error in %s: %s",
            category_name, operation_name, e.what());
}

void ErrorHandler::log_stub_mode_warning(const char* operation_name) {
    STABILIZER_LOG_INFO( "Stub mode: %s skipped (OpenCV not available)", operation_name);
}

void ErrorHandler::log_critical_error(ErrorCategory category, const char* operation_name,
                                     const char* details) {
    const char* category_name = get_category_name(category);
    if (details) {
        STABILIZER_LOG_ERROR( "[%s] CRITICAL: %s - %s", category_name, operation_name, details);
    } else {
        STABILIZER_LOG_ERROR( "[%s] CRITICAL: %s", category_name, operation_name);
    }
}

void ErrorHandler::log_error(ErrorCategory category, const char* operation_name,
                            const char* details) {
    const char* category_name = get_category_name(category);
    if (details) {
        STABILIZER_LOG_ERROR( "[%s] ERROR: %s - %s", category_name, operation_name, details);
    } else {
        STABILIZER_LOG_ERROR( "[%s] ERROR: %s", category_name, operation_name);
    }
}

void ErrorHandler::log_warning(ErrorCategory category, const char* operation_name,
                              const char* details) {
    const char* category_name = get_category_name(category);
    if (details) {
        STABILIZER_LOG_WARNING( "[%s] %s - %s", category_name, operation_name, details);
    } else {
        STABILIZER_LOG_WARNING( "[%s] %s", category_name, operation_name);
    }
}

const char* ErrorHandler::get_category_name(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::INITIALIZATION:
            return "INIT";
        case ErrorCategory::FRAME_PROCESSING:
            return "FRAME";
        case ErrorCategory::FEATURE_DETECTION:
            return "DETECT";
        case ErrorCategory::FEATURE_TRACKING:
            return "TRACK";
        case ErrorCategory::TRANSFORM_CALCULATION:
            return "TRANSFORM";
        case ErrorCategory::MEMORY_ALLOCATION:
            return "MEMORY";
        case ErrorCategory::CONFIGURATION:
            return "CONFIG";
        case ErrorCategory::OPENCV_INTERNAL:
            return "OPENCV";
        default:
            return "UNKNOWN";
    }
}

} // namespace obs_stabilizer