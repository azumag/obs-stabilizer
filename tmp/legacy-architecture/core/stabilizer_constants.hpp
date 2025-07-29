/*
OBS Stabilizer Plugin - Centralized Constants
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include <cstdint>

namespace obs_stabilizer {
namespace StabilizerConstants {

// Frame dimension limits
constexpr uint32_t MAX_FRAME_WIDTH = 8192;
constexpr uint32_t MAX_FRAME_HEIGHT = 8192;
constexpr uint32_t MIN_FRAME_WIDTH = 32;
constexpr uint32_t MIN_FRAME_HEIGHT = 32;
constexpr uint32_t MIN_FEATURE_DETECTION_SIZE = 50;  // Minimum size for reliable feature detection

// Feature detection limits
constexpr int MIN_FEATURES_REQUIRED = 50;
constexpr int MIN_FEATURES_RELIABLE = 10;
constexpr int MAX_FEATURES_DEFAULT = 2000;
constexpr double MIN_FEATURE_QUALITY = 0.001;
constexpr double MAX_FEATURE_QUALITY = 0.1;
constexpr double DEFAULT_FEATURE_QUALITY = 0.01;

// Transform validation limits
constexpr double MAX_TRANSLATION = 100.0;
constexpr double MAX_TRANSLATION_WARN = 200.0;
constexpr double MIN_SCALE_FACTOR = 0.1;
constexpr double MAX_SCALE_FACTOR = 3.0;
constexpr double IDENTITY_TOLERANCE = 1e-6;

// Processing parameters
constexpr int DEFAULT_SMOOTHING_RADIUS = 30;
constexpr int MIN_SMOOTHING_RADIUS = 10;
constexpr int MAX_SMOOTHING_RADIUS = 100;
constexpr int DEFAULT_REFRESH_THRESHOLD = 30;

// Error handling thresholds
constexpr int MAX_CONSECUTIVE_FAILURES = 10;
constexpr int ERROR_RECOVERY_THRESHOLD = 5;
constexpr double DEFAULT_ERROR_THRESHOLD = 30.0;

// UI preset configurations
namespace Presets {
    // Gaming preset (fast response)
    constexpr int GAMING_FEATURES = 150;
    constexpr int GAMING_SMOOTHING = 15;
    constexpr double GAMING_THRESHOLD = 40.0;

    // Streaming preset (balanced)
    constexpr int STREAMING_FEATURES = 200;
    constexpr int STREAMING_SMOOTHING = 30;
    constexpr double STREAMING_THRESHOLD = 30.0;

    // Recording preset (high quality)
    constexpr int RECORDING_FEATURES = 400;
    constexpr int RECORDING_SMOOTHING = 50;
    constexpr double RECORDING_THRESHOLD = 20.0;
}

// UI parameter ranges
namespace UIRanges {
    constexpr int MIN_UI_FEATURES = 100;
    constexpr int MAX_UI_FEATURES = 1000;
    constexpr int MIN_UI_SMOOTHING = 10;
    constexpr int MAX_UI_SMOOTHING = 100;
    constexpr double MIN_UI_THRESHOLD = 10.0;
    constexpr double MAX_UI_THRESHOLD = 100.0;
}

// Memory management
constexpr size_t STACK_BUFFER_SIZE = 512;
constexpr size_t MAX_LOG_MESSAGE_SIZE = 1024;
constexpr size_t TRANSFORM_HISTORY_RESERVE = 100;

// Video format constants (from OBS)
enum class VideoFormat : uint32_t {
    I420 = 3,
    NV12 = 4
};

// Legacy constants for compatibility
constexpr uint32_t VIDEO_FORMAT_I420 = static_cast<uint32_t>(VideoFormat::I420);
constexpr uint32_t VIDEO_FORMAT_NV12 = static_cast<uint32_t>(VideoFormat::NV12);


} // namespace StabilizerConstants
} // namespace obs_stabilizer