/*
OBS Stabilizer Plugin - Simplified Core Stabilization Header
*/

#pragma once

#include <obs-module.h>
#include <opencv2/opencv.hpp>
#include <vector>

// Simple configuration structure
struct StabilizerConfig {
    bool enable_stabilization = true;
    int smoothing_radius = 30;
    int max_features = 200;
    double feature_quality = 0.01;
    double min_distance = 30.0;
    int detection_interval = 10;
};

// Simplified stabilizer implementation
class VideoStabilizer {
private:
    StabilizerConfig config_;
    std::vector<cv::Point2f> prev_features_;
    std::vector<cv::Mat> transform_history_;
    cv::Mat prev_gray_;
    int history_index_ = 0;
    int frames_since_detection_ = 0;
    bool initialized_ = false;

    void detect_features(const cv::Mat& gray);
    cv::Mat track_features_and_compute_transform(const cv::Mat& current_gray);
    cv::Mat smooth_transform(const cv::Mat& transform);
    void apply_transform_to_frame(struct obs_source_frame* frame, const cv::Mat& transform);
    void apply_nv12_transform(struct obs_source_frame* frame, const cv::Mat& transform);
    void apply_i420_transform(struct obs_source_frame* frame, const cv::Mat& transform);

public:
    void update_config(const StabilizerConfig& config);
    bool process_frame(struct obs_source_frame* frame);
    void reset();
};