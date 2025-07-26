/*
OBS Stabilizer Plugin - Unified Parameter Validation System
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include <obs-module.h>
#include <cstdint>
#include <vector>

#ifdef ENABLE_STABILIZATION
#include <opencv2/opencv.hpp>
#endif

namespace obs_stabilizer {

// Validation result with detailed error information
struct ValidationResult {
    bool is_valid = false;
    const char* error_message = nullptr;
    
    ValidationResult() = default;
    ValidationResult(bool valid) : is_valid(valid) {}
    ValidationResult(bool valid, const char* msg) : is_valid(valid), error_message(msg) {}
    
    operator bool() const { return is_valid; }
};

// Centralized parameter validation for consistent bounds checking
class ParameterValidator {
public:
    // Frame validation with comprehensive checks
    static ValidationResult validate_frame_basic(struct obs_source_frame* frame);
    static ValidationResult validate_frame_dimensions(struct obs_source_frame* frame);
    static ValidationResult validate_frame_nv12(struct obs_source_frame* frame);
    static ValidationResult validate_frame_i420(struct obs_source_frame* frame);
    
    // OpenCV matrix validation
#ifdef ENABLE_STABILIZATION
    static ValidationResult validate_matrix_not_empty(const cv::Mat& mat, const char* matrix_name);
    static ValidationResult validate_matrix_size(const cv::Mat& mat, int min_rows, int min_cols, const char* matrix_name);
    static ValidationResult validate_transform_matrix(const cv::Mat& transform);
    static ValidationResult validate_feature_points(const std::vector<cv::Point2f>& points, size_t min_count, const char* points_name);
#endif
    
    // Configuration parameter validation
    static ValidationResult validate_smoothing_radius(int radius);
    static ValidationResult validate_feature_count(int count);
    static ValidationResult validate_threshold_value(double threshold, double min_val, double max_val, const char* param_name);
    
    // Numeric bounds validation
    static ValidationResult validate_positive_integer(int value, const char* param_name);
    static ValidationResult validate_range_integer(int value, int min_val, int max_val, const char* param_name);
    static ValidationResult validate_range_double(double value, double min_val, double max_val, const char* param_name);
    
    // Memory safety validation
    static ValidationResult validate_pointer_not_null(const void* ptr, const char* ptr_name);
    static ValidationResult validate_array_access(const void* array, size_t index, size_t max_size, const char* array_name);
    static ValidationResult validate_buffer_size(size_t actual_size, size_t required_size, const char* buffer_name);

private:
    // Internal validation helpers
    static bool is_valid_video_format(uint32_t format);
    static bool check_integer_overflow(uint32_t width, uint32_t height);
    static const char* get_format_name(uint32_t format);
};

// Convenience macros for common validation patterns
#define VALIDATE_AND_RETURN_IF_INVALID(validation, result_var) \
    do { \
        auto validation_result = (validation); \
        if (!validation_result) { \
            obs_log(LOG_ERROR, "Validation failed: %s", \
                    validation_result.error_message ? validation_result.error_message : "Unknown error"); \
            result_var.success = false; \
            return result_var; \
        } \
    } while(0)

#define VALIDATE_AND_RETURN_FALSE_IF_INVALID(validation) \
    do { \
        auto validation_result = (validation); \
        if (!validation_result) { \
            obs_log(LOG_ERROR, "Validation failed: %s", \
                    validation_result.error_message ? validation_result.error_message : "Unknown error"); \
            return false; \
        } \
    } while(0)

#define VALIDATE_AND_LOG_WARNING_IF_INVALID(validation) \
    do { \
        auto validation_result = (validation); \
        if (!validation_result) { \
            obs_log(LOG_WARNING, "Validation warning: %s", \
                    validation_result.error_message ? validation_result.error_message : "Unknown warning"); \
        } \
    } while(0)

} // namespace obs_stabilizer