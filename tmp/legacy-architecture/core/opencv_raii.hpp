/*
OBS Stabilizer Plugin - OpenCV RAII Wrappers
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#ifdef ENABLE_STABILIZATION
#include <opencv2/opencv.hpp>
#include <memory>

namespace obs_stabilizer {

// RAII wrapper for OpenCV Mat objects
class CVMatGuard {
public:
    explicit CVMatGuard(cv::Mat&& mat) : mat_(std::move(mat)) {}
    explicit CVMatGuard(const cv::Mat& mat) : mat_(mat.clone()) {}

    ~CVMatGuard() {
        if (!mat_.empty()) {
            mat_.release();
        }
    }

    // Non-copyable but movable
    CVMatGuard(const CVMatGuard&) = delete;
    CVMatGuard& operator=(const CVMatGuard&) = delete;

    CVMatGuard(CVMatGuard&& other) noexcept : mat_(std::move(other.mat_)) {}
    CVMatGuard& operator=(CVMatGuard&& other) noexcept {
        if (this != &other) {
            if (!mat_.empty()) {
                mat_.release();
            }
            mat_ = std::move(other.mat_);
        }
        return *this;
    }

    const cv::Mat& get() const { return mat_; }
    cv::Mat& get() { return mat_; }

    bool empty() const { return mat_.empty(); }

private:
    cv::Mat mat_;
};

// RAII wrapper for vector of Points
class CVPointsGuard {
public:
    explicit CVPointsGuard(std::vector<cv::Point2f>&& points)
        : points_(std::move(points)) {}

    ~CVPointsGuard() {
        points_.clear();
        points_.shrink_to_fit();
    }

    // Non-copyable but movable
    CVPointsGuard(const CVPointsGuard&) = delete;
    CVPointsGuard& operator=(const CVPointsGuard&) = delete;

    CVPointsGuard(CVPointsGuard&& other) noexcept : points_(std::move(other.points_)) {}
    CVPointsGuard& operator=(CVPointsGuard&& other) noexcept {
        if (this != &other) {
            points_ = std::move(other.points_);
        }
        return *this;
    }

    const std::vector<cv::Point2f>& get() const { return points_; }
    std::vector<cv::Point2f>& get() { return points_; }

    size_t size() const { return points_.size(); }
    bool empty() const { return points_.empty(); }

private:
    std::vector<cv::Point2f> points_;
};

// RAII factory functions
inline CVMatGuard make_mat_guard(cv::Mat&& mat) {
    return CVMatGuard(std::move(mat));
}

inline CVMatGuard make_mat_guard(const cv::Mat& mat) {
    return CVMatGuard(mat);
}

inline CVPointsGuard make_points_guard(std::vector<cv::Point2f>&& points) {
    return CVPointsGuard(std::move(points));
}

} // namespace obs_stabilizer

#endif // ENABLE_STABILIZATION