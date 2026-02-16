/*
 * OBS Stabilizer Plugin - StabilizerWrapper Implementation
 * Thread-safe RAII wrapper for StabilizerCore
 *
 * RATIONALE: Thread safety is implemented here (not in StabilizerCore) because:
 * 1. OBS UI thread can update properties concurrently with video thread processing
 * 2. StabilizerCore is kept simple for performance (single-threaded design)
 * 3. Wrapper provides clean separation: thread safety vs video processing
 *
 * Mutex locking strategy:
 * - Lock for all public methods to prevent data races
 * - Lock duration is minimized to avoid contention
 * - Const methods use mutable mutex for proper const-correctness
 */

#include "stabilizer_wrapper.hpp"
#include <exception>

StabilizerWrapper::StabilizerWrapper() = default;
StabilizerWrapper::~StabilizerWrapper() = default;

bool StabilizerWrapper::initialize(uint32_t width, uint32_t height, const StabilizerCore::StabilizerParams& params) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        stabilizer.reset();
        stabilizer.reset(new StabilizerCore());
        if (!stabilizer->initialize(width, height, params)) {
            stabilizer.reset();
            return false;
        }
        return true;
    } catch (const std::exception&) {
        stabilizer.reset();
        return false;
    }
}

cv::Mat StabilizerWrapper::process_frame(cv::Mat frame) {
    // Lock is acquired before accessing stabilizer to prevent concurrent modifications
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        if (!stabilizer) {
            return frame;
        }
        return stabilizer->process_frame(frame);
    } catch (const std::exception&) {
        return frame.clone();
    }
}

bool StabilizerWrapper::is_initialized() {
    std::lock_guard<std::mutex> lock(mutex_);
    return stabilizer != nullptr;
}

std::string StabilizerWrapper::get_last_error() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stabilizer) {
        return stabilizer->get_last_error();
    }
    return "Not initialized";
}

StabilizerCore::PerformanceMetrics StabilizerWrapper::get_performance_metrics() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stabilizer) {
        return stabilizer->get_performance_metrics();
    }
    return {};
}

void StabilizerWrapper::update_parameters(const StabilizerCore::StabilizerParams& params) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stabilizer) {
        stabilizer->update_parameters(params);
    }
}

StabilizerCore::StabilizerParams StabilizerWrapper::get_current_params() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stabilizer) {
        return stabilizer->get_current_params();
    }
    return {};
}

void StabilizerWrapper::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stabilizer) {
        stabilizer->reset();
    }
}

bool StabilizerWrapper::is_ready() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stabilizer) {
        return stabilizer->is_ready();
    }
    return false;
}
