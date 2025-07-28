/*
OBS Stabilizer Plugin - Unified Parameter Validation Implementation
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "parameter_validator.hpp"
#include "stabilizer_constants.hpp"
#include <cmath>
#include <climits>

namespace obs_stabilizer {

ValidationResult ParameterValidator::validate_frame_basic(frame_t* frame) {
    if (!frame) {
        return ValidationResult(false, "Frame pointer is null");
    }
    
    if (!get_frame_data(frame, 0)) {
        return ValidationResult(false, "Frame data[0] is null");
    }
    
    if (get_frame_width(frame) == 0 || get_frame_height(frame) == 0) {
        return ValidationResult(false, "Frame dimensions are zero");
    }
    
    if (!is_valid_video_format(get_frame_format(frame))) {
        return ValidationResult(false, "Unsupported video format");
    }
    
    return ValidationResult(true);
}

ValidationResult ParameterValidator::validate_frame_dimensions(frame_t* frame) {
    auto basic_validation = validate_frame_basic(frame);
    if (!basic_validation) {
        return basic_validation;
    }
    
    // Validate frame dimensions to prevent integer overflow
    if (get_frame_width(frame) > StabilizerConstants::MAX_FRAME_WIDTH || 
        get_frame_height(frame) > StabilizerConstants::MAX_FRAME_HEIGHT) {
        return ValidationResult(false, "Frame dimensions too large");
    }
    
    // Check for potential integer overflow in size calculations
    if (check_integer_overflow(get_frame_width(frame), get_frame_height(frame))) {
        return ValidationResult(false, "Frame size would cause integer overflow");
    }
    
    // Validate minimum size for feature detection  
    if (get_frame_width(frame) < StabilizerConstants::MIN_FEATURE_DETECTION_SIZE || 
        get_frame_height(frame) < StabilizerConstants::MIN_FEATURE_DETECTION_SIZE) {
        return ValidationResult(false, "Frame too small for reliable feature detection");
    }
    
    return ValidationResult(true);
}

ValidationResult ParameterValidator::validate_frame_nv12(frame_t* frame) {
    auto basic_validation = validate_frame_dimensions(frame);
    if (!basic_validation) {
        return basic_validation;
    }
    
    if (get_frame_format(frame) != StabilizerConstants::VIDEO_FORMAT_NV12) {
        return ValidationResult(false, "Frame is not NV12 format");
    }
    
    // Validate Y plane
    if (get_frame_linesize(frame, 0) < get_frame_width(frame)) {
        return ValidationResult(false, "NV12 Y plane linesize too small");
    }
    
    // Validate UV plane (if present)
    if (get_frame_data(frame, 1)) {
        if (get_frame_linesize(frame, 1) < get_frame_width(frame)) {
            return ValidationResult(false, "NV12 UV plane linesize too small");
        }
    }
    
    return ValidationResult(true);
}

ValidationResult ParameterValidator::validate_frame_i420(frame_t* frame) {
    auto basic_validation = validate_frame_dimensions(frame);
    if (!basic_validation) {
        return basic_validation;
    }
    
    if (get_frame_format(frame) != StabilizerConstants::VIDEO_FORMAT_I420) {
        return ValidationResult(false, "Frame is not I420 format");
    }
    
    // Validate Y plane
    if (get_frame_linesize(frame, 0) < get_frame_width(frame)) {
        return ValidationResult(false, "I420 Y plane linesize too small");
    }
    
    // Validate U plane
    if (get_frame_data(frame, 1)) {
        if (get_frame_linesize(frame, 1) < get_frame_width(frame) / 2) {
            return ValidationResult(false, "I420 U plane linesize too small");
        }
    }
    
    // Validate V plane
    if (get_frame_data(frame, 2)) {
        if (get_frame_linesize(frame, 2) < get_frame_width(frame) / 2) {
            return ValidationResult(false, "I420 V plane linesize too small");
        }
    }
    
    return ValidationResult(true);
}

#ifdef ENABLE_STABILIZATION
ValidationResult ParameterValidator::validate_matrix_not_empty(const cv::Mat& mat, const char* matrix_name) {
    if (mat.empty()) {
        return ValidationResult(false, matrix_name ? matrix_name : "Matrix is empty");
    }
    return ValidationResult(true);
}

ValidationResult ParameterValidator::validate_matrix_size(const cv::Mat& mat, int min_rows, int min_cols, const char* matrix_name) {
    auto empty_check = validate_matrix_not_empty(mat, matrix_name);
    if (!empty_check) {
        return empty_check;
    }
    
    if (mat.rows < min_rows || mat.cols < min_cols) {
        return ValidationResult(false, matrix_name ? matrix_name : "Matrix size insufficient");
    }
    
    return ValidationResult(true);
}

ValidationResult ParameterValidator::validate_transform_matrix(const cv::Mat& transform) {
    auto size_check = validate_matrix_size(transform, 2, 3, "Transform matrix");
    if (!size_check) {
        return size_check;
    }
    
    // Validate transform values for reasonableness
    double dx = transform.at<double>(0, 2);
    double dy = transform.at<double>(1, 2);
    double a = transform.at<double>(0, 0);
    double b = transform.at<double>(0, 1);
    
    // Check for NaN/infinite values
    if (std::isnan(dx) || std::isinf(dx) || std::isnan(dy) || std::isinf(dy) ||
        std::isnan(a) || std::isinf(a) || std::isnan(b) || std::isinf(b)) {
        return ValidationResult(false, "Transform contains invalid values (NaN/Inf)");
    }
    
    // Calculate scale with protection against negative values
    double scale_squared = a * a + b * b;
    if (scale_squared < 0.0) {
        return ValidationResult(false, "Transform scale calculation error");
    }
    double scale = std::sqrt(scale_squared);
    
    if (std::abs(dx) > StabilizerConstants::MAX_TRANSLATION || 
        std::abs(dy) > StabilizerConstants::MAX_TRANSLATION) {
        return ValidationResult(false, "Transform translation values too large");
    }
    
    if (scale < StabilizerConstants::MIN_SCALE_FACTOR || 
        scale > StabilizerConstants::MAX_SCALE_FACTOR) {
        return ValidationResult(false, "Transform scale values unreasonable");
    }
    
    return ValidationResult(true);
}

ValidationResult ParameterValidator::validate_feature_points(const std::vector<cv::Point2f>& points, 
                                                           size_t min_count, const char* points_name) {
    if (points.size() < min_count) {
        return ValidationResult(false, points_name ? points_name : "Insufficient feature points");
    }
    
    return ValidationResult(true);
}
#endif

ValidationResult ParameterValidator::validate_smoothing_radius(int radius) {
    return validate_range_integer(radius, StabilizerConstants::MIN_SMOOTHING_RADIUS, 
                                 StabilizerConstants::MAX_SMOOTHING_RADIUS, "Smoothing radius");
}

ValidationResult ParameterValidator::validate_feature_count(int count) {
    return validate_range_integer(count, StabilizerConstants::MIN_FEATURES_REQUIRED, 
                                 StabilizerConstants::MAX_FEATURES_DEFAULT, "Feature count");
}

ValidationResult ParameterValidator::validate_threshold_value(double threshold, double min_val, double max_val, const char* param_name) {
    return validate_range_double(threshold, min_val, max_val, param_name);
}

ValidationResult ParameterValidator::validate_positive_integer(int value, const char* param_name) {
    if (value <= 0) {
        return ValidationResult(false, param_name ? param_name : "Value must be positive");
    }
    return ValidationResult(true);
}

ValidationResult ParameterValidator::validate_range_integer(int value, int min_val, int max_val, const char* param_name) {
    if (value < min_val || value > max_val) {
        return ValidationResult(false, param_name ? param_name : "Value out of range");
    }
    return ValidationResult(true);
}

ValidationResult ParameterValidator::validate_range_double(double value, double min_val, double max_val, const char* param_name) {
    if (value < min_val || value > max_val || std::isnan(value) || std::isinf(value)) {
        return ValidationResult(false, param_name ? param_name : "Value out of range or invalid");
    }
    return ValidationResult(true);
}

ValidationResult ParameterValidator::validate_pointer_not_null(const void* ptr, const char* ptr_name) {
    if (!ptr) {
        return ValidationResult(false, ptr_name ? ptr_name : "Pointer is null");
    }
    return ValidationResult(true);
}

ValidationResult ParameterValidator::validate_array_access(const void* array, size_t index, size_t max_size, const char* array_name) {
    auto null_check = validate_pointer_not_null(array, array_name);
    if (!null_check) {
        return null_check;
    }
    
    if (index >= max_size) {
        return ValidationResult(false, array_name ? array_name : "Array index out of bounds");
    }
    
    return ValidationResult(true);
}

ValidationResult ParameterValidator::validate_buffer_size(size_t actual_size, size_t required_size, const char* buffer_name) {
    if (actual_size < required_size) {
        return ValidationResult(false, buffer_name ? buffer_name : "Buffer too small");
    }
    return ValidationResult(true);
}

// Private helper methods
bool ParameterValidator::is_valid_video_format(uint32_t format) {
    return format == StabilizerConstants::VIDEO_FORMAT_NV12 || 
           format == StabilizerConstants::VIDEO_FORMAT_I420;
}

bool ParameterValidator::check_integer_overflow(uint32_t width, uint32_t height) {
    // Check for zero dimensions first (division by zero protection)
    if (width == 0 || height == 0) return true;
    
    // Check if width * height * 4 (for maximum bytes per pixel) would overflow
    if (width > UINT32_MAX / height) return true;
    if ((uint64_t)width * height > SIZE_MAX / 4) return true;
    return false;
}

const char* ParameterValidator::get_format_name(uint32_t format) {
    switch (format) {
        case StabilizerConstants::VIDEO_FORMAT_NV12: return "NV12";
        case StabilizerConstants::VIDEO_FORMAT_I420: return "I420";
        default: return "Unknown";
    }
}

} // namespace obs_stabilizer