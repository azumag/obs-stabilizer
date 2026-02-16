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

            // Calculate required buffer size based on format
            size_t required_size = 0;
            uint32_t linesizes[DATA_PLANES_COUNT] = {0};
            uint8_t* data_pointers[DATA_PLANES_COUNT] = {nullptr};

            switch (reference_frame->format) {
                case VIDEO_FORMAT_BGRA:
                case VIDEO_FORMAT_BGRX:
                case VIDEO_FORMAT_BGR3:
                    // Single plane formats
                    required_size = converted.total() * converted.elemSize();
                    linesizes[0] = static_cast<uint32_t>(converted.cols * converted.elemSize());
                    break;

                case VIDEO_FORMAT_NV12:
                    // NV12: Y plane + interleaved UV plane
                    {
                        int width = mat.cols;
                        int height = mat.rows;
                        int y_size = width * height;
                        int uv_size = (width * height) / 4;
                        required_size = y_size + uv_size * 2;
                        linesizes[0] = width;
                    }
                    break;

                case VIDEO_FORMAT_I420:
                    // I420: Y + U + V planes
                    {
                        int width = mat.cols;
                        int height = mat.rows;
                        int y_size = width * height;
                        int uv_size = (width / 2) * (height / 2);
                        required_size = y_size + uv_size * 2;
                        linesizes[0] = width;
                        linesizes[1] = width / 2;
                        linesizes[2] = width / 2;
                    }
                    break;

                default:
                    obs_log(LOG_ERROR, "Unsupported output format in FrameBuffer::create: %d", reference_frame->format);
                    Performance::track_conversion_failure();
                    return nullptr;
            }

            if (required_size == 0) {
                obs_log(LOG_ERROR, "Converted matrix has zero size");
                Performance::track_conversion_failure();
                return nullptr;
            }

            // Use RAII wrapper to manage obs_source_frame and data buffer
            // This addresses code review Issue #2: Manual Memory Management
            // The wrapper ensures automatic cleanup on any exception or early return
            try {
                OBSFrameRAII raii_frame(required_size);

                // Initialize frame structure with exception safety
                memset(raii_frame.get(), 0, sizeof(obs_source_frame));
                copy_frame_metadata(reference_frame, raii_frame.get());

                // Set data pointers and linesizes based on format
                raii_frame.get()->data[0] = raii_frame.get_data_buffer();
                raii_frame.get()->linesize[0] = linesizes[0];

                // Copy frame data
                if (reference_frame->format == VIDEO_FORMAT_I420) {
                    // I420: Copy Y, U, V planes separately
                    int y_size = mat.cols * mat.rows;
                    int uv_size = (mat.cols / 2) * (mat.rows / 2);
                    const uint8_t* src_y = converted.data;
                    const uint8_t* src_u = converted.data + y_size;
                    const uint8_t* src_v = src_u + uv_size;

                    memcpy(raii_frame.get()->data[0], src_y, y_size);
                    raii_frame.get()->data[1] = raii_frame.get()->data[0] + y_size;
                    memcpy(raii_frame.get()->data[1], src_u, uv_size);
                    raii_frame.get()->data[2] = raii_frame.get()->data[1] + uv_size;
                    memcpy(raii_frame.get()->data[2], src_v, uv_size);
                    raii_frame.get()->linesize[1] = linesizes[1];
                    raii_frame.get()->linesize[2] = linesizes[2];
                } else {
                    // All other formats: single plane copy
                    memcpy(raii_frame.get()->data[0], converted.data, required_size);
                }

                // Clear other data planes
                for (int i = 3; i < DATA_PLANES_COUNT; i++) {
                    raii_frame.get()->data[i] = nullptr;
                    raii_frame.get()->linesize[i] = 0;
                }

                // Release ownership of frame - caller is now responsible for cleanup
                return raii_frame.release();

            } catch (const std::bad_alloc& e) {
                // RAII wrapper handles cleanup automatically on exception
                obs_log(LOG_ERROR, "Memory allocation failed in FrameBuffer::create: %s", e.what());
                Performance::track_conversion_failure();
                return nullptr;
            } catch (...) {
                // RAII wrapper handles cleanup automatically on any exception
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
            // Free data buffer first (allocated by OBSFrameRAII)
            // Note: When OBSFrameRAII::release() is called, data_buffer ownership
            // is transferred to the caller, so we must delete it here
            if (frame->data[0]) {
                delete[] frame->data[0];
                frame->data[0] = nullptr;
            }

            // Free frame structure (allocated by OBSFrameRAII)
            delete frame;

        } catch (const std::exception& e) {
            obs_log(LOG_ERROR, "Exception in FrameBuffer::release: %s", e.what());
        }
    }

    cv::Mat FrameBuffer::convert_mat_format(const cv::Mat& mat, uint32_t target_format) {
        cv::Mat converted;

        // Convert to BGR3 first if needed (common intermediate format)
        cv::Mat bgr_mat;
        if (mat.channels() == 4) {
            cv::cvtColor(mat, bgr_mat, cv::COLOR_BGRA2BGR);
        } else if (mat.channels() == 3) {
            bgr_mat = mat;
        } else if (mat.channels() == 1) {
            cv::cvtColor(mat, bgr_mat, cv::COLOR_GRAY2BGR);
        } else {
            obs_log(LOG_ERROR, "Unsupported input channels: %d", mat.channels());
            return cv::Mat();
        }

        switch (target_format) {
            case VIDEO_FORMAT_BGRA:
                if (mat.channels() == 4) {
                    converted = mat;
                } else {
                    cv::cvtColor(mat, converted, cv::COLOR_BGR2BGRA);
                }
                break;
            case VIDEO_FORMAT_BGR3:
                converted = bgr_mat;
                break;
            case VIDEO_FORMAT_NV12:
                // Convert BGR to I420 first, then rearrange to NV12
                {
                    cv::Mat yuv420;
                    cv::cvtColor(bgr_mat, yuv420, cv::COLOR_BGR2YUV_I420);

                    // Rearrange I420 to NV12 (interleave UV)
                    int height = mat.rows;
                    int width = mat.cols;
                    int y_size = width * height;
                    int uv_size = (width * height) / 4;
                    converted.create(height + height / 2, width, CV_8UC1);

                    // Copy Y plane
                    memcpy(converted.data, yuv420.data, y_size);

                    // Interleave UV plane
                    const uint8_t* u_plane = yuv420.data + y_size;
                    const uint8_t* v_plane = u_plane + uv_size;
                    uint8_t* uv_plane = converted.data + y_size;
                    for (int i = 0; i < uv_size; i++) {
                        uv_plane[i * 2] = u_plane[i];
                        uv_plane[i * 2 + 1] = v_plane[i];
                    }
                }
                break;
            case VIDEO_FORMAT_I420:
                // Convert BGR directly to I420
                cv::cvtColor(bgr_mat, converted, cv::COLOR_BGR2YUV_I420);
                break;
            default:
                obs_log(LOG_ERROR, "Unsupported output format: %d", target_format);
                return cv::Mat();
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

    // NOTE: validate_cv_mat() is implemented inline in frame_utils.hpp to eliminate code duplication
    // The single inline implementation works in both OBS mode and standalone mode (testing).
    // This follows DRY principle - only one implementation to maintain.

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

    // NOTE: Using std::atomic for thread-safety. While OBS filters are
    // single-threaded by design, this ensures correctness in test environments
    // and potential future multi-threading scenarios. The atomic operations
    // have minimal overhead and provide safe access to statistics.
    //
    // REFACTORED: Removed unused total_time and total_conversions tracking to eliminate misleading metrics.
    // Detailed timing metrics are provided by StabilizerCore::PerformanceMetrics.
    // This namespace now focuses only on conversion failure tracking for diagnostics.

    namespace {
        struct PerformanceData {
            std::atomic<size_t> failed_conversions{0};
        };

        PerformanceData* get_perf_data() {
            // Static local variable is properly destroyed at shutdown by C++ runtime
            static PerformanceData instance;
            return &instance;
        }
    }

    void Performance::track_conversion_failure() {
        // Atomic operation - thread-safe with minimal overhead
        PerformanceData* data = get_perf_data();
        data->failed_conversions.fetch_add(1, std::memory_order_relaxed);
    }

    Performance::ConversionStats Performance::get_stats() {
        // Atomic load operation - thread-safe
        PerformanceData* data = get_perf_data();

        ConversionStats stats;
        stats.failed_conversions = data->failed_conversions.load(std::memory_order_relaxed);

        return stats;
    }

} // namespace FRAME_UTILS
