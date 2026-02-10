/*
 * OBS Stabilizer Plugin - Frame Conversion Utilities
 * Unified frame conversion logic to eliminate code duplication
 */

#pragma once

#ifdef HAVE_OBS_HEADERS
#include <obs-module.h>
#endif

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <climits> // For SIZE_MAX reference (security audit)
#include <cstring> // For memset and memcpy

namespace FRAME_UTILS {

    // Constants for frame buffer management
    constexpr int DATA_PLANES_COUNT = 8;

    // Maximum dimension constraints (prevent integer overflow)
    constexpr uint32_t MAX_FRAME_WIDTH = 16384;   // 16K
    constexpr uint32_t MAX_FRAME_HEIGHT = 16384;  // 16K

    // Frame format enumeration
    enum class FrameFormat {
        BGRA,
        BGRX,
        BGR3,
        NV12,
        I420,
        UNKNOWN
    };

#ifdef HAVE_OBS_HEADERS
    // Frame conversion utilities (only available when OBS headers are present)
    namespace Conversion {
        // Convert OBS frame to OpenCV Mat
        cv::Mat obs_to_cv(const obs_source_frame* frame);
        
        // Convert OpenCV Mat to OBS frame
        obs_source_frame* cv_to_obs(const cv::Mat& mat, const obs_source_frame* reference_frame);
        
        // Get format name for logging
        std::string get_format_name(uint32_t obs_format);
        
        // Check if format is supported
        bool is_supported_format(uint32_t obs_format);
    }

    // Per-call frame buffer management
    //
    // Implementation Notes:
    // - Each create() call allocates a new independent buffer
    // - Ownership is transferred to caller (OBS)
    // - OBS handles buffer deallocation when frame is consumed
    // - This approach prevents buffer corruption in multi-threaded scenarios
    // - Trade-off: Higher memory usage vs. thread safety and correctness
    //
    // Lifecycle:
    // 1. create() allocates new obs_source_frame and data buffer
    // 2. Pointer is returned to caller
    // 3. OBS processes the frame
    // 4. OBS deallocates the frame when done (obs_source_output_video)
    // 5. Optionally, caller can manually release() if needed
    class FrameBuffer {
    public:
        // Create frame buffer from OpenCV Mat
        // Returns a new buffer with ownership transferred to caller
        // Caller must ensure proper deallocation (typically handled by OBS)
        static obs_source_frame* create(const cv::Mat& mat,
                                        const obs_source_frame* reference_frame);

        // Release frame buffer (manual cleanup if needed)
        // Use this if OBS didn't consume the frame or for custom workflows
        // Note: Only use if you know OBS won't handle deallocation
        static void release(obs_source_frame* frame);

    private:
        // Format conversion helpers
        static cv::Mat convert_mat_format(const cv::Mat& mat, uint32_t target_format);
        static void copy_frame_metadata(const obs_source_frame* src, obs_source_frame* dst);
    };

    // Validation utilities
    namespace Validation {
        // Validate OBS frame structure
        bool validate_obs_frame(const obs_source_frame* frame);
        
        // Validate OpenCV Mat
        bool validate_cv_mat(const cv::Mat& mat);
        
        // Get error message for invalid frame
        std::string get_frame_error_message(const obs_source_frame* frame);
    }
#else
    // Minimal validation utilities for standalone mode (no OBS dependencies)
    namespace Validation {
        // Validate OpenCV Mat only
        // This implementation mirrors the full version in frame_utils.cpp
        // to ensure consistent behavior between standalone and OBS-linked builds
        inline bool validate_cv_mat(const cv::Mat& mat) {
            if (mat.empty()) {
                return false;
            }

            // Check for invalid dimensions
            // cv::Mat can have negative dimensions when constructed with invalid parameters
            // These should be rejected as they indicate corrupted or improperly initialized data
            if (mat.rows <= 0 || mat.cols <= 0) {
                return false;
            }

            // Validate pixel depth - only 8-bit unsigned formats are supported
            // 16-bit (CV_16UC*) and other formats require different processing pipelines
            // and are not compatible with the current stabilization algorithms
            int depth = mat.depth();
            if (depth != CV_8U) {
                // Unsupported bit depth - only 8-bit unsigned is supported
                return false;
            }

            // Validate channel count
            // 1-channel (grayscale), 3-channel (BGR), and 4-channel (BGRA) formats are supported
            // 2-channel formats are not supported by the current processing pipeline
            int channels = mat.channels();
            if (channels != 1 && channels != 3 && channels != 4) {
                // Unsupported channel count
                return false;
            }

            return true;
        }
    }
#endif

    // Performance monitoring (available in both modes)
    namespace Performance {
        // Track conversion performance
        void track_conversion_time(const std::string& operation, double duration_ms);
        
        // Track conversion failures
        void track_conversion_failure();
        
        // Get performance statistics
        struct ConversionStats {
            size_t total_conversions;
            double avg_conversion_time;
            size_t failed_conversions;
        };
        
        static ConversionStats get_stats();
    }

    // Color conversion utilities (available in both modes)
    namespace ColorConversion {
        /**
         * Convert OpenCV Mat to grayscale
         * Supports BGRA, BGR, and grayscale input formats
         * @param frame Input frame (BGRA, BGR, or grayscale)
         * @return Grayscale Mat or empty Mat on error
         */
        inline cv::Mat convert_to_grayscale(const cv::Mat& frame) {
            if (frame.empty()) {
                return cv::Mat();
            }

            cv::Mat gray;
            switch (frame.channels()) {
                case 4:
                    cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
                    break;
                case 3:
                    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
                    break;
                case 1:
                    gray = frame;
                    break;
                default:
                    return cv::Mat();
            }
            return gray;
        }
    }

} // namespace FRAME_UTILS
