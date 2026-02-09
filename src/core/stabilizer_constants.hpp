#pragma once

#include <cstdint>

namespace StabilizerConstants {

// Image size constraints
constexpr int MIN_IMAGE_SIZE = 32;
constexpr int MAX_IMAGE_WIDTH = 7680;
constexpr int MAX_IMAGE_HEIGHT = 4320;

// Smoothing Parameters
namespace Smoothing {
    constexpr int MIN_RADIUS = 1;
    constexpr int MAX_RADIUS = 200;
    constexpr int DEFAULT_RADIUS = 10;
    constexpr int GAMING_RADIUS = 25;
    constexpr int STREAMING_RADIUS = 30;
    constexpr int RECORDING_RADIUS = 50;
}

// Correction Parameters
namespace Correction {
    constexpr float MIN_MAX = 0.0f;
    constexpr float MAX_MAX = 100.0f;
    constexpr float DEFAULT_MAX = 20.0f;
    constexpr float GAMING_MAX = 40.0f;
    constexpr float STREAMING_MAX = 30.0f;
    constexpr float RECORDING_MAX = 20.0f;
}

// Feature Detection Parameters
namespace Features {
    constexpr int MIN_COUNT = 50;
    constexpr int MAX_COUNT = 2000;
    constexpr int DEFAULT_COUNT = 200;
    constexpr int GAMING_COUNT = 150;
    constexpr int RECORDING_COUNT = 400;
}

// Quality Parameters
namespace Quality {
    constexpr float MIN_LEVEL = 0.001f;
    constexpr float MAX_LEVEL = 0.1f;
    constexpr float DEFAULT_LEVEL = 0.01f;
    constexpr float GAMING_LEVEL = 0.015f;
    constexpr float RECORDING_LEVEL = 0.005f;
}

// Distance Parameters
namespace Distance {
    constexpr float MIN = 1.0f;
    constexpr float MAX = 200.0f;
    constexpr float DEFAULT = 10.0f;
    constexpr float GAMING = 25.0f;
    constexpr float RECORDING = 20.0f;
}

// Block Size Parameters
namespace Block {
    constexpr int MIN_SIZE = 3;
    constexpr int MAX_SIZE = 31;
    constexpr int DEFAULT_SIZE = 3;
}

// Harris Corner Detection Parameters
namespace Harris {
    constexpr float MIN_K = 0.01f;
    constexpr float MAX_K = 0.1f;
    constexpr float DEFAULT_K = 0.04f;
}

// Optical Flow Parameters
namespace OpticalFlow {
    constexpr int MIN_PYRAMID_LEVELS = 2;
    constexpr int MAX_PYRAMID_LEVELS = 5;
    constexpr int DEFAULT_PYRAMID_LEVELS = 3;
    constexpr int RECORDING_PYRAMID_LEVELS = 4;

    constexpr int MIN_WINDOW_SIZE = 5;
    constexpr int MAX_WINDOW_SIZE = 31;
    constexpr int DEFAULT_WINDOW_SIZE = 21;
    constexpr int RECORDING_WINDOW_SIZE = 31;

    constexpr int MAX_ITERATIONS = 30;
    constexpr float EPSILON = 0.01f;
}

// Adaptive Feature Ranges for Presets
namespace AdaptiveFeatures {
    // Gaming preset
    constexpr int GAMING_MIN = 100;
    constexpr int GAMING_MAX = 400;

    // Streaming preset
    constexpr int STREAMING_MIN = 150;
    constexpr int STREAMING_MAX = 500;

    // Recording preset
    constexpr int RECORDING_MIN = 200;
    constexpr int RECORDING_MAX = 800;

    // Maximum feature count for adaptive stabilization (performance optimized range)
    constexpr int MAX_ADAPTIVE_FEATURES = 500;

    // Feature refresh thresholds for presets
    constexpr float GAMING_REFRESH = 0.6f;
    constexpr float STREAMING_REFRESH = 0.5f;
    constexpr float RECORDING_REFRESH = 0.4f;
}

// Content Detection Parameters
namespace ContentDetection {
    // Threshold for determining if a pixel contains content (non-black)
    // Values below this are considered black border pixels
    constexpr int CONTENT_THRESHOLD = 10;
    // Maximum border search region in pixels
    constexpr int BORDER_SEARCH_MAX = 100;
}

} // namespace StabilizerConstants