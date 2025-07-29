/*
OBS Stabilizer Plugin - Type-Safe Transform Matrix Wrapper
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include <memory>
#include <array>
#include <vector>
#include <atomic>
#include <mutex>

#ifdef ENABLE_STABILIZATION
#include <opencv2/opencv.hpp>
#endif

namespace obs_stabilizer {

// Type-safe transform matrix wrapper that handles OpenCV/stub mode compatibility
//
// THREAD SAFETY NOTES:
// - This class provides thread-safe read operations using atomic operations
// - Write operations are protected by internal mutex for safety
// - Multiple readers can access simultaneously without blocking
// - The fallback_data_ array provides lock-free access to basic transform info
class TransformMatrix {
public:
    // Default constructor - creates identity transform
    TransformMatrix();

    // Copy constructor and assignment
    TransformMatrix(const TransformMatrix& other);
    TransformMatrix& operator=(const TransformMatrix& other);

    // Move constructor and assignment
    TransformMatrix(TransformMatrix&& other) noexcept;
    TransformMatrix& operator=(TransformMatrix&& other) noexcept;

    // Destructor
    ~TransformMatrix();

#ifdef ENABLE_STABILIZATION
    // OpenCV integration constructors
    explicit TransformMatrix(const cv::Mat& opencv_matrix);
    TransformMatrix& operator=(const cv::Mat& opencv_matrix);

    // Get OpenCV matrix (throws in stub mode)
    cv::Mat to_opencv_mat() const;
    const cv::Mat& get_opencv_ref() const;

    // Check if the internal matrix is valid OpenCV matrix
    bool has_opencv_data() const;
#endif

    // Common interface that works in both OpenCV and stub modes
    bool is_identity() const;
    bool is_valid() const;
    bool is_empty() const;

    // Get transform components (works in both modes)
    double get_translation_x() const;
    double get_translation_y() const;
    double get_scale() const;
    double get_rotation() const; // in radians

    // Set transform components (works in both modes)
    void set_translation(double dx, double dy);
    void set_scale(double scale);
    void set_rotation(double radians);
    void set_identity();
    void clear();

    // Matrix operations
    TransformMatrix operator*(const TransformMatrix& other) const;
    TransformMatrix& operator*=(const TransformMatrix& other);

    // Validation
    bool is_reasonable() const; // Check for reasonable transform values

    // Serialization for debugging/logging
    std::string to_string() const;

    // Raw data access (for advanced use cases)
    std::array<double, 6> get_raw_data() const; // [a, b, c, d, tx, ty] for 2x3 affine
    void set_raw_data(const std::array<double, 6>& data);

private:
    // Implementation details hidden from header
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    // Thread safety
    mutable std::mutex mutex_;
    mutable std::atomic<bool> data_valid_{true};

    // Fallback data for stub mode (when OpenCV unavailable)
    std::array<double, 6> fallback_data_; // 2x3 affine matrix: [a, b, c, d, tx, ty]
    bool has_fallback_data_;

    // Internal helpers
    void update_fallback_from_opencv();
    void update_opencv_from_fallback();
    void reset_to_identity();
};

// Utility functions for transform operations
namespace transform_utils {
    // Create common transforms
    TransformMatrix create_translation(double dx, double dy);
    TransformMatrix create_scale(double scale);
    TransformMatrix create_rotation(double radians);
    TransformMatrix create_identity();

    // Transform validation
    bool is_transform_reasonable(const TransformMatrix& transform);

    // Interpolation
    TransformMatrix interpolate(const TransformMatrix& a, const TransformMatrix& b, double t);

    // Average multiple transforms
    TransformMatrix average_transforms(const std::vector<TransformMatrix>& transforms);
}

} // namespace obs_stabilizer