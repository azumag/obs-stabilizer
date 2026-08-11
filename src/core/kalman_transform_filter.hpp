#pragma once

#include <opencv2/core.hpp>
#include <opencv2/video/tracking.hpp>

/**
 * Smooths affine transform components with a constant-velocity Kalman model.
 *
 * State: [dx, dy, angle, scale, vx, vy, vangle, vscale]
 * Measurement: [dx, dy, angle, scale]
 */
class KalmanTransformFilter {
public:
    struct NoiseConfig {
        float process_noise = 1e-3f;
        float measurement_noise = 1e-1f;
        float initial_error = 1.0f;
    };

    KalmanTransformFilter();
    explicit KalmanTransformFilter(const NoiseConfig& config);

    cv::Vec4f update(const cv::Vec4f& measurement, float delta_time = 1.0f);
    cv::Vec4f predict(float delta_time = 1.0f);
    void reset();
    bool is_initialized() const noexcept;

private:
    void configure_transition(float delta_time);
    void initialize_from_measurement(const cv::Vec4f& measurement);

    cv::KalmanFilter filter_;
    NoiseConfig config_;
    bool initialized_ = false;
};
