#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

namespace STABILIZER_PERFORMANCE {

/** Qualitative real-time processing health classification. */
enum class Status {
    Good,
    Warning,
    Poor,
};

/** Immutable-style snapshot of accumulated performance observations. */
struct Snapshot {
    /** Number of frames represented by this snapshot. */
    uint64_t frame_count = 0;
    /** Mean frame processing latency in milliseconds. */
    double average_processing_ms = 0.0;
    /** Estimated throughput derived from average latency. */
    double estimated_fps = 0.0;
    /** Percentage of frames with successful feature tracking. */
    double feature_success_rate = 0.0;
    /** Qualitative classification derived from processing latency. */
    Status status = Status::Good;
};

/** Classify a processing latency against the real-time thresholds. */
inline Status classify_processing_time(double processing_ms)
{
    if (processing_ms < 10.0) {
        return Status::Good;
    }
    if (processing_ms <= 20.0) {
        return Status::Warning;
    }
    return Status::Poor;
}

/** Return a human-readable label for a performance status. */
inline const char *status_label(Status status)
{
    switch (status) {
    case Status::Good:
        return "Good";
    case Status::Warning:
        return "Warning";
    case Status::Poor:
        return "Poor";
    }
    return "Unknown";
}

/** Return operator guidance appropriate for a performance status. */
inline std::string recommendation_for(Status status)
{
    switch (status) {
    case Status::Good:
        return "Performance is within the real-time target.";
    case Status::Warning:
        return "Consider reducing feature count or smoothing radius.";
    case Status::Poor:
        return "Reduce feature count and disable expensive options before increasing resolution.";
    }
    return "No recommendation available.";
}

/** Accumulates lightweight per-frame timing and tracking-success metrics. */
class Accumulator {
public:
    /** Record one frame observation. Negative latency is clamped to zero. */
    void record_frame(double processing_ms, bool feature_tracking_succeeded)
    {
        processing_ms = std::max(0.0, processing_ms);
        ++frame_count_;
        total_processing_ms_ += processing_ms;
        if (feature_tracking_succeeded) {
            ++successful_feature_frames_;
        }
    }

    /** Clear all accumulated observations. */
    void reset()
    {
        frame_count_ = 0;
        successful_feature_frames_ = 0;
        total_processing_ms_ = 0.0;
    }

    /** Return a derived snapshot of the current accumulated observations. */
    Snapshot snapshot() const
    {
        Snapshot result;
        result.frame_count = frame_count_;
        if (frame_count_ == 0) {
            return result;
        }

        result.average_processing_ms = total_processing_ms_ / static_cast<double>(frame_count_);
        result.estimated_fps = result.average_processing_ms > 0.0
            ? 1000.0 / result.average_processing_ms
            : 0.0;
        result.feature_success_rate =
            100.0 * static_cast<double>(successful_feature_frames_) /
            static_cast<double>(frame_count_);
        result.status = classify_processing_time(result.average_processing_ms);
        return result;
    }

private:
    uint64_t frame_count_ = 0;
    uint64_t successful_feature_frames_ = 0;
    double total_processing_ms_ = 0.0;
};

} // namespace STABILIZER_PERFORMANCE
