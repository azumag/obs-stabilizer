#include "kalman_transform_filter.hpp"

#include <algorithm>
#include <stdexcept>

KalmanTransformFilter::KalmanTransformFilter()
    : KalmanTransformFilter(NoiseConfig{}) {}

KalmanTransformFilter::KalmanTransformFilter(const NoiseConfig& config)
    : filter_(8, 4, 0, CV_32F), config_(config) {
    if (config_.process_noise <= 0.0f || config_.measurement_noise <= 0.0f ||
        config_.initial_error <= 0.0f) {
        throw std::invalid_argument("Kalman noise values must be positive");
    }

    filter_.measurementMatrix = cv::Mat::zeros(4, 8, CV_32F);
    for (int i = 0; i < 4; ++i) {
        filter_.measurementMatrix.at<float>(i, i) = 1.0f;
    }
    cv::setIdentity(filter_.processNoiseCov, cv::Scalar(config_.process_noise));
    cv::setIdentity(filter_.measurementNoiseCov, cv::Scalar(config_.measurement_noise));
    cv::setIdentity(filter_.errorCovPost, cv::Scalar(config_.initial_error));
    configure_transition(1.0f);
}

void KalmanTransformFilter::configure_transition(float delta_time) {
    const float dt = std::max(delta_time, 1e-4f);
    filter_.transitionMatrix = cv::Mat::eye(8, 8, CV_32F);
    for (int i = 0; i < 4; ++i) {
        filter_.transitionMatrix.at<float>(i, i + 4) = dt;
    }
}

void KalmanTransformFilter::initialize_from_measurement(const cv::Vec4f& measurement) {
    filter_.statePost = cv::Mat::zeros(8, 1, CV_32F);
    for (int i = 0; i < 4; ++i) {
        filter_.statePost.at<float>(i) = measurement[i];
    }
    cv::setIdentity(filter_.errorCovPost, cv::Scalar(config_.initial_error));
    initialized_ = true;
}

cv::Vec4f KalmanTransformFilter::update(const cv::Vec4f& measurement, float delta_time) {
    if (!initialized_) {
        initialize_from_measurement(measurement);
        return measurement;
    }

    configure_transition(delta_time);
    filter_.predict();
    cv::Mat measurement_matrix(4, 1, CV_32F);
    for (int i = 0; i < 4; ++i) {
        measurement_matrix.at<float>(i) = measurement[i];
    }
    const cv::Mat corrected = filter_.correct(measurement_matrix);
    return {corrected.at<float>(0), corrected.at<float>(1),
            corrected.at<float>(2), corrected.at<float>(3)};
}

cv::Vec4f KalmanTransformFilter::predict(float delta_time) {
    if (!initialized_) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    configure_transition(delta_time);
    const cv::Mat predicted = filter_.predict();
    return {predicted.at<float>(0), predicted.at<float>(1),
            predicted.at<float>(2), predicted.at<float>(3)};
}

void KalmanTransformFilter::reset() {
    initialized_ = false;
    filter_.statePost = cv::Mat::zeros(8, 1, CV_32F);
    cv::setIdentity(filter_.errorCovPost, cv::Scalar(config_.initial_error));
}

bool KalmanTransformFilter::is_initialized() const noexcept {
    return initialized_;
}
