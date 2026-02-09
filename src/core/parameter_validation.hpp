#pragma once

#include "stabilizer_core.hpp"
#include "stabilizer_constants.hpp"
#include "frame_utils.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

/**
 * @brief Parameter validation utilities for the stabilizer
 *
 * This namespace provides validation and clamping functions for stabilizer
 * parameters to ensure they stay within safe and valid ranges.
 */
namespace VALIDATION {

/**
 * @brief Validate and clamp stabilizer parameters to safe ranges
 *
 * This function ensures all stabilizer parameters are within acceptable limits
 * as defined in stabilizer_constants.hpp. Clamping prevents invalid values
 * from causing runtime errors or undefined behavior.
 *
 * @param params Input parameters to validate
 * @return Validated and clamped parameters
 */
inline StabilizerCore::StabilizerParams validate_parameters(const StabilizerCore::StabilizerParams& params) {
    using namespace StabilizerConstants;

    StabilizerCore::StabilizerParams validated = params;

    // Validate smoothing radius (Issue #167: ensure reasonable limits)
    validated.smoothing_radius = std::clamp(validated.smoothing_radius,
                                           Smoothing::MIN_RADIUS,
                                           Smoothing::MAX_RADIUS);

    // Validate max correction percentage
    validated.max_correction = std::clamp(validated.max_correction,
                                          Correction::MIN_MAX,
                                          Correction::MAX_MAX);

    // Validate feature count
    validated.feature_count = std::clamp(validated.feature_count,
                                         Features::MIN_COUNT,
                                         Features::MAX_COUNT);

    // Validate quality level
    validated.quality_level = std::clamp(validated.quality_level,
                                         Quality::MIN_LEVEL,
                                         Quality::MAX_LEVEL);

    // Validate min distance
    validated.min_distance = std::clamp(validated.min_distance,
                                       Distance::MIN,
                                       Distance::MAX);

    // Validate block size (must be odd, within range)
    validated.block_size = std::clamp(validated.block_size,
                                     Block::MIN_SIZE,
                                     Block::MAX_SIZE);
    // Ensure block_size is odd as required by Shi-Tomasi corner detection
    if (validated.block_size % 2 == 0) {
        validated.block_size++;
    }

    // Validate Harris K parameter
    validated.k = std::clamp(validated.k,
                            Harris::MIN_K,
                            Harris::MAX_K);

    // Validate optical flow pyramid levels
    validated.optical_flow_pyramid_levels = std::clamp(validated.optical_flow_pyramid_levels,
                                                       OpticalFlow::MIN_PYRAMID_LEVELS,
                                                       OpticalFlow::MAX_PYRAMID_LEVELS);

    // Validate optical flow window size (must be odd, within range)
    validated.optical_flow_window_size = std::clamp(validated.optical_flow_window_size,
                                                    OpticalFlow::MIN_WINDOW_SIZE,
                                                    OpticalFlow::MAX_WINDOW_SIZE);
    // Ensure window_size is odd as required by Lucas-Kanade optical flow
    if (validated.optical_flow_window_size % 2 == 0) {
        validated.optical_flow_window_size++;
    }

    // Validate adaptive feature ranges
    validated.adaptive_feature_min = std::clamp(validated.adaptive_feature_min,
                                                AdaptiveFeatures::GAMING_MIN,
                                                AdaptiveFeatures::MAX_ADAPTIVE_FEATURES);
    validated.adaptive_feature_max = std::clamp(validated.adaptive_feature_max,
                                                AdaptiveFeatures::GAMING_MIN,
                                                AdaptiveFeatures::MAX_ADAPTIVE_FEATURES);

    // Ensure min <= max for adaptive features
    if (validated.adaptive_feature_min > validated.adaptive_feature_max) {
        std::swap(validated.adaptive_feature_min, validated.adaptive_feature_max);
    }

    // Validate feature refresh threshold
    validated.feature_refresh_threshold = std::clamp(validated.feature_refresh_threshold,
                                                     0.0f,
                                                     1.0f);

    // Validate high-pass filter attenuation
    validated.high_pass_attenuation = std::clamp(validated.high_pass_attenuation,
                                                0.0,
                                                1.0);

    // Validate tracking error threshold
    validated.tracking_error_threshold = std::clamp(validated.tracking_error_threshold,
                                                    0.0,
                                                    1000.0);

    // Validate RANSAC thresholds
    validated.ransac_threshold_min = std::clamp(validated.ransac_threshold_min,
                                               0.1f,
                                               100.0f);
    validated.ransac_threshold_max = std::clamp(validated.ransac_threshold_max,
                                               0.1f,
                                               100.0f);

    // Ensure RANSAC min <= max
    if (validated.ransac_threshold_min > validated.ransac_threshold_max) {
        std::swap(validated.ransac_threshold_min, validated.ransac_threshold_max);
    }

    // Validate min point spread
    validated.min_point_spread = std::clamp(validated.min_point_spread,
                                            0.0f,
                                            1000.0f);

    return validated;
}

/**
 * @brief Validate frame dimensions against constraints
 *
 * @param width Frame width
 * @param height Frame height
 * @return true if dimensions are valid, false otherwise
 */
inline bool validate_dimensions(uint32_t width, uint32_t height) {
    using namespace FRAME_UTILS;

    // Check minimum size
    if (width < StabilizerConstants::MIN_IMAGE_SIZE || height < StabilizerConstants::MIN_IMAGE_SIZE) {
        return false;
    }

    // Check maximum size (prevent integer overflow)
    if (width > MAX_FRAME_WIDTH || height > MAX_FRAME_HEIGHT) {
        return false;
    }

    return true;
}

/**
 * @brief Check if a feature point is valid
 *
 * @param point Feature point to validate
 * @param width Frame width
 * @param height Frame height
 * @return true if point is valid, false otherwise
 */
inline bool is_valid_feature_point(const cv::Point2f& point, int width, int height) {
    // Check if point is within frame bounds
    if (point.x < 0 || point.x >= width ||
        point.y < 0 || point.y >= height) {
        return false;
    }

    // Check for NaN or infinite values
    if (std::isnan(point.x) || std::isnan(point.y) ||
        std::isinf(point.x) || std::isinf(point.y)) {
        return false;
    }

    return true;
}

/**
 * @brief Validate transformation matrix
 *
 * @param transform 2x3 or 3x3 transformation matrix
 * @return true if transform is valid, false otherwise
 */
inline bool is_valid_transform(const cv::Mat& transform) {
    if (transform.empty()) {
        return false;
    }

    // Check matrix dimensions
    if ((transform.rows != 2 || transform.cols != 3) &&
        (transform.rows != 3 || transform.cols != 3)) {
        return false;
    }

    // Check for NaN or infinite values
    for (int i = 0; i < transform.rows; i++) {
        for (int j = 0; j < transform.cols; j++) {
            double val = transform.at<double>(i, j);
            if (std::isnan(val) || std::isinf(val)) {
                return false;
            }
        }
    }

    return true;
}

} // namespace VALIDATION
