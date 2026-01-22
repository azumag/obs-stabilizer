/*
 * OBS Stabilizer Plugin - Frame Conversion Utilities
 * Unified frame conversion logic to eliminate code duplication
 */

#pragma once

#ifdef HAVE_OBS_HEADERS
#include <obs-module.h>
#else
#include "obs-module.h"
#endif

#include <opencv2/opencv.hpp>
#include <mutex>
#include <vector>
#include <string>

namespace FRAME_UTILS {

    // Frame format enumeration
    enum class FrameFormat {
        BGRA,
        BGRX,
        BGR3,
        NV12,
        I420,
        UNKNOWN
    };

    // Frame conversion utilities
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

    // Thread-safe frame buffer management
    //
    // Mutex Usage Analysis:
    // - Single static mutex protects shared buffer across all FrameBuffer::create() calls
    // - Typical OBS plugin usage: single video source → minimal contention
    // - Multi-source scenario: each call waits for mutex → potential bottleneck
    // - Current design prioritizes simplicity and memory efficiency over maximum throughput
    // - Alternative: Per-source buffers would eliminate mutex but increase memory usage
    class FrameBuffer {
    public:
        // Create frame buffer from OpenCV Mat
        static obs_source_frame* create(const cv::Mat& mat, 
                                       const obs_source_frame* reference_frame);
        
        // Clean up resources
        static void cleanup();
        
        // Get current buffer size for debugging
        static size_t get_buffer_size();
        
    private:
        // Static members for thread-safe buffer management
        static std::mutex mutex_;
        static struct {
            std::vector<uint8_t> buffer;
            obs_source_frame frame;
            bool initialized;
        } buffer_;
        
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

    // Performance monitoring
    namespace Performance {
        // Track conversion performance
        void track_conversion_time(const std::string& operation, double duration_ms);
        
        // Get performance statistics
        struct ConversionStats {
            size_t total_conversions;
            double avg_conversion_time;
            size_t failed_conversions;
        };
        
        static ConversionStats get_stats();
    }

} // namespace FRAME_UTILS