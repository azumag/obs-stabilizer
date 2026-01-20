#pragma once

#include <cstdint>

namespace StabilizerConstants {

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

} // namespace StabilizerConstants