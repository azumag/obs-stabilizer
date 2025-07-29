/*
OBS Stabilizer Plugin - UI Constants
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

namespace obs_stabilizer {
namespace UIConstants {

// UI String constants for DRY compliance
namespace Strings {
    constexpr const char* FILTER_NAME = "Video Stabilizer";
    constexpr const char* ENABLE_STABILIZATION_TEXT = "Enable Video Stabilization";
    constexpr const char* STABILIZATION_PRESET = "Stabilization Preset";
    constexpr const char* CUSTOM = "Custom";
    constexpr const char* GAMING_PRESET = "Gaming (Fast Response)";
    constexpr const char* STREAMING_PRESET = "Streaming (Balanced)";
    constexpr const char* RECORDING_PRESET = "Recording (High Quality)";
    constexpr const char* PRESET_DESCRIPTION = "Choose a preset optimized for your use case, or select Custom for manual configuration.";
    constexpr const char* SMOOTHING_STRENGTH = "Smoothing Strength";
    constexpr const char* SMOOTHING_DESCRIPTION = "Number of frames used for transform smoothing. Higher values = smoother but more latency.";
    constexpr const char* FEATURE_POINTS = "Feature Points";
    constexpr const char* FEATURE_POINTS_DESCRIPTION = "Maximum number of feature points to track. Higher values = more accurate but slower.";
    constexpr const char* STABILITY_THRESHOLD = "Stability Threshold";
    constexpr const char* STABILITY_DESCRIPTION = "Error threshold for tracking quality. Lower values = stricter quality requirements.";
    constexpr const char* EDGE_HANDLING = "Edge Handling";
    constexpr const char* CROP_BORDERS = "Crop Borders";
    constexpr const char* BLACK_PADDING = "Black Padding";
    constexpr const char* SCALE_TO_FIT = "Scale to Fit";
    constexpr const char* EDGE_DESCRIPTION = "How to handle stabilization borders: Crop removes edges, Padding adds black borders, Scale stretches to fit.";
    constexpr const char* FEATURE_QUALITY = "Feature Quality";
    constexpr const char* FEATURE_QUALITY_DESCRIPTION = "Minimum quality threshold for feature detection. Lower values detect more features but may be less stable.";
    constexpr const char* REFRESH_THRESHOLD = "Refresh Threshold";
    constexpr const char* REFRESH_DESCRIPTION = "Number of frames before refreshing feature detection. Lower values = more responsive but higher CPU usage.";
    constexpr const char* ADAPTIVE_REFRESH = "Adaptive Refresh";
    constexpr const char* ADAPTIVE_DESCRIPTION = "Automatically adjust refresh rate based on tracking quality.";
    constexpr const char* GPU_ACCELERATION = "GPU Acceleration (Experimental)";
    constexpr const char* GPU_DESCRIPTION = "Enable GPU acceleration for stabilization processing. May not be available on all systems.";
    constexpr const char* PROCESSING_THREADS = "Processing Threads";
    constexpr const char* THREADS_DESCRIPTION = "Number of threads to use for stabilization processing. Higher values may improve performance on multi-core systems.";
}

// Error message constants for internationalization support
namespace ErrorMessages {
    constexpr const char* INVALID_FRAME_DIMENSIONS = "Invalid frame dimensions (rows: %d, cols: %d) - possible frame acquisition failure";
}

} // namespace UIConstants
} // namespace obs_stabilizer