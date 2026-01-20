/*
 * OBS Stabilizer Plugin - Frame Conversion Utilities Implementation
 * Unified frame conversion logic to eliminate code duplication
 */

#include "frame_utils.hpp"
#include "stabilizer_constants_c.h"
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
                        cv::Mat yuv(frame->height + frame->height/2, frame->width, 
                                   CV_8UC1, frame->data[0]);
                        cv::cvtColor(yuv, mat, cv::COLOR_YUV2BGRA_I420);
                    }
                    break;

                default:
                    obs_log(LOG_ERROR, "Unsupported frame format: %d", frame->format);
                    return cv::Mat();
            }

            return mat.clone();

        } catch (const cv::Exception& e) {
            obs_log(LOG_ERROR, "OpenCV exception in obs_to_cv: %s", e.what());
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
            
            if (buffer_.buffer.size() < required_size) {
                buffer_.buffer.resize(required_size);
            } else if (buffer_.buffer.size() > required_size * MEMORY_GROWTH_FACTOR) {
                buffer_.buffer.shrink_to_fit();
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
