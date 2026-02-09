/*
 * OBS Stabilizer Plugin - StabilizerWrapper Implementation
 * RAII wrapper for StabilizerCore without thread safety
 * OBS filters are single-threaded, so mutex is unnecessary
 */

#include "stabilizer_wrapper.hpp"
#include <exception>

StabilizerWrapper::StabilizerWrapper() = default;
StabilizerWrapper::~StabilizerWrapper() = default;

bool StabilizerWrapper::initialize(uint32_t width, uint32_t height, const StabilizerCore::StabilizerParams& params) {
    try {
        stabilizer.reset();
        stabilizer = std::make_unique<StabilizerCore>();
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
    return stabilizer != nullptr;
}

void StabilizerWrapper::clear_state() {
    try {
        if (stabilizer) {
            stabilizer->reset();
        }
    } catch (const std::exception&) {
    }
}

std::string StabilizerWrapper::get_last_error() {
    if (stabilizer) {
        return stabilizer->get_last_error();
    }
    return "Not initialized";
}

StabilizerCore::PerformanceMetrics StabilizerWrapper::get_performance_metrics() {
    if (stabilizer) {
        return stabilizer->get_performance_metrics();
    }
    return {};
}

void StabilizerWrapper::update_parameters(const StabilizerCore::StabilizerParams& params) {
    if (stabilizer) {
        stabilizer->update_parameters(params);
    }
}

StabilizerCore::StabilizerParams StabilizerWrapper::get_current_params() {
    if (stabilizer) {
        return stabilizer->get_current_params();
    }
    return {};
}

void StabilizerWrapper::reset() {
    if (stabilizer) {
        stabilizer->reset();
    }
}

bool StabilizerWrapper::is_ready() {
    if (stabilizer) {
        return stabilizer->is_ready();
    }
    return false;
}
