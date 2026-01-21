# Motion Classifier and Adaptive Stabilization Design

## Overview

This document defines the architecture and technical specifications for implementing advanced motion detection and automatic parameter adjustment in the OBS Stabilizer plugin.

## System Architecture

### Class Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     StabilizerCore                         │
│  - Existing stabilization engine                            │
│  - process_frame()                                         │
│  - get_current_transforms()                                 │
└────────────┬────────────────────────────────────────────────┘
             │ uses
             ▼
┌─────────────────────────────────────────────────────────────┐
│               AdaptiveStabilizer                            │
│  - MotionClassifier classifier_                             │
│  - MotionType current_motion_type_                         │
│  - AdaptiveParameters params_map_                           │
│  - process_with_adaptation()                                │
│  - update_adaptive_params()                                 │
└────────────┬────────────────────────────────────────────────┘
             │ uses
             ▼
┌─────────────────────────────────────────────────────────────┐
│               MotionClassifier                              │
│  - classify_motion(transforms)                             │
│  - calculate_variance(transforms)                           │
│  - detect_directionality(transforms)                        │
│  - calculate_frequency_metrics(transforms)                  │
└─────────────────────────────────────────────────────────────┘
```

## Motion Type Classification

### Motion Types

```cpp
enum class MotionType {
    Static,        // Minimal movement (< 1% max correction)
    SlowMotion,    // Gentle movement (1-5% max correction)
    FastMotion,    // Rapid movement (5-15% max correction)
    CameraShake,   // High-frequency jitter
    PanZoom        // Systematic directional motion
};
```

### Classification Algorithm

#### 1. Transform Analysis Metrics

```cpp
struct MotionMetrics {
    double mean_magnitude;           // Average movement per frame
    double variance_magnitude;       // Variance of movement magnitudes
    double directional_variance;      // Variance in movement direction
    double high_frequency_ratio;      // Ratio of high-frequency components
    double consistency_score;        // Directional consistency (0-1)
    size_t transform_count;          // Number of transforms analyzed
};
```

#### 2. Classification Logic

**Static Content Detection**
- Condition: `mean_magnitude < 1.0%` AND `variance_magnitude < 0.5%`
- Reason: Minimal movement indicates static scene

**Slow Motion Detection**
- Condition: `1.0% ≤ mean_magnitude < 5.0%` AND `variance_magnitude < 3.0%`
- Reason: Gentle movement indicates slow motion

**Fast Motion Detection**
- Condition: `5.0% ≤ mean_magnitude < 15.0%` AND `variance_magnitude < 8.0%`
- Reason: High average movement indicates fast action

**Camera Shake Detection**
- Condition: `variance_magnitude > 8.0%` OR `high_frequency_ratio > 0.6`
- Reason: High variance or high-frequency jitter indicates shake

**Pan/Zoom Detection**
- Condition: `consistency_score > 0.7` AND `directional_variance < 2.0%`
- Reason: Consistent directional movement indicates pan/zoom

## Adaptive Parameter Adjustment

### Parameter Mapping Table

| Motion Type   | Smoothing Radius | Max Correction | Feature Count | Quality Level | Refresh Interval |
|--------------|------------------|----------------|---------------|---------------|------------------|
| Static       | 5-10             | 10-20%         | 100-150       | 0.015         | 50 frames        |
| SlowMotion   | 20-30            | 20-30%         | 150-200       | 0.010         | 40 frames        |
| FastMotion   | 40-60            | 30-40%         | 200-300       | 0.010         | 30 frames        |
| CameraShake  | 50-80            | 40-50%         | 300-400       | 0.005         | 20 frames        |
| PanZoom      | 10-20            | 15-25%         | 200-250       | 0.010         | 35 frames        |

### Adaptive Logic

```cpp
class AdaptiveParameters {
public:
    StabilizerCore::StabilizerParams get_params(MotionType type) {
        StabilizerCore::StabilizerParams params;
        
        switch (type) {
            case MotionType::Static:
                params.smoothing_radius = config_.static_smoothing;
                params.max_correction = config_.static_correction;
                params.feature_count = config_.static_features;
                params.quality_level = config_.static_quality;
                params.feature_refresh_threshold = 0.9f; // Less frequent refresh
                break;
                
            case MotionType::SlowMotion:
                params.smoothing_radius = config_.slow_smoothing;
                params.max_correction = config_.slow_correction;
                params.feature_count = config_.slow_features;
                params.quality_level = config_.slow_quality;
                params.feature_refresh_threshold = 0.7f;
                break;
                
            case MotionType::FastMotion:
                params.smoothing_radius = config_.fast_smoothing;
                params.max_correction = config_.fast_correction;
                params.feature_count = config_.fast_features;
                params.quality_level = config_.fast_quality;
                params.feature_refresh_threshold = 0.5f; // More frequent refresh
                break;
                
            case MotionType::CameraShake:
                params.smoothing_radius = config_.shake_smoothing;
                params.max_correction = config_.shake_correction;
                params.feature_count = config_.shake_features;
                params.quality_level = config_.shake_quality;
                params.feature_refresh_threshold = 0.4f; // Very frequent refresh
                break;
                
            case MotionType::PanZoom:
                params.smoothing_radius = config_.pan_smoothing;
                params.max_correction = config_.pan_correction;
                params.feature_count = config_.pan_features;
                params.quality_level = config_.pan_quality;
                params.feature_refresh_threshold = 0.6f;
                break;
        }
        
        return params;
    }
};
```

## Motion-Specific Smoothing

### 1. Standard Smoothing (SlowMotion, FastMotion)
- **Algorithm**: Moving average with configurable window
- **Implementation**: Existing `smooth_transforms_optimized()`

### 2. High-Pass Filtering (CameraShake)
```cpp
cv::Mat smooth_high_pass(const std::deque<cv::Mat>& transforms) {
    // Apply band-stop filter to remove high-frequency jitter
    cv::Mat smoothed = calculate_low_pass(transforms);
    cv::Mat high_freq = transforms.back() - smoothed;
    
    // Attenuate high-frequency components
    cv::Mat result = smoothed + high_freq * 0.3; // 30% high-pass
    
    return result;
}
```

### 3. Directional Smoothing (PanZoom)
```cpp
cv::Mat smooth_directional(const std::deque<cv::Mat>& transforms, 
                          const cv::Vec2d& direction) {
    cv::Mat smoothed = cv::Mat::zeros(2, 3, CV_64F);
    
    // Smooth along motion direction more aggressively
    for (const auto& t : transforms) {
        double* ptr = t.ptr<double>(0);
        
        // Translation components
        double parallel_mag = ptr[2] * direction[0] + ptr[5] * direction[1];
        double perp_mag = ptr[2] * direction[1] - ptr[5] * direction[0];
        
        // Less smoothing for parallel motion (preserve intentional pan/zoom)
        smoothed.at<double>(0, 2) += ptr[2] * 0.8; // 80% preserve
        smoothed.at<double>(1, 2) += ptr[5] * 0.8;
        
        // More smoothing for perpendicular jitter
        // ... apply differential smoothing
    }
    
    return smoothed / static_cast<double>(transforms.size());
}
```

## UI Integration

### OBS Properties Panel

```cpp
static obs_properties_t *adaptive_stabilizer_properties(void *data) {
    obs_properties_t *props = obs_properties_create();
    
    // Adaptive Mode Toggle
    obs_properties_add_bool(props, "adaptive_enabled", "Enable Adaptive Mode");
    
    // Motion Sensitivity Slider
    obs_properties_add_int_slider(props, "motion_sensitivity", 
                                 "Motion Sensitivity", 1, 10, 1);
    
    // Per-Motion-Type Settings Group
    obs_property_t* group = obs_properties_add_group(
        props, OBS_GROUP_NORMAL, "Motion-Specific Settings", true);
    
    // Static Content Settings
    obs_properties_add_int_slider(props, "static_smoothing", 
                                "Static - Smoothing Radius", 5, 20, 1);
    obs_properties_add_float_slider(props, "static_correction", 
                                  "Static - Max Correction (%)", 5, 30, 1);
    
    // Repeat for Slow, Fast, Shake, PanZoom...
    
    // Debug Mode
    obs_properties_add_bool(props, "debug_adaptive", "Debug Adaptive Mode");
    
    return props;
}
```

### Debug Visualization

```cpp
void draw_debug_info(cv::Mat& frame, MotionType type, 
                   const MotionMetrics& metrics) {
    // Display current motion type
    std::string type_str = motion_type_to_string(type);
    cv::putText(frame, "Motion: " + type_str, cv::Point(10, 30),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
    
    // Display metrics
    cv::putText(frame, 
                "Mean: " + std::to_string(metrics.mean_magnitude) + "%",
                cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.7,
                cv::Scalar(0, 255, 0), 1);
    
    cv::putText(frame, 
                "Variance: " + std::to_string(metrics.variance_magnitude) + "%",
                cv::Point(10, 85), cv::FONT_HERSHEY_SIMPLEX, 0.7,
                cv::Scalar(0, 255, 0), 1);
    
    // Display current parameters
    // ...
}
```

## Implementation Phases

### Phase 1: Motion Classifier Implementation

**Files to Create:**
- `src/core/motion_classifier.hpp`
- `src/core/motion_classifier.cpp`

**Key Classes:**
```cpp
class MotionClassifier {
public:
    MotionClassifier();
    ~MotionClassifier();
    
    MotionType classify(const std::deque<cv::Mat>& transforms);
    MotionMetrics calculate_metrics(const std::deque<cv::Mat>& transforms);
    
private:
    MotionMetrics metrics_;
    size_t window_size_;
    double sensitivity_threshold_;
};
```

**Acceptance Criteria:**
- MotionClassifier correctly identifies all 5 motion types (> 90% accuracy)
- Unit tests for each motion type with synthetic data
- Performance: < 0.5ms per classification

### Phase 2: Adaptive Parameter Logic

**Files to Create:**
- `src/core/adaptive_stabilizer.hpp`
- `src/core/adaptive_stabilizer.cpp`

**Key Classes:**
```cpp
class AdaptiveStabilizer : public StabilizerCore {
public:
    AdaptiveStabilizer();
    ~AdaptiveStabilizer();
    
    cv::Mat process_with_adaptation(const cv::Mat& frame);
    void update_adaptive_params(MotionType type);
    
private:
    MotionClassifier classifier_;
    MotionType current_motion_type_;
    std::map<MotionType, StabilizerParams> params_map_;
    bool adaptive_enabled_;
    int motion_sensitivity_;
};
```

**Acceptance Criteria:**
- Parameters adapt correctly for each motion type
- Smooth transitions between motion types
- No oscillation in parameter values

### Phase 3: Motion-Specific Smoothing

**Modifications:**
- Extend `StabilizerCore::smooth_transforms()` with motion-type-specific implementations
- Add high-pass filter for camera shake
- Add directional smoothing for pan/zoom

**Acceptance Criteria:**
- Camera shake reduced by > 80%
- Pan/zoom artifacts minimized
- Performance overhead < 10%

### Phase 4: UI Controls

**Modifications:**
- Extend `stabilizer_opencv.cpp` properties panel
- Add adaptive mode toggle
- Add per-motion-type parameter controls
- Add debug visualization

**Acceptance Criteria:**
- All UI controls functional in OBS
- Real-time parameter display works
- Debug mode shows motion type and metrics

### Phase 5: Testing and Tuning

**Tests to Implement:**
- Synthetic motion generation for each type
- Classification accuracy tests
- Parameter transition smoothness tests
- Performance benchmarks
- Real-world video tests

**Acceptance Criteria:**
- Classification accuracy > 90%
- All tests pass
- No performance regression
- Documentation complete

## Performance Targets

| Metric                     | Target          |
|----------------------------|-----------------|
| Classification Time         | < 0.5ms/frame  |
| Parameter Update Time       | < 0.1ms/frame  |
| Additional Overhead        | < 5% total      |
| Static Content Performance  | No degradation  |
| Smooth Transitions          | < 3 frames      |

## Dependencies

- **StabilizerCore**: ✅ Complete
- **Transform History**: ✅ Available (`transforms_` deque)
- **OBS Integration**: ✅ Complete
- **UI Framework**: ✅ Complete

## Risks and Mitigations

| Risk                          | Mitigation                        |
|-------------------------------|-----------------------------------|
| Classification errors          | Fallback to streaming preset      |
| Parameter oscillation          | Smoothing on parameter changes     |
| Performance degradation        | Profile and optimize hot paths     |
| UI complexity                 | Collapse advanced settings         |
| Real-world variability        | Extensive testing with diverse video |

## Success Metrics

1. **Quality Improvement**: Stabilization quality improved by > 20% vs fixed parameters
2. **Classification Accuracy**: > 90% correct motion type identification
3. **User Satisfaction**: Reduced manual parameter tuning
4. **Performance**: No degradation for static content
5. **Test Coverage**: 100% for new functionality
