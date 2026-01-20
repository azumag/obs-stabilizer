/*
OBS Stabilizer Plugin - Constants and Configuration
Defines all magic numbers and their documented purposes
*/

#pragma once

#include <stdint.h>
#include <stddef.h>

// Performance targets from ARCHITECTURE.md
namespace PERFORMANCE_TARGETS {
    constexpr double TARGET_720P_MS = 2.0;      // <2ms/frame for 60fps+ support
    constexpr double TARGET_1080P_MS = 4.0;     // <4ms/frame for 30fps+ support
    constexpr double TARGET_1440P_MS = 8.0;     // <8ms/frame
    constexpr double TARGET_4K_MS = 15.0;        // <15ms/frame
}

// Parameter ranges from ARCHITECTURE.md
namespace PARAM_RANGES {
    // Smoothing radius: 10-100 frames
    constexpr int SMOOTHING_MIN = 10;
    constexpr int SMOOTHING_MAX = 100;
    constexpr int SMOOTHING_DEFAULT = 30;
    
    // Max correction: 10.0-100.0%
    constexpr float CORRECTION_MIN = 10.0f;
    constexpr float CORRECTION_MAX = 100.0f;
    constexpr float CORRECTION_DEFAULT = 50.0f;
    
    // Feature count: 100-1000 points
    constexpr int FEATURES_MIN = 100;
    constexpr int FEATURES_MAX = 1000;
    constexpr int FEATURES_DEFAULT = 200;
    
    // Quality level: 0.001-0.1
    constexpr float QUALITY_MIN = 0.001f;
    constexpr float QUALITY_MAX = 0.1f;
    constexpr float QUALITY_DEFAULT = 0.01f;
    
    // Min distance: 10-100 pixels
    constexpr float DISTANCE_MIN = 10.0f;
    constexpr float DISTANCE_MAX = 100.0f;
    constexpr float DISTANCE_DEFAULT = 30.0f;
}

// OpenCV algorithm constants
namespace OPENCV_PARAMS {
    // Feature detection
    constexpr int BLOCK_SIZE_DEFAULT = 3;           // Recommended for derivative covariation matrix
    constexpr bool USE_HARRIS_DEFAULT = false;     // Use cornerMinEigenVal by default
    constexpr float HARRIS_K_DEFAULT = 0.04f;       // Free parameter for Harris detector
    
    // Optical flow tracking
    constexpr int WIN_SIZE_DEFAULT = 30;            // Search window size at each pyramid level
    constexpr int MAX_LEVEL_DEFAULT = 3;            // 0-based maximal pyramid level number
    constexpr int MAX_COUNT_DEFAULT = 30;             // Maximum iterations for LK algorithm
    constexpr float EPSILON_DEFAULT = 0.01f;        // Desired accuracy for convergence
    constexpr float MIN_EIG_THRESHOLD_DEFAULT = 0.0001f; // Minimal eigen threshold
    
    // Feature refresh thresholds
    constexpr int REFRESH_FEATURE_THRESHOLD_DIVISOR = 2;  // Refresh when < features/2
    constexpr int REFRESH_FRAME_INTERVAL = 10;              // Refresh every N frames
    constexpr int MIN_FEATURES_AFTER_REFRESH = 50;          // Minimum features after tracking
    
    // Transform calculation
    constexpr int MIN_FEATURES_FOR_TRANSFORM = 4;  // Minimum 4 point pairs for perspective transform (homography)
}

// Preset configurations
namespace PRESETS {
    // Gaming preset - optimized for low latency
    namespace GAMING {
        constexpr int SMOOTHING_RADIUS = 15;
        constexpr float MAX_CORRECTION = 30.0f;
        constexpr int FEATURE_COUNT = 300;
        constexpr float QUALITY_LEVEL = 0.005f;
        constexpr float MIN_DISTANCE = 20.0f;
    }
    
    // Streaming preset - balanced performance
    namespace STREAMING {
        constexpr int SMOOTHING_RADIUS = 30;
        constexpr float MAX_CORRECTION = 50.0f;
        constexpr int FEATURE_COUNT = 200;
        constexpr float QUALITY_LEVEL = 0.01f;
        constexpr float MIN_DISTANCE = 30.0f;
    }
    
    // Recording preset - optimized for quality
    namespace RECORDING {
        constexpr int SMOOTHING_RADIUS = 60;
        constexpr float MAX_CORRECTION = 80.0f;
        constexpr int FEATURE_COUNT = 150;
        constexpr float QUALITY_LEVEL = 0.02f;
        constexpr float MIN_DISTANCE = 40.0f;
    }
}

// Safety and validation constants
namespace SAFETY {
    constexpr int MIN_SMOOTHING_OVERRIDE = 5;      // Absolute minimum allowed
    constexpr int MAX_SMOOTHING_OVERRIDE = 100;     // Absolute maximum allowed
    constexpr float MIN_CORRECTION_OVERRIDE = 10.0f;  // Absolute minimum allowed
    constexpr float MAX_CORRECTION_OVERRIDE = 100.0f; // Absolute maximum allowed
    constexpr int MIN_FEATURES_OVERRIDE = 50;       // Absolute minimum allowed
    constexpr int MAX_FEATURES_OVERRIDE = 500;       // Absolute maximum allowed
    constexpr float MIN_QUALITY_OVERRIDE = 0.001f;  // Absolute minimum allowed
    constexpr float MAX_QUALITY_OVERRIDE = 0.1f;     // Absolute maximum allowed
    constexpr float MIN_DISTANCE_OVERRIDE = 10.0f;   // Absolute minimum allowed
    constexpr float MAX_DISTANCE_OVERRIDE = 100.0f;  // Absolute maximum allowed
    constexpr int EVEN_NUMBER_VALIDATION_THRESHOLD = 2;  // Used for odd number validation
}

// Memory and frame size limits
namespace MEMORY {
    constexpr size_t MAX_FRAME_SIZE = 3840 * 2160 * 4; // 4K RGBA maximum safe size
    constexpr int MAX_TRANSFORM_HISTORY = 100;          // Maximum frames to store in history
    constexpr int DEBUG_OUTPUT_INTERVAL = 30;           // Log debug info every N frames
}

// Magic numbers replaced with named constants for maintainability
namespace MAGIC_NUMBERS {
    // Frame buffer management
    constexpr int DATA_PLANES_COUNT = 8;              // Number of data planes in OBS frame
    constexpr size_t MEMORY_GROWTH_FACTOR = 2;        // Buffer shrink threshold multiplier
    
    // Performance monitoring
    constexpr int DEBUG_OUTPUT_INTERVAL = 30;          // Log debug info every N frames
    
    // Algorithm parameters
    constexpr int DEFAULT_SEARCH_WINDOW = 30;          // LK optical flow search window
    constexpr int MAX_PYRAMID_LEVELS = 3;              // Maximum pyramid levels for optical flow
    constexpr double CONVERGENCE_EPSILON = 0.01;       // LK algorithm convergence threshold
    
    // Feature detection
    constexpr int MIN_FEATURES_THRESHOLD = 4;          // Minimum features for transform calculation
}

// Video format constants
namespace VIDEO_FORMATS {
    constexpr uint32_t FORMAT_BGRA = 0;  // BGRA format value
    constexpr uint32_t FORMAT_NV12 = 1;   // NV12 format value
    constexpr uint32_t FORMAT_I420 = 2;   // I420 format value
}

// Centralized parameter validation
namespace VALIDATION {
    // Type-safe validation template
    template<typename T>
    T clamp_value(T value, T min_val, T max_val) {
        return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
    }
    

    
    // Parameter range definitions
    namespace RANGES {
        constexpr int SMOOTHING_MIN = PARAM_RANGES::SMOOTHING_MIN;      // Consistent with PARAM_RANGES
        constexpr int SMOOTHING_MAX = PARAM_RANGES::SMOOTHING_MAX;     // Consistent with PARAM_RANGES
        constexpr float CORRECTION_MIN = SAFETY::MIN_CORRECTION_OVERRIDE;  // Absolute minimum
        constexpr float CORRECTION_MAX = SAFETY::MAX_CORRECTION_OVERRIDE; // Absolute maximum
        constexpr int FEATURES_MIN = PARAM_RANGES::FEATURES_MIN;       // Consistent with PARAM_RANGES
        constexpr int FEATURES_MAX = PARAM_RANGES::FEATURES_MAX;      // Consistent with PARAM_RANGES
        constexpr float QUALITY_MIN = PARAM_RANGES::QUALITY_MIN;   // Consistent with PARAM_RANGES
        constexpr float QUALITY_MAX = PARAM_RANGES::QUALITY_MAX;    // Consistent with PARAM_RANGES
        constexpr float DISTANCE_MIN = PARAM_RANGES::DISTANCE_MIN;  // Consistent with PARAM_RANGES
        constexpr float DISTANCE_MAX = PARAM_RANGES::DISTANCE_MAX; // Consistent with PARAM_RANGES
        constexpr int BLOCK_SIZE_MIN = OPENCV_PARAMS::BLOCK_SIZE_DEFAULT;      // Consistent with OPENCV_PARAMS
        constexpr int BLOCK_SIZE_MAX = 31;     // Safe upper limit
        constexpr float K_MIN = OPENCV_PARAMS::HARRIS_K_DEFAULT;         // Consistent with OPENCV_PARAMS
        constexpr float K_MAX = 0.1f;          // Safe upper limit
    }
    
    // Complete parameter validation function
    template<typename Params>
    Params validate_parameters(const Params& params) {
        Params validated = params;
        
        validated.smoothing_radius = clamp_value(validated.smoothing_radius, 
                                                 RANGES::SMOOTHING_MIN, RANGES::SMOOTHING_MAX);
        validated.max_correction = clamp_value(validated.max_correction, 
                                               RANGES::CORRECTION_MIN, RANGES::CORRECTION_MAX);
        validated.feature_count = clamp_value(validated.feature_count, 
                                             RANGES::FEATURES_MIN, RANGES::FEATURES_MAX);
        validated.quality_level = clamp_value(validated.quality_level, 
                                              RANGES::QUALITY_MIN, RANGES::QUALITY_MAX);
        validated.min_distance = clamp_value(validated.min_distance, 
                                            RANGES::DISTANCE_MIN, RANGES::DISTANCE_MAX);
        validated.block_size = clamp_value(validated.block_size, RANGES::BLOCK_SIZE_MIN, RANGES::BLOCK_SIZE_MAX);
        if (validated.block_size % SAFETY::EVEN_NUMBER_VALIDATION_THRESHOLD == 0) {
            validated.block_size++;
        }
        validated.k = clamp_value(validated.k, RANGES::K_MIN, RANGES::K_MAX);
        
        return validated;
    }
}