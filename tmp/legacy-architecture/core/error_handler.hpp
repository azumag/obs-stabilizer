/*
OBS Stabilizer Plugin - Unified Error Handling System
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include "config_macros.hpp"
#include <string>
#include <functional>

#ifdef HAVE_OBS_HEADERS
#include <obs-module.h>
#endif

#if STABILIZER_OPENCV_AVAILABLE
#include <opencv2/opencv.hpp>
#endif

namespace obs_stabilizer {

// Unified error categories for consistent handling
enum class ErrorCategory {
    INITIALIZATION,
    FRAME_PROCESSING,
    FEATURE_DETECTION,
    FEATURE_TRACKING,
    TRANSFORM_CALCULATION,
    MEMORY_ALLOCATION,
    CONFIGURATION,
    OPENCV_INTERNAL,
    CLEANUP,
    VALIDATION
};

// Centralized error handler for consistent logging and recovery
//
// THREAD SAFETY NOTES:
// - All static methods are thread-safe as they use OBS logging which is thread-safe
// - Error categorization and statistics are maintained per-thread via thread_local storage
// - The safe_execute_* template methods provide exception safety guarantees
class ErrorHandler {
public:
    // Execute function with OpenCV exception handling
    template<typename Func, typename ReturnType>
    static bool safe_execute_cv(Func&& func, ReturnType& result,
                               ErrorCategory category, const char* operation_name) noexcept {
        return OpenCVGuard::execute_or([func = std::forward<Func>(func), &result, category, operation_name]() -> bool {
            try {
                result = func();
                return true;
#if STABILIZER_OPENCV_AVAILABLE
            } catch (const cv::Exception& e) {
                handle_opencv_error(e, category, operation_name);
                return false;
#endif
            } catch (const std::exception& e) {
                handle_standard_error(e, category, operation_name);
                return false;
            } catch (...) {
                log_critical_error(category, operation_name, "Unknown exception caught");
                return false;
            }
        }, [operation_name]() -> bool {
            // Stub mode - always fail gracefully
            log_stub_mode_warning(operation_name);
            return false;
        }());
    }

    // Execute function with standard exception handling
    template<typename Func>
    static bool safe_execute(Func&& func, ErrorCategory category, const char* operation_name) noexcept {
        try {
            func();
            return true;
        } catch (const std::exception& e) {
            handle_standard_error(e, category, operation_name);
            return false;
        } catch (...) {
            log_critical_error(category, operation_name, "Unknown exception caught");
            return false;
        }
    }

    // Execute function returning boolean with unified error context
    template<typename Func>
    static bool safe_execute_bool(Func&& func, ErrorCategory category, const char* operation_name) noexcept {
        return OpenCVGuard::execute_or([func = std::forward<Func>(func), category, operation_name]() -> bool {
            try {
                return func();
#if STABILIZER_OPENCV_AVAILABLE
            } catch (const cv::Exception& e) {
                handle_opencv_error(e, category, operation_name);
                return false;
#endif
            } catch (const std::exception& e) {
                handle_standard_error(e, category, operation_name);
                return false;
            } catch (...) {
                log_critical_error(category, operation_name, "Unknown exception caught");
                return false;
            }
        }, [operation_name]() -> bool {
            // Stub mode - log and return false
            log_stub_mode_warning(operation_name);
            return false;
        }());
    }

    // Log critical errors with escalation
    static void log_critical_error(ErrorCategory category, const char* operation_name,
                                  const char* details = nullptr);

    // Log standard errors
    static void log_error(ErrorCategory category, const char* operation_name,
                         const char* details = nullptr);

    // Log warnings with context
    static void log_warning(ErrorCategory category, const char* operation_name,
                           const char* details = nullptr);

    // Get error category name for logging
    static const char* get_category_name(ErrorCategory category);

private:
#if STABILIZER_OPENCV_AVAILABLE
    static void handle_opencv_error(const cv::Exception& e, ErrorCategory category,
                                   const char* operation_name);
#endif
    static void handle_standard_error(const std::exception& e, ErrorCategory category,
                                     const char* operation_name);
    static void log_stub_mode_warning(const char* operation_name);
};

// Convenience macros for common error handling patterns
#define SAFE_CV_EXECUTE(func, result, category, op_name) \
    ErrorHandler::safe_execute_cv(func, result, category, op_name)

#define SAFE_EXECUTE(func, category, op_name) \
    ErrorHandler::safe_execute(func, category, op_name)

#define SAFE_BOOL_EXECUTE(func, category, op_name) \
    ErrorHandler::safe_execute_bool(func, category, op_name)

} // namespace obs_stabilizer