/*
OBS Stabilizer Plugin - Type-Safe Transform Matrix Implementation
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "transform_matrix.hpp"
#include "error_handler.hpp"
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <numeric>

namespace obs_stabilizer {

// PIMPL implementation for OpenCV integration
struct TransformMatrix::Impl {
#ifdef ENABLE_STABILIZATION
    cv::Mat opencv_matrix;
    bool has_opencv = true;
    
    Impl() : opencv_matrix(cv::Mat::eye(2, 3, CV_64F)), has_opencv(true) {}
    explicit Impl(const cv::Mat& mat) : opencv_matrix(mat.clone()), has_opencv(true) {}
#else
    bool has_opencv = false;
    Impl() : has_opencv(false) {}
#endif
};

TransformMatrix::TransformMatrix() 
    : pimpl_(std::make_unique<Impl>())
    , fallback_data_{{1.0, 0.0, 0.0, 1.0, 0.0, 0.0}} // Identity matrix
    , has_fallback_data_(true) {
    update_opencv_from_fallback();
}

TransformMatrix::TransformMatrix(const TransformMatrix& other)
    : pimpl_(std::make_unique<Impl>(*other.pimpl_))
    , fallback_data_(other.fallback_data_)
    , has_fallback_data_(other.has_fallback_data_) {
}

TransformMatrix& TransformMatrix::operator=(const TransformMatrix& other) {
    if (this != &other) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::lock_guard<std::mutex> other_lock(other.mutex_);
        *pimpl_ = *other.pimpl_;
        fallback_data_ = other.fallback_data_;
        has_fallback_data_ = other.has_fallback_data_;
        data_valid_.store(true);
    }
    return *this;
}

TransformMatrix::TransformMatrix(TransformMatrix&& other) noexcept
    : pimpl_(std::move(other.pimpl_))
    , fallback_data_(std::move(other.fallback_data_))
    , has_fallback_data_(other.has_fallback_data_) {
    other.has_fallback_data_ = false;
}

TransformMatrix& TransformMatrix::operator=(TransformMatrix&& other) noexcept {
    if (this != &other) {
        pimpl_ = std::move(other.pimpl_);
        fallback_data_ = std::move(other.fallback_data_);
        has_fallback_data_ = other.has_fallback_data_;
        other.has_fallback_data_ = false;
    }
    return *this;
}

TransformMatrix::~TransformMatrix() = default;

#ifdef ENABLE_STABILIZATION
TransformMatrix::TransformMatrix(const cv::Mat& opencv_matrix)
    : pimpl_(std::make_unique<Impl>(opencv_matrix))
    , has_fallback_data_(true) {
    update_fallback_from_opencv();
}

TransformMatrix& TransformMatrix::operator=(const cv::Mat& opencv_matrix) {
    pimpl_->opencv_matrix = opencv_matrix.clone();
    pimpl_->has_opencv = true;
    update_fallback_from_opencv();
    return *this;
}

cv::Mat TransformMatrix::to_opencv_mat() const {
    if (!pimpl_->has_opencv) {
        throw std::runtime_error("OpenCV not available - cannot convert to cv::Mat");
    }
    return pimpl_->opencv_matrix.clone();
}

const cv::Mat& TransformMatrix::get_opencv_ref() const {
    if (!pimpl_->has_opencv) {
        throw std::runtime_error("OpenCV not available - cannot get cv::Mat reference");
    }
    return pimpl_->opencv_matrix;
}

bool TransformMatrix::has_opencv_data() const {
    return pimpl_->has_opencv && !pimpl_->opencv_matrix.empty();
}
#endif

bool TransformMatrix::is_identity() const {
    if (!has_fallback_data_) return false;
    
    const double epsilon = 1e-9;
    return (std::abs(fallback_data_[0] - 1.0) < epsilon &&  // a
            std::abs(fallback_data_[1]) < epsilon &&         // b
            std::abs(fallback_data_[2]) < epsilon &&         // c  
            std::abs(fallback_data_[3] - 1.0) < epsilon &&  // d
            std::abs(fallback_data_[4]) < epsilon &&         // tx
            std::abs(fallback_data_[5]) < epsilon);          // ty
}

bool TransformMatrix::is_valid() const {
    if (!has_fallback_data_) return false;
    
    // Check for NaN or infinite values
    for (double val : fallback_data_) {
        if (std::isnan(val) || std::isinf(val)) {
            return false;
        }
    }
    return true;
}

bool TransformMatrix::is_empty() const {
#ifdef ENABLE_STABILIZATION
    if (pimpl_->has_opencv) {
        return pimpl_->opencv_matrix.empty();
    }
#endif
    return !has_fallback_data_;
}

double TransformMatrix::get_translation_x() const {
    return has_fallback_data_ ? fallback_data_[4] : 0.0;
}

double TransformMatrix::get_translation_y() const {
    return has_fallback_data_ ? fallback_data_[5] : 0.0;
}

double TransformMatrix::get_scale() const {
    if (!has_fallback_data_) return 1.0;
    
    // Calculate scale from transformation matrix
    double a = fallback_data_[0];
    double b = fallback_data_[1];
    return std::sqrt(a * a + b * b);
}

double TransformMatrix::get_rotation() const {
    if (!has_fallback_data_) return 0.0;
    
    // Calculate rotation from transformation matrix
    double a = fallback_data_[0];
    double b = fallback_data_[1];
    return std::atan2(b, a);
}

void TransformMatrix::set_translation(double dx, double dy) {
    if (!has_fallback_data_) reset_to_identity();
    
    fallback_data_[4] = dx;
    fallback_data_[5] = dy;
    update_opencv_from_fallback();
}

void TransformMatrix::set_scale(double scale) {
    if (!has_fallback_data_) reset_to_identity();
    
    double current_rotation = get_rotation();
    fallback_data_[0] = scale * std::cos(current_rotation);
    fallback_data_[1] = scale * std::sin(current_rotation);
    fallback_data_[2] = -scale * std::sin(current_rotation);
    fallback_data_[3] = scale * std::cos(current_rotation);
    update_opencv_from_fallback();
}

void TransformMatrix::set_rotation(double radians) {
    if (!has_fallback_data_) reset_to_identity();
    
    double current_scale = get_scale();
    fallback_data_[0] = current_scale * std::cos(radians);
    fallback_data_[1] = current_scale * std::sin(radians);
    fallback_data_[2] = -current_scale * std::sin(radians);
    fallback_data_[3] = current_scale * std::cos(radians);
    update_opencv_from_fallback();
}

void TransformMatrix::set_identity() {
    reset_to_identity();
    update_opencv_from_fallback();
}

void TransformMatrix::clear() {
    has_fallback_data_ = false;
#ifdef ENABLE_STABILIZATION
    if (pimpl_->has_opencv) {
        pimpl_->opencv_matrix = cv::Mat();
    }
#endif
}

TransformMatrix TransformMatrix::operator*(const TransformMatrix& other) const {
    TransformMatrix result = *this;
    result *= other;
    return result;
}

TransformMatrix& TransformMatrix::operator*=(const TransformMatrix& other) {
    if (!has_fallback_data_ || !other.has_fallback_data_) {
        clear();
        return *this;
    }
    
    // Matrix multiplication for 2x3 affine transformation
    // [a c tx]   [a' c' tx']   [a*a'+b*c'   a*c'+b*d'   a*tx'+b*ty'+tx]
    // [b d ty] * [b' d' ty'] = [b*a'+d*c'   b*c'+d*d'   b*tx'+d*ty'+ty]
    //                         
    double a = fallback_data_[0], b = fallback_data_[1];
    double c = fallback_data_[2], d = fallback_data_[3];
    double tx = fallback_data_[4], ty = fallback_data_[5];
    
    double a2 = other.fallback_data_[0], b2 = other.fallback_data_[1];
    double c2 = other.fallback_data_[2], d2 = other.fallback_data_[3];
    double tx2 = other.fallback_data_[4], ty2 = other.fallback_data_[5];
    
    fallback_data_[0] = a * a2 + b * c2;  // new a
    fallback_data_[1] = a * b2 + b * d2;  // new b
    fallback_data_[2] = c * a2 + d * c2;  // new c
    fallback_data_[3] = c * b2 + d * d2;  // new d
    fallback_data_[4] = a * tx2 + b * ty2 + tx;  // new tx
    fallback_data_[5] = c * tx2 + d * ty2 + ty;  // new ty
    
    update_opencv_from_fallback();
    return *this;
}

bool TransformMatrix::is_reasonable() const {
    if (!is_valid()) return false;
    
    double scale = get_scale();
    double tx = get_translation_x();
    double ty = get_translation_y();
    
    // Check for reasonable transform values
    return (scale >= 0.5 && scale <= 2.0 &&
            std::abs(tx) <= 100.0 &&
            std::abs(ty) <= 100.0);
}

std::string TransformMatrix::to_string() const {
    if (!has_fallback_data_) {
        return "TransformMatrix(empty)";
    }
    
    std::ostringstream oss;
    oss << "TransformMatrix([" 
        << fallback_data_[0] << ", " << fallback_data_[2] << ", " << fallback_data_[4] << "; "
        << fallback_data_[1] << ", " << fallback_data_[3] << ", " << fallback_data_[5] << "])";
    return oss.str();
}

std::array<double, 6> TransformMatrix::get_raw_data() const {
    return fallback_data_;
}

void TransformMatrix::set_raw_data(const std::array<double, 6>& data) {
    fallback_data_ = data;
    has_fallback_data_ = true;
    update_opencv_from_fallback();
}

// Private helper methods
void TransformMatrix::update_fallback_from_opencv() {
#ifdef ENABLE_STABILIZATION
    if (pimpl_->has_opencv && !pimpl_->opencv_matrix.empty() && 
        pimpl_->opencv_matrix.rows >= 2 && pimpl_->opencv_matrix.cols >= 3 &&
        pimpl_->opencv_matrix.type() == CV_64F) {
        
        bool extraction_success = ErrorHandler::safe_execute_bool([&]() -> bool {
            // Validate matrix bounds before access
            if (pimpl_->opencv_matrix.rows < 2 || pimpl_->opencv_matrix.cols < 3) {
                return false;
            }
            
            fallback_data_[0] = pimpl_->opencv_matrix.at<double>(0, 0); // a
            fallback_data_[1] = pimpl_->opencv_matrix.at<double>(1, 0); // b
            fallback_data_[2] = pimpl_->opencv_matrix.at<double>(0, 1); // c
            fallback_data_[3] = pimpl_->opencv_matrix.at<double>(1, 1); // d
            fallback_data_[4] = pimpl_->opencv_matrix.at<double>(0, 2); // tx
            fallback_data_[5] = pimpl_->opencv_matrix.at<double>(1, 2); // ty
            
            // Validate extracted values
            bool valid = true;
            for (double val : fallback_data_) {
                if (std::isnan(val) || std::isinf(val)) {
                    valid = false;
                    break;
                }
            }
            
            return valid;
        }, ErrorCategory::OPENCV_INTERNAL, "update_fallback_from_opencv");
        
        has_fallback_data_ = extraction_success;
    }
#endif
}

void TransformMatrix::update_opencv_from_fallback() {
#ifdef ENABLE_STABILIZATION
    if (pimpl_->has_opencv && has_fallback_data_) {
        pimpl_->opencv_matrix = cv::Mat::zeros(2, 3, CV_64F);
        pimpl_->opencv_matrix.at<double>(0, 0) = fallback_data_[0]; // a
        pimpl_->opencv_matrix.at<double>(1, 0) = fallback_data_[1]; // b
        pimpl_->opencv_matrix.at<double>(0, 1) = fallback_data_[2]; // c
        pimpl_->opencv_matrix.at<double>(1, 1) = fallback_data_[3]; // d
        pimpl_->opencv_matrix.at<double>(0, 2) = fallback_data_[4]; // tx
        pimpl_->opencv_matrix.at<double>(1, 2) = fallback_data_[5]; // ty
    }
#endif
}

void TransformMatrix::reset_to_identity() {
    fallback_data_ = {{1.0, 0.0, 0.0, 1.0, 0.0, 0.0}};
    has_fallback_data_ = true;
}

// Utility functions implementation
namespace transform_utils {

TransformMatrix create_translation(double dx, double dy) {
    TransformMatrix transform;
    transform.set_translation(dx, dy);
    return transform;
}

TransformMatrix create_scale(double scale) {
    TransformMatrix transform;
    transform.set_scale(scale);
    return transform;
}

TransformMatrix create_rotation(double radians) {
    TransformMatrix transform;
    transform.set_rotation(radians);
    return transform;
}

TransformMatrix create_identity() {
    return TransformMatrix();
}

bool is_transform_reasonable(const TransformMatrix& transform) {
    return transform.is_reasonable();
}

TransformMatrix interpolate(const TransformMatrix& a, const TransformMatrix& b, double t) {
    if (t <= 0.0) return a;
    if (t >= 1.0) return b;
    
    auto data_a = a.get_raw_data();
    auto data_b = b.get_raw_data();
    
    std::array<double, 6> result;
    for (size_t i = 0; i < 6; ++i) {
        result[i] = data_a[i] * (1.0 - t) + data_b[i] * t;
    }
    
    TransformMatrix interpolated;
    interpolated.set_raw_data(result);
    return interpolated;
}

TransformMatrix average_transforms(const std::vector<TransformMatrix>& transforms) {
    if (transforms.empty()) {
        return create_identity();
    }
    
    std::array<double, 6> average_data = {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};
    
    for (const auto& transform : transforms) {
        auto data = transform.get_raw_data();
        for (size_t i = 0; i < 6; ++i) {
            average_data[i] += data[i];
        }
    }
    
    double count = static_cast<double>(transforms.size());
    if (count > 0.0) {  // Division by zero protection
        for (double& val : average_data) {
            val /= count;
        }
    }
    
    TransformMatrix result;
    result.set_raw_data(average_data);
    return result;
}

} // namespace transform_utils

} // namespace obs_stabilizer