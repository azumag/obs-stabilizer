#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "core/frame_utils.hpp"
#include "core/stabilizer_constants.hpp"

/**
 * Stateless frame analysis helpers kept separate from StabilizerCore.
 *
 * FrameAnalyzer owns no frame storage and does not retain references to the
 * supplied cv::Mat. Its methods are therefore safe to reuse across filter
 * instances as long as callers do not mutate a frame concurrently.
 */
class FrameAnalyzer {
public:
    /**
     * Validate dimensions, depth, and channel layout accepted by the
     * stabilization pipeline.
     */
    static bool is_valid_frame(const cv::Mat& frame) noexcept {
        // Preserve the shared frame policy first. In particular, the
        // stabilization pipeline accepts only CV_8U input; allowing 16-bit or
        // floating-point Mats through here would defer failure into OpenCV
        // feature/color-conversion routines.
        if (!FRAME_UTILS::Validation::validate_cv_mat(frame)) {
            return false;
        }

        if (frame.cols < StabilizerConstants::MIN_IMAGE_SIZE ||
            frame.rows < StabilizerConstants::MIN_IMAGE_SIZE ||
            frame.cols > StabilizerConstants::MAX_IMAGE_WIDTH ||
            frame.rows > StabilizerConstants::MAX_IMAGE_HEIGHT) {
            return false;
        }

        return true;
    }

    /**
     * Return the minimal rectangle containing pixels above the configured
     * content threshold.
     *
     * Empty, unsupported, or all-black frames deliberately fall back to the
     * complete frame rectangle. This preserves the existing edge-handling
     * behavior and avoids producing an invalid crop region.
     */
    static cv::Rect detect_content_bounds(const cv::Mat& frame) {
        if (frame.empty()) {
            return {};
        }

        cv::Mat gray = FRAME_UTILS::ColorConversion::convert_to_grayscale(frame);
        if (gray.empty()) {
            return full_frame(frame);
        }

        cv::Mat binary;
        cv::threshold(
            gray,
            binary,
            StabilizerConstants::ContentDetection::CONTENT_THRESHOLD,
            255,
            cv::THRESH_BINARY);

        std::vector<cv::Point> non_zero_pixels;
        cv::findNonZero(binary, non_zero_pixels);
        if (non_zero_pixels.empty()) {
            return full_frame(frame);
        }

        return cv::boundingRect(non_zero_pixels);
    }

private:
    static cv::Rect full_frame(const cv::Mat& frame) noexcept {
        return {0, 0, frame.cols, frame.rows};
    }
};
