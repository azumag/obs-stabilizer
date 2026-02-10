/*
 * OBS Stabilizer Plugin - Frame Conversion Utilities Implementation
 * Unified frame conversion logic to eliminate code duplication
 */

#include "frame_utils.hpp"
#include <cstring>
#include <algorithm>

namespace FRAME_UTILS {



    // ============================================================================
    // Conversion Implementation
    // ============================================================================

    cv::Mat Conversion::obs_to_cv(const obs_source_frame* frame) {
        if (!frame || !frame->data[0]) {
            Performance::track_conversion_failure();
            return cv::Mat();
        }

        // Validate frame dimensions to prevent integer overflow
        if (frame->width == 0 || frame->height == 0 ||
            frame->width > MAX_FRAME_WIDTH || frame->height > MAX_FRAME_HEIGHT) {
            obs_log(LOG_ERROR, "Invalid frame dimensions: %ux%u (max: %ux%u)",
                     frame->width, frame->height, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);
            Performance::track_conversion_failure();
            return cv::Mat();
        }

        try {
            cv::Mat mat;

            switch (frame->format) {
                case VIDEO_FORMAT_BGRA:
                    mat = cv::Mat(frame->height, frame->width, CV_8UC4, 
                                 frame->data[0], frame->linesize[0]);
                    break;

                case VIDEO_FORMAT_BGRX:
                    mat = cv::Mat(frame->height, frame->width, CV_8UC4, 
                                 frame->data[0], frame->linesize[0]);
                    break;

                case VIDEO_FORMAT_BGR3:
                    mat = cv::Mat(frame->height, frame->width, CV_8UC3, 
                                 frame->data[0], frame->linesize[0]);
                    break;

                case VIDEO_FORMAT_NV12:
                    {
                        cv::Mat yuv(frame->height + frame->height/2, frame->width,
                                   CV_8UC1, frame->data[0]);
                        cv::cvtColor(yuv, mat, cv::COLOR_YUV2BGRA_NV12);
                    }
                    break;

                case VIDEO_FORMAT_I420:
                    {
                        if (!frame->data[1] || !frame->data[2]) {
                            obs_log(LOG_ERROR, "I420 format missing U/V plane data");
                            return cv::Mat();
                        }

                        // Calculate sizes with overflow protection
                        // Note: SIZE_MAX from <climits> used for overflow detection reference
                        // Use size_t for size calculations (unsigned, larger range than int)
                        const size_t y_size = static_cast<size_t>(frame->width) *
                                             static_cast<size_t>(frame->height);
                        const size_t uv_size = static_cast<size_t>(frame->width / 2) *
                                               static_cast<size_t>(frame->height / 2);

                        // Check for integer overflow in uv_size * 2 multiplication
                        // This must be checked before the final addition to prevent overflow
                        const size_t uv_size_doubled = uv_size * 2;
                        if (uv_size > 0 && uv_size_doubled / 2 != uv_size) {
                            obs_log(LOG_ERROR, "Integer overflow in I420 UV size calculation");
                            Performance::track_conversion_failure();
                            return cv::Mat();
                        }

                        // Check for integer overflow in final total size addition
                        const size_t total_size = y_size + uv_size_doubled;
                        if (y_size > 0 && total_size < y_size) {
                            obs_log(LOG_ERROR, "Integer overflow in I420 total size calculation");
                            Performance::track_conversion_failure();
                            return cv::Mat();
                        }

                        std::vector<uint8_t> yuv_buffer;
                        yuv_buffer.resize(total_size);

                        uint8_t* yuv_ptr = yuv_buffer.data();

                        memcpy(yuv_ptr, frame->data[0], y_size);
                        yuv_ptr += y_size;

                        memcpy(yuv_ptr, frame->data[1], uv_size);
                        yuv_ptr += uv_size;

                        memcpy(yuv_ptr, frame->data[2], uv_size);

                        cv::Mat yuv(frame->height + frame->height / 2, frame->width,
                                   CV_8UC1, yuv_buffer.data());
                        cv::cvtColor(yuv, mat, cv::COLOR_YUV2BGRA_I420);
                    }
                    break;

                default:
                    obs_log(LOG_ERROR, "Unsupported frame format: %d", frame->format);
                    Performance::track_conversion_failure();
                    return cv::Mat();
            }

            return mat.clone();

        } catch (const cv::Exception& e) {
            obs_log(LOG_ERROR, "OpenCV exception in obs_to_cv: %s", e.what());
            Performance::track_conversion_failure();
            return cv::Mat();
        }
    }

    obs_source_frame* Conversion::cv_to_obs(const cv::Mat& mat, 
                                           const obs_source_frame* reference_frame) {
        return FrameBuffer::create(mat, reference_frame);
    }

    std::string Conversion::get_format_name(uint32_t obs_format) {
        switch (obs_format) {
            case VIDEO_FORMAT_BGRA: return "BGRA";
            case VIDEO_FORMAT_BGRX: return "BGRX";
            case VIDEO_FORMAT_BGR3: return "BGR3";
            case VIDEO_FORMAT_NV12: return "NV12";
            case VIDEO_FORMAT_I420: return "I420";
            default: return "UNKNOWN";
        }
    }

    bool Conversion::is_supported_format(uint32_t obs_format) {
        return obs_format == VIDEO_FORMAT_BGRA ||
               obs_format == VIDEO_FORMAT_BGRX ||
               obs_format == VIDEO_FORMAT_BGR3 ||
               obs_format == VIDEO_FORMAT_NV12 ||
               obs_format == VIDEO_FORMAT_I420;
    }

    // ============================================================================
    // FrameBuffer Implementation
    // ============================================================================

    obs_source_frame* FrameBuffer::create(const cv::Mat& mat,
                                         const obs_source_frame* reference_frame) {
        if (mat.empty() || !reference_frame) {
            obs_log(LOG_ERROR, "Invalid input in FrameBuffer::create: mat=%s, ref=%s",
                    mat.empty() ? "empty" : "valid",
                    reference_frame ? "valid" : "null");
            Performance::track_conversion_failure();
            return nullptr;
        }

        try {
            // Validate reference frame dimensions
            if (reference_frame->width == 0 || reference_frame->height == 0) {
                obs_log(LOG_ERROR, "Invalid reference frame dimensions: %ux%u",
                        reference_frame->width, reference_frame->height);
                Performance::track_conversion_failure();
                return nullptr;
            }

            // Convert Mat to target format
            cv::Mat converted = convert_mat_format(mat, reference_frame->format);

            if (converted.empty()) {
                obs_log(LOG_ERROR, "Failed to convert Mat to target format");
                Performance::track_conversion_failure();
                return nullptr;
            }

            // Calculate required buffer size
            size_t required_size = converted.total() * converted.elemSize();

            if (required_size == 0) {
                obs_log(LOG_ERROR, "Converted matrix has zero size");
                Performance::track_conversion_failure();
                return nullptr;
            }

            // Allocate obs_source_frame and data buffer together
            // This ensures both are deallocated together
            uint8_t* data_buffer = new (std::nothrow) uint8_t[required_size];
            if (!data_buffer) {
                obs_log(LOG_ERROR, "Failed to allocate data buffer: %zu bytes", required_size);
                Performance::track_conversion_failure();
                return nullptr;
            }

            obs_source_frame* frame = new (std::nothrow) obs_source_frame();
            if (!frame) {
                delete[] data_buffer;
                obs_log(LOG_ERROR, "Failed to allocate obs_source_frame structure");
                Performance::track_conversion_failure();
                return nullptr;
            }

            // Initialize frame structure with exception safety
            // Wrap operations that could potentially throw to prevent memory leaks
            try {
                memset(frame, 0, sizeof(obs_source_frame));
                copy_frame_metadata(reference_frame, frame);

                // Set data pointers
                frame->data[0] = data_buffer;
                frame->linesize[0] = static_cast<uint32_t>(converted.step);

                // Copy frame data
                memcpy(frame->data[0], converted.data, required_size);

                // Clear other data planes
                for (int i = 1; i < DATA_PLANES_COUNT; i++) {
                    frame->data[i] = nullptr;
                    frame->linesize[i] = 0;
                }

                return frame;

            } catch (...) {
                // Clean up resources if any exception occurs during initialization
                delete[] data_buffer;
                delete frame;
                obs_log(LOG_ERROR, "Exception during frame buffer initialization");
                Performance::track_conversion_failure();
                return nullptr;
            }

        } catch (const std::bad_alloc& e) {
            obs_log(LOG_ERROR, "Memory allocation failed in FrameBuffer::create: %s", e.what());
            Performance::track_conversion_failure();
            return nullptr;
        } catch (const std::exception& e) {
            obs_log(LOG_ERROR, "Exception in FrameBuffer::create: %s", e.what());
            Performance::track_conversion_failure();
            return nullptr;
        }
    }

    void FrameBuffer::release(obs_source_frame* frame) {
        if (!frame) {
            return;
        }

        try {
            // Free data buffer
            if (frame->data[0]) {
                delete[] frame->data[0];
            }

            // Free frame structure
            delete frame;

        } catch (const std::exception& e) {
            obs_log(LOG_ERROR, "Exception in FrameBuffer::release: %s", e.what());
        }
    }

    cv::Mat FrameBuffer::convert_mat_format(const cv::Mat& mat, uint32_t target_format) {
        cv::Mat converted;

        switch (target_format) {
            case VIDEO_FORMAT_BGRA:
                if (mat.channels() == 4) {
                    converted = mat;
                } else {
                    cv::cvtColor(mat, converted, cv::COLOR_BGR2BGRA);
                }
                break;
            case VIDEO_FORMAT_BGR3:
                if (mat.channels() == 3) {
                    converted = mat;
                } else {
                    cv::cvtColor(mat, converted, cv::COLOR_BGRA2BGR);
                }
                break;
            default:
                obs_log(LOG_ERROR, "Unsupported output format: %d", target_format);
                return mat;
        }

        return converted;
    }

    void FrameBuffer::copy_frame_metadata(const obs_source_frame* src,
                                        obs_source_frame* dst) {
        if (!src || !dst) {
            return;
        }

        // Copy all relevant metadata fields from obs_source_frame structure
        // These fields are essential for proper OBS frame handling
        dst->width = src->width;
        dst->height = src->height;
        dst->format = src->format;
        dst->timestamp = src->timestamp;

        // Copy color range information for accurate color reproduction
        // full_range: 0 = limited range (16-235), 1 = full range (0-255)
        dst->full_range = src->full_range;

        // Copy flip flag for vertical flip handling
        // flip: non-zero value indicates frame is vertically flipped
        dst->flip = src->flip;

        // Copy flags for additional frame properties
        // This includes various OBS-specific flags (e.g., keyframe, preroll)
        dst->flags = src->flags;

        // Note: Other fields like min/max_timestamp may be relevant for some use cases
        // but are not always set by OBS. We copy only the essential fields here.
    }

    // ============================================================================
    // Validation Implementation
    // ============================================================================

    bool Validation::validate_obs_frame(const obs_source_frame* frame) {
        if (!frame) {
            return false;
        }

        if (!frame->data[0]) {
            return false;
        }

        if (frame->width == 0 || frame->height == 0) {
            return false;
        }

        if (!Conversion::is_supported_format(frame->format)) {
            return false;
        }

        return true;
    }

    bool Validation::validate_cv_mat(const cv::Mat& mat) {
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

    std::string Validation::get_frame_error_message(const obs_source_frame* frame) {
        if (!frame) {
            return "Frame is null";
        }

        if (!frame->data[0]) {
            return "Frame data is null";
        }

        if (frame->width == 0 || frame->height == 0) {
            return "Invalid frame dimensions";
        }

        if (!Conversion::is_supported_format(frame->format)) {
            return "Unsupported frame format: " + Conversion::get_format_name(frame->format);
        }

        return "Unknown validation error";
    }

    // ============================================================================
    // Performance Implementation
    // ============================================================================

    namespace {
        struct PerformanceData {
            size_t total_conversions;
            double total_time;
            size_t failed_conversions;
        };

        PerformanceData perf_data = {0, 0.0, 0};
        std::mutex perf_mutex;
    }

    void Performance::track_conversion_time(const std::string& operation, double duration_ms) {
        std::lock_guard<std::mutex> lock(perf_mutex);
        perf_data.total_conversions++;
        perf_data.total_time += duration_ms;
    }

    void Performance::track_conversion_failure() {
        std::lock_guard<std::mutex> lock(perf_mutex);
        perf_data.failed_conversions++;
    }

    Performance::ConversionStats Performance::get_stats() {
        std::lock_guard<std::mutex> lock(perf_mutex);

        ConversionStats stats;
        stats.total_conversions = perf_data.total_conversions;
        stats.avg_conversion_time = perf_data.total_conversions > 0 
            ? perf_data.total_time / perf_data.total_conversions 
            : 0.0;
        stats.failed_conversions = perf_data.failed_conversions;

        return stats;
    }

} // namespace FRAME_UTILS
