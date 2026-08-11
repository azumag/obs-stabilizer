/*
 * OBS Stabilizer Plugin - Frame Conversion Utilities
 * Unified frame conversion logic to eliminate code duplication
 */

#pragma once

#include <opencv2/opencv.hpp>
#include "obs_compat.h"
#include <memory>
#include <vector>
#include <string>
#include <climits>
#include <cstring>
#include <atomic>

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
        // Convert OBS frame to an owning OpenCV Mat copy.
        cv::Mat obs_to_cv(const obs_source_frame* frame);

        /**
         * Create a zero-copy OpenCV view over a packed OBS frame.
         *
         * The returned Mat borrows frame->data[0]. The caller must not retain it
         * after the source obs_source_frame or its backing storage becomes invalid.
         * Only packed BGRA, BGRX, and BGR3 frames are supported because planar
         * formats require color conversion and therefore cannot be represented as
         * a single borrowed Mat with the current API.
         *
         * @param frame Source OBS frame whose storage remains owned by OBS.
         * @return Borrowed Mat view, or an empty Mat for invalid/unsupported input.
         */
        inline cv::Mat obs_to_cv_view(const obs_source_frame* frame) {
            if (!frame || !frame->data[0] || frame->width == 0 || frame->height == 0 ||
                frame->width > MAX_FRAME_WIDTH || frame->height > MAX_FRAME_HEIGHT) {
                return cv::Mat();
            }

            switch (frame->format) {
                case VIDEO_FORMAT_BGRA:
                case VIDEO_FORMAT_BGRX:
                    return cv::Mat(frame->height, frame->width, CV_8UC4,
                                   frame->data[0], frame->linesize[0]);
                case VIDEO_FORMAT_BGR3:
                    return cv::Mat(frame->height, frame->width, CV_8UC3,
                                   frame->data[0], frame->linesize[0]);
                default:
                    return cv::Mat();
            }
        }

        // Convert OpenCV Mat to OBS frame
        obs_source_frame* cv_to_obs(const cv::Mat& mat, const obs_source_frame* reference_frame);

        // Convert and copy pixels into an existing OBS-owned frame
        bool cv_to_obs_in_place(const cv::Mat& mat, obs_source_frame* destination_frame);

        // Get format name for logging
        std::string get_format_name(uint32_t obs_format);

        // Check if format is supported
        bool is_supported_format(uint32_t obs_format);
    }

    // Per-call frame buffer management
    class FrameBuffer {
    public:
        // Create frame buffer from OpenCV Mat
        static obs_source_frame* create(const cv::Mat& mat,
                                        const obs_source_frame* reference_frame);

        // Release frame buffer (manual cleanup if needed)
        static void release(obs_source_frame* frame);

    private:
        // Format conversion helpers
        static cv::Mat convert_mat_format(const cv::Mat& mat, uint32_t target_format);
        static void copy_frame_metadata(const obs_source_frame* src, obs_source_frame* dst);

        // RAII wrapper for obs_source_frame to ensure proper memory management
        // This addresses the code review Issue #2: Manual Memory Management
        class OBSFrameRAII {
        private:
            obs_source_frame* frame_;
            std::unique_ptr<uint8_t[]> data_buffer_;

            // Prevent copying - single ownership model
            OBSFrameRAII(const OBSFrameRAII&) = delete;
            OBSFrameRAII& operator=(const OBSFrameRAII&) = delete;

        public:
            // Constructor - allocates data buffer and frame structure
            // Uses RAII pattern to ensure resources are properly managed
            explicit OBSFrameRAII(size_t data_size) {
                // Allocate data buffer with unique_ptr for automatic cleanup
                data_buffer_ = std::make_unique<uint8_t[]>(data_size);

                // Allocate frame structure (OBS requires heap allocation)
                // Using new (nothrow) to avoid exceptions during allocation failure
                frame_ = new (std::nothrow) obs_source_frame();
                if (!frame_) {
                    data_buffer_.reset();  // Clean up buffer allocation
                    throw std::bad_alloc();
                }
            }

            // Destructor - cleans up both frame and data buffer
            // data_buffer_ is automatically freed by unique_ptr
            ~OBSFrameRAII() {
                if (frame_) {
                    delete frame_;
                    frame_ = nullptr;
                }
                // data_buffer_ is automatically freed by unique_ptr destructor
            }

            // Get raw pointer to frame structure
            obs_source_frame* get() const { return frame_; }

            // Get raw pointer to data buffer
            uint8_t* get_data_buffer() const { return data_buffer_.get(); }

            // Release ownership of frame structure
            // Caller becomes responsible for deleting the frame and its data buffer
            // Returns the frame pointer and transfers ownership
            obs_source_frame* release() {
                obs_source_frame* tmp = frame_;
                frame_ = nullptr;
                // data_buffer_ is still owned by this wrapper, so we must transfer it
                // The caller is now responsible for deleting data_buffer_ when done
                uint8_t* buffer_ptr = data_buffer_.release();
                if (tmp) {
                    tmp->data[0] = buffer_ptr;
                }
                return tmp;
            }
        };
    };
#endif

    // Validation utilities
    namespace Validation {
        // Validate OpenCV Mat (inline for both OBS and standalone modes)
        // RATIONALE: This is implemented inline to eliminate code duplication.
        // The single implementation serves both OBS mode and standalone mode (testing).
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
            // and are not compatible with current stabilization algorithms
            int depth = mat.depth();
            if (depth != CV_8U) {
                return false;
            }

            // Validate channel count
            // 1-channel (grayscale), 3-channel (BGR), and 4-channel (BGRA) formats are supported
            // 2-channel formats are not supported by current processing pipeline
            int channels = mat.channels();
            if (channels != 1 && channels != 3 && channels != 4) {
                return false;
            }

            return true;
        }

#ifdef HAVE_OBS_HEADERS
        // OBS-specific validation utilities (only available when OBS headers are present)
        // Validate OBS frame structure
        bool validate_obs_frame(const obs_source_frame* frame);

        // Get error message for invalid frame
        std::string get_frame_error_message(const obs_source_frame* frame);
#endif
    }

    // Performance monitoring (available in both modes)
    // RATIONALE: Tracks conversion failures to help diagnose issues.
    // Detailed timing metrics are provided by StabilizerCore::PerformanceMetrics,
    // so this namespace focuses only on conversion failure tracking.
    namespace Performance {
        // Track conversion failures
        void track_conversion_failure();

        // Get performance statistics
        struct ConversionStats {
            size_t failed_conversions;
        };

        ConversionStats get_stats();
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
