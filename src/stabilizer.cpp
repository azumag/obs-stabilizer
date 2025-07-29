/*
OBS Stabilizer Plugin - Simplified Core Stabilization
Architectural simplification following Gemini review requirements
*/

#include "stabilizer.h"
#include <chrono>

void VideoStabilizer::detect_features(const cv::Mat& gray) {
    cv::goodFeaturesToTrack(gray, prev_features_, config_.max_features, 
                           config_.feature_quality, config_.min_distance);
    frames_since_detection_ = 0;
}

cv::Mat VideoStabilizer::track_features_and_compute_transform(const cv::Mat& current_gray) {
    if (prev_features_.empty()) {
        detect_features(current_gray);
        prev_gray_ = current_gray.clone();
        return cv::Mat::eye(2, 3, CV_64F);
    }

    // Track features using Lucas-Kanade
    std::vector<cv::Point2f> current_features;
    std::vector<uchar> status;
    std::vector<float> error;
    
    cv::calcOpticalFlowPyrLK(prev_gray_, current_gray, prev_features_, 
                            current_features, status, error);

    // Filter good matches
    std::vector<cv::Point2f> good_prev, good_current;
    for (size_t i = 0; i < status.size(); ++i) {
        if (status[i] && error[i] < 50.0) {
            good_prev.push_back(prev_features_[i]);
            good_current.push_back(current_features[i]);
        }
    }

    cv::Mat transform = cv::Mat::eye(2, 3, CV_64F);
    if (good_prev.size() >= 6) {
        transform = cv::estimateAffinePartial2D(good_current, good_prev);
        if (transform.empty()) {
            transform = cv::Mat::eye(2, 3, CV_64F);
        }
    }

    // Refresh features periodically
    frames_since_detection_++;
    if (frames_since_detection_ >= config_.detection_interval || good_prev.size() < 50) {
        detect_features(current_gray);
    } else {
        prev_features_ = current_features;
    }

    prev_gray_ = current_gray.clone();
    return transform;
}

cv::Mat VideoStabilizer::smooth_transform(const cv::Mat& transform) {
    // Add to history
    if (transform_history_.size() < static_cast<size_t>(config_.smoothing_radius)) {
        transform_history_.push_back(transform.clone());
    } else {
        transform_history_[history_index_] = transform.clone();
        history_index_ = (history_index_ + 1) % config_.smoothing_radius;
    }

    // Compute smoothed transform (simple average)
    cv::Mat smoothed = cv::Mat::zeros(2, 3, CV_64F);
    for (const auto& t : transform_history_) {
        smoothed += t;
    }
    smoothed /= static_cast<double>(transform_history_.size());
    return smoothed;
}

void VideoStabilizer::apply_transform_to_frame(struct obs_source_frame* frame, const cv::Mat& transform) {
    if (frame->format == VIDEO_FORMAT_NV12) {
        apply_nv12_transform(frame, transform);
    } else if (frame->format == VIDEO_FORMAT_I420) {
        apply_i420_transform(frame, transform);
    }
}

void VideoStabilizer::apply_nv12_transform(struct obs_source_frame* frame, const cv::Mat& transform) {
    // Transform Y plane
    cv::Mat y_plane(frame->height, frame->width, CV_8UC1, 
                   frame->data[0], frame->linesize[0]);
    cv::Mat y_warped;
    cv::warpAffine(y_plane, y_warped, transform, y_plane.size());
    y_warped.copyTo(y_plane);

    // Transform UV plane (half resolution)
    cv::Mat uv_plane(frame->height / 2, frame->width / 2, CV_8UC2,
                    frame->data[1], frame->linesize[1]);
    cv::Mat uv_warped;
    cv::Mat uv_transform = transform.clone();
    uv_transform.at<double>(0, 2) /= 2.0; // Scale translation
    uv_transform.at<double>(1, 2) /= 2.0;
    cv::warpAffine(uv_plane, uv_warped, uv_transform, uv_plane.size());
    uv_warped.copyTo(uv_plane);
}

void VideoStabilizer::apply_i420_transform(struct obs_source_frame* frame, const cv::Mat& transform) {
    // Transform Y plane
    cv::Mat y_plane(frame->height, frame->width, CV_8UC1,
                   frame->data[0], frame->linesize[0]);
    cv::Mat y_warped;
    cv::warpAffine(y_plane, y_warped, transform, y_plane.size());
    y_warped.copyTo(y_plane);

    // Transform U and V planes (quarter resolution)
    cv::Mat u_transform = transform.clone();
    u_transform.at<double>(0, 2) /= 2.0;
    u_transform.at<double>(1, 2) /= 2.0;

    cv::Mat u_plane(frame->height / 2, frame->width / 2, CV_8UC1,
                   frame->data[1], frame->linesize[1]);
    cv::Mat u_warped;
    cv::warpAffine(u_plane, u_warped, u_transform, u_plane.size());
    u_warped.copyTo(u_plane);

    cv::Mat v_plane(frame->height / 2, frame->width / 2, CV_8UC1,
                   frame->data[2], frame->linesize[2]);
    cv::Mat v_warped;
    cv::warpAffine(v_plane, v_warped, u_transform, v_plane.size());
    v_warped.copyTo(v_plane);
}

void VideoStabilizer::update_config(const StabilizerConfig& config) {
    config_ = config;
}

bool VideoStabilizer::process_frame(struct obs_source_frame* frame) {
    if (!config_.enable_stabilization || !frame) {
        return false;
    }

    try {
        // Extract grayscale from frame
        cv::Mat current_gray;
        if (frame->format == VIDEO_FORMAT_NV12) {
            cv::Mat y_plane(frame->height, frame->width, CV_8UC1,
                           frame->data[0], frame->linesize[0]);
            current_gray = y_plane.clone();
        } else if (frame->format == VIDEO_FORMAT_I420) {
            cv::Mat y_plane(frame->height, frame->width, CV_8UC1,
                           frame->data[0], frame->linesize[0]);
            current_gray = y_plane.clone();
        } else {
            return false; // Unsupported format
        }

        // Compute and smooth transform
        cv::Mat raw_transform = track_features_and_compute_transform(current_gray);
        cv::Mat smoothed_transform = smooth_transform(raw_transform);

        // Apply transform to frame
        apply_transform_to_frame(frame, smoothed_transform);
        
        initialized_ = true;
        return true;

    } catch (const cv::Exception& e) {
        obs_log(LOG_WARNING, "OpenCV error in stabilization: %s", e.what());
        return false;
    } catch (...) {
        obs_log(LOG_ERROR, "Unknown error in stabilization");
        return false;
    }
}

void VideoStabilizer::reset() {
    prev_features_.clear();
    transform_history_.clear();
    prev_gray_.release();
    history_index_ = 0;
    frames_since_detection_ = 0;
    initialized_ = false;
}