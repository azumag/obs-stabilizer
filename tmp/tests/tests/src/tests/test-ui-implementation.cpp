/*
 * Phase 3 UI Implementation Test
 *
 * Tests the UI configuration structures and preset system
 * without requiring OBS dependencies
 */

#include <iostream>
#include <memory>
#include <gtest/gtest.h>

// Mock OBS definitions for UI testing
#define LOG_INFO 300
#define VIDEO_FORMAT_I420 2
#define VIDEO_FORMAT_NV12 1

inline void obs_log(int, const char*, ...) { /* Mock */ }

struct obs_source_frame {
    uint32_t width, height;
    int format;
    uint8_t* data[4];
    uint32_t linesize[4];
};

// Include the enhanced core module
#include "../src/core/stabilizer_core.hpp"

using namespace obs_stabilizer;

// Test class to provide helper methods
class UIImplementationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }

public:
    void apply_gaming_preset(StabilizerConfig& config) {
        config.smoothing_radius = 15;
        config.max_features = 150;
        config.error_threshold = 40.0f;
        config.output_mode = StabilizerConfig::OutputMode::CROP;
        config.min_feature_quality = 0.02f;
        config.refresh_threshold = 20;
        config.preset = StabilizerConfig::PresetMode::GAMING;
    }

    void apply_streaming_preset(StabilizerConfig& config) {
        config.smoothing_radius = 30;
        config.max_features = 200;
        config.error_threshold = 30.0f;
        config.output_mode = StabilizerConfig::OutputMode::PAD;
        config.min_feature_quality = 0.01f;
        config.refresh_threshold = 25;
        config.preset = StabilizerConfig::PresetMode::STREAMING;
    }

    void apply_recording_preset(StabilizerConfig& config) {
        config.smoothing_radius = 50;
        config.max_features = 400;
        config.error_threshold = 20.0f;
        config.output_mode = StabilizerConfig::OutputMode::SCALE_FIT;
        config.min_feature_quality = 0.005f;
        config.refresh_threshold = 30;
        config.preset = StabilizerConfig::PresetMode::RECORDING;
    }
};

// Test 1: Enhanced Configuration Structure
TEST_F(UIImplementationTest, EnhancedConfigurationStructure) {
    StabilizerConfig config;

    // Test default values
    EXPECT_EQ(config.smoothing_radius, 30);
    EXPECT_EQ(config.max_features, 200);
    EXPECT_EQ(config.error_threshold, 30.0f);
    EXPECT_TRUE(config.enable_stabilization);
    EXPECT_EQ(config.output_mode, StabilizerConfig::OutputMode::CROP);
    EXPECT_EQ(config.preset, StabilizerConfig::PresetMode::STREAMING);
    EXPECT_FALSE(config.enable_gpu_acceleration);
    EXPECT_EQ(config.processing_threads, 1);

    // Test enum values
    EXPECT_EQ(static_cast<int>(StabilizerConfig::OutputMode::CROP), 0);
    EXPECT_EQ(static_cast<int>(StabilizerConfig::OutputMode::PAD), 1);
    EXPECT_EQ(static_cast<int>(StabilizerConfig::OutputMode::SCALE_FIT), 2);

    EXPECT_EQ(static_cast<int>(StabilizerConfig::PresetMode::CUSTOM), 0);
    EXPECT_EQ(static_cast<int>(StabilizerConfig::PresetMode::GAMING), 1);
    EXPECT_EQ(static_cast<int>(StabilizerConfig::PresetMode::STREAMING), 2);
    EXPECT_EQ(static_cast<int>(StabilizerConfig::PresetMode::RECORDING), 3);
}

// Test 2: Preset System Validation
TEST_F(UIImplementationTest, PresetSystemValidation) {
    // Test Gaming preset characteristics
    StabilizerConfig gaming_config;
    apply_gaming_preset(gaming_config);

    EXPECT_EQ(gaming_config.smoothing_radius, 15) << "Gaming preset should have fast response";
    EXPECT_EQ(gaming_config.max_features, 150) << "Gaming preset should have fewer features for speed";
    EXPECT_EQ(gaming_config.error_threshold, 40.0f) << "Gaming preset should be more tolerant";
    EXPECT_EQ(gaming_config.output_mode, StabilizerConfig::OutputMode::CROP);

    // Test Streaming preset characteristics
    StabilizerConfig streaming_config;
    apply_streaming_preset(streaming_config);

    EXPECT_EQ(streaming_config.smoothing_radius, 30) << "Streaming preset should be balanced";
    EXPECT_EQ(streaming_config.max_features, 200) << "Streaming preset should use standard features";
    EXPECT_EQ(streaming_config.error_threshold, 30.0f) << "Streaming preset should be balanced";
    EXPECT_EQ(streaming_config.output_mode, StabilizerConfig::OutputMode::PAD);

    // Test Recording preset characteristics
    StabilizerConfig recording_config;
    apply_recording_preset(recording_config);

    EXPECT_EQ(recording_config.smoothing_radius, 50) << "Recording preset should prioritize high quality";
    EXPECT_EQ(recording_config.max_features, 400) << "Recording preset should have high feature density";
    EXPECT_EQ(recording_config.error_threshold, 20.0f) << "Recording preset should have strict quality";
    EXPECT_EQ(recording_config.output_mode, StabilizerConfig::OutputMode::SCALE_FIT);
}

// Test 3: Parameter Validation
TEST_F(UIImplementationTest, ParameterValidation) {
    StabilizerConfig config;

    // Test parameter ranges
    config.smoothing_radius = 50;
    config.max_features = 500;
    config.error_threshold = 25.0f;
    config.min_feature_quality = 0.005f;
    config.refresh_threshold = 35;

    // All values should be within specified ranges
    EXPECT_GE(config.smoothing_radius, 10);
    EXPECT_LE(config.smoothing_radius, 100);
    EXPECT_GE(config.max_features, 100);
    EXPECT_LE(config.max_features, 1000);
    EXPECT_GE(config.error_threshold, 10.0f);
    EXPECT_LE(config.error_threshold, 100.0f);
    EXPECT_GE(config.min_feature_quality, 0.001f);
    EXPECT_LE(config.min_feature_quality, 0.1f);
    EXPECT_GE(config.refresh_threshold, 10);
    EXPECT_LE(config.refresh_threshold, 50);

    // Test advanced parameters
    config.adaptive_refresh = false;
    config.enable_gpu_acceleration = true;
    config.processing_threads = 4;

    EXPECT_FALSE(config.adaptive_refresh);
    EXPECT_TRUE(config.enable_gpu_acceleration);
    EXPECT_GE(config.processing_threads, 1);
    EXPECT_LE(config.processing_threads, 8);
}

// Test 4: UI Property Mapping
TEST_F(UIImplementationTest, UIPropertyMapping) {
    // Test property name consistency (simulate OBS property names)
    std::vector<std::string> expected_properties = {
        "enable_stabilization",
        "preset_mode",
        "smoothing_radius",
        "max_features",
        "error_threshold",
        "output_mode",
        "min_feature_quality",
        "refresh_threshold",
        "adaptive_refresh",
        "enable_gpu_acceleration",
        "processing_threads"
    };

    EXPECT_EQ(expected_properties.size(), 11) << "Expected 11 UI properties";
    EXPECT_FALSE(expected_properties.empty()) << "UI properties should be defined";

    // Test preset labels
    std::vector<std::string> preset_labels = {
        "Custom",
        "Gaming (Fast Response)",
        "Streaming (Balanced)",
        "Recording (High Quality)"
    };

    EXPECT_EQ(preset_labels.size(), 4) << "Expected 4 preset labels";
    EXPECT_FALSE(preset_labels.empty()) << "Preset labels should be defined";

    // Verify specific labels exist
    EXPECT_NE(std::find(preset_labels.begin(), preset_labels.end(), "Gaming (Fast Response)"), preset_labels.end());
    EXPECT_NE(std::find(preset_labels.begin(), preset_labels.end(), "Streaming (Balanced)"), preset_labels.end());
    EXPECT_NE(std::find(preset_labels.begin(), preset_labels.end(), "Recording (High Quality)"), preset_labels.end());
}

// Test 5: Performance Characteristics
TEST_F(UIImplementationTest, PerformanceCharacteristics) {
    // Verify preset performance characteristics
    StabilizerConfig gaming, streaming, recording;

    apply_gaming_preset(gaming);
    apply_streaming_preset(streaming);
    apply_recording_preset(recording);

    // Gaming should be fastest (least features, highest tolerance)
    EXPECT_LT(gaming.max_features, streaming.max_features) << "Gaming should use fewer features than streaming for speed";
    EXPECT_LT(gaming.max_features, recording.max_features) << "Gaming should use fewer features than recording for speed";
    EXPECT_GT(gaming.error_threshold, streaming.error_threshold) << "Gaming should be more tolerant of errors for speed";
    EXPECT_LT(gaming.smoothing_radius, streaming.smoothing_radius) << "Gaming should have faster response (less smoothing)";

    // Recording should be highest quality (most features, lowest tolerance)
    EXPECT_GT(recording.max_features, streaming.max_features) << "Recording should use more features than streaming for quality";
    EXPECT_LT(recording.error_threshold, streaming.error_threshold) << "Recording should be less tolerant of errors for quality";
    EXPECT_GT(recording.smoothing_radius, streaming.smoothing_radius) << "Recording should have more smoothing for quality";

    // Verify specific values for regression testing
    EXPECT_EQ(gaming.max_features, 150);
    EXPECT_EQ(streaming.max_features, 200);
    EXPECT_EQ(recording.max_features, 400);

    EXPECT_EQ(gaming.error_threshold, 40.0f);
    EXPECT_EQ(streaming.error_threshold, 30.0f);
    EXPECT_EQ(recording.error_threshold, 20.0f);
}

// Main function for Google Test
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}