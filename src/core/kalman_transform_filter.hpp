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
    /** Noise covariance parameters used to initialize the Kalman model. */
    struct NoiseConfig {
        /** Process-noise covariance applied to the motion model. */
        float process_noise = 1e-3f;
        /** Measurement-noise covariance applied to observed transforms. */
        float measurement_noise = 1e-1f;
        /** Initial posterior error covariance. */
        float initial_error = 1.0f;
    };

    /** Construct a filter with default noise parameters. */
    KalmanTransformFilter();
    /** Construct a filter with explicit noise parameters. */
    explicit KalmanTransformFilter(const NoiseConfig& config);

    /** Incorporate a transform measurement and return the filtered state. */
    cv::Vec4f update(const cv::Vec4f& measurement, float delta_time = 1.0f);
    /** Predict the next transform without incorporating a measurement. */
    cv::Vec4f predict(float delta_time = 1.0f);
    /** Clear filter state so the next measurement initializes the model. */
    void reset();
    /** Return true after the model has been initialized by a measurement. */
    bool is_initialized() const noexcept;

private:
    void configure_transition(float delta_time);
    void initialize_from_measurement(const cv::Vec4f& measurement);

    cv::KalmanFilter filter_;
    NoiseConfig config_;
    bool initialized_ = false;
};
