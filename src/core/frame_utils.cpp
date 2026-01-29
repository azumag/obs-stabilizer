/*
 * OBS Stabilizer Plugin - Frame Conversion Utilities Implementation
 * Unified frame conversion logic to eliminate code duplication
 */

#include "frame_utils.hpp"
#include <cstring>
#include <algorithm>

namespace FRAME_UTILS {

    // Static member initialization
    std::mutex FrameBuffer::mutex_;
    FrameBuffer::BufferStruct FrameBuffer::buffer_ = { {}, {}, false };

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

                        // Check for integer overflow in total size
                        const size_t total_size = y_size + uv_size * 2;
                        if (y_size > 0 && total_size < y_size) {
                            obs_log(LOG_ERROR, "Integer overflow in I420 buffer size calculation");
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
            Performance::track_conversion_failure();
            return nullptr;
        }

        try {
            std::lock_guard<std::mutex> lock(mutex_);

            if (!buffer_.initialized) {
                buffer_.frame = *reference_frame;
                buffer_.initialized = true;
            }

            buffer_.frame.width = reference_frame->width;
            buffer_.frame.height = reference_frame->height;
            buffer_.frame.format = reference_frame->format;
            buffer_.frame.timestamp = reference_frame->timestamp;

            cv::Mat converted = convert_mat_format(mat, reference_frame->format);

            size_t required_size = converted.total() * converted.elemSize();

            if (converted.empty()) {
                obs_log(LOG_ERROR, "Converted matrix is empty in FrameBuffer::create");
                Performance::track_conversion_failure();
                return nullptr;
            }

            if (required_size == 0) {
                obs_log(LOG_ERROR, "Required buffer size is zero in FrameBuffer::create");
                Performance::track_conversion_failure();
                return nullptr;
            }

            if (!converted.data) {
                obs_log(LOG_ERROR, "Converted matrix data pointer is null in FrameBuffer::create");
                Performance::track_conversion_failure();
                return nullptr;
            }

            if (converted.data == nullptr) {
                obs_log(LOG_ERROR, "Converted matrix data pointer is null in FrameBuffer::create");
                Performance::track_conversion_failure();
                return nullptr;
            }

            if (buffer_.buffer.size() < required_size) {
                buffer_.buffer.resize(required_size);
            } else if (buffer_.buffer.size() > required_size * MEMORY_GROWTH_FACTOR) {
                buffer_.buffer.shrink_to_fit();
            }

            if (buffer_.buffer.size() < required_size) {
                obs_log(LOG_ERROR, "Buffer allocation failed: requested %zu, got %zu",
                         required_size, buffer_.buffer.size());
                Performance::track_conversion_failure();
                return nullptr;
            }

            buffer_.frame.data[0] = buffer_.buffer.data();
            buffer_.frame.linesize[0] = static_cast<uint32_t>(converted.step);

            memcpy(buffer_.frame.data[0], converted.data, required_size);

            for (int i = 1; i < DATA_PLANES_COUNT; i++) {
                buffer_.frame.data[i] = nullptr;
                buffer_.frame.linesize[i] = 0;
            }

            return &buffer_.frame;

        } catch (const std::exception& e) {
            obs_log(LOG_ERROR, "Exception in FrameBuffer::create: %s", e.what());
            Performance::track_conversion_failure();
            return nullptr;
        }
    }

    void FrameBuffer::cleanup() {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_.buffer.clear();
        buffer_.buffer.shrink_to_fit();
        buffer_.initialized = false;
    }

    size_t FrameBuffer::get_buffer_size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return buffer_.buffer.size();
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

        dst->width = src->width;
        dst->height = src->height;
        dst->format = src->format;
        dst->timestamp = src->timestamp;
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

        if (mat.rows <= 0 || mat.cols <= 0) {
            return false;
        }

        if (mat.type() != CV_8UC3 && mat.type() != CV_8UC4) {
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
