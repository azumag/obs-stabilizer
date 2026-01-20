#pragma once

#include <cstdint>

namespace TestConstants {

// Standard Video Resolutions
namespace Resolution {
    constexpr int VGA_WIDTH = 640;
    constexpr int VGA_HEIGHT = 480;
    constexpr int QVGA_WIDTH = 320;
    constexpr int QVGA_HEIGHT = 240;
    constexpr int HD_WIDTH = 1920;
    constexpr int HD_HEIGHT = 1080;
    constexpr int SD_WIDTH = 854;
    constexpr int SD_HEIGHT = 480;
    constexpr int HD720_WIDTH = 1280;
    constexpr int HD720_HEIGHT = 720;
    constexpr int HD360_WIDTH = 640;
    constexpr int HD360_HEIGHT = 360;
}

// Test Sequence Lengths
namespace FrameCount {
    constexpr int SHORT_SEQUENCE = 10;
    constexpr int STANDARD_SEQUENCE = 20;
    constexpr int LONG_SEQUENCE = 100;
    constexpr int VERY_LONG_SEQUENCE = 200;
}

// Motion Parameters
namespace Motion {
    constexpr int SMALL_MOTION = 20;
    constexpr int MEDIUM_MOTION = 30;
    constexpr int LARGE_MOTION = 50;
    constexpr float SMALL_ROTATION = 3.0f;
    constexpr float MEDIUM_ROTATION = 5.0f;
    constexpr float LARGE_ROTATION = 30.0f;
    constexpr float DEFAULT_ZOOM = 1.2f;
}

// Feature Detection Parameters
namespace Features {
    constexpr int MIN_COUNT = 10;
    constexpr int LOW_COUNT = 50;
    constexpr int DEFAULT_COUNT = 100;
    constexpr int HIGH_COUNT = 200;
    constexpr int MAX_COUNT = 1000;
}

// Thresholds and Limits
namespace Thresholds {
    constexpr int MIN_FRAME_SIZE = 10;
    constexpr int MAX_FRAME_SIZE = 10000;
    constexpr int DEFAULT_PATTERN_OFFSET = 0;
    constexpr int HIGH_PATTERN_OFFSET = 3;
}

// Processing Parameters
namespace Processing {
    constexpr int SMALL_SMOOTHING_WINDOW = 10;
    constexpr int MEDIUM_SMOOTHING_WINDOW = 30;
    constexpr int LARGE_SMOOTHING_WINDOW = 50;
    constexpr float DEFAULT_QUALITY_LEVEL = 0.01f;
    constexpr float DEFAULT_MIN_DISTANCE = 7.0f;
}

} // namespace TestConstants