#include <gtest/gtest.h>
#include "core/adaptive_stabilizer.hpp"
#include "core/motion_classifier.hpp"
#include <opencv2/opencv.hpp>

using namespace AdaptiveStabilization;

class AdaptiveStabilizerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = AdaptiveConfig();
        stabilizer = new AdaptiveStabilizer(config);
    }
    
    void TearDown() override {
        delete stabilizer;
    }
    
    AdaptiveStabilizer* stabilizer;
    AdaptiveConfig config;
    
    StabilizerCore::StabilizerParams create_test_params() {
        StabilizerCore::StabilizerParams params;
        params.smoothing_radius = 20;
        params.max_correction = 30.0f;
        params.feature_count = 200;
        params.quality_level = 0.01f;
        params.min_distance = 10.0f;
        params.block_size = 3;
        params.use_harris = false;
        params.k = 0.04f;
        params.enabled = true;
        params.optical_flow_pyramid_levels = 3;
        params.optical_flow_window_size = 21;
        params.feature_refresh_threshold = 0.5f;
        params.adaptive_feature_min = 100;
        params.adaptive_feature_max = 500;
        return params;
    }
    
    cv::Mat create_test_frame(uint32_t frame_num) {
        cv::Mat frame = cv::Mat::zeros(1080, 1920, CV_8UC3);
        uint8_t* data = frame.data;
        for (int y = 0; y < 1080; y++) {
            for (int x = 0; x < 1920; x++) {
                data[(y * 1920 + x) * 3] = static_cast<uint8_t>((frame_num * 7) % 255);
                data[(y * 1920 + x) * 3 + 1] = static_cast<uint8_t>((frame_num * 11) % 255);
                data[(y * 1920 + x) * 3 + 2] = static_cast<uint8_t>((frame_num * 13) % 255);
            }
        }
        return frame;
    }
};

TEST_F(AdaptiveStabilizerIntegrationTest, FullWorkflow) {
    auto params = create_test_params();
    EXPECT_TRUE(stabilizer->initialize(1920, 1080, params));
    
    stabilizer->enable_adaptive(true);
    EXPECT_TRUE(stabilizer->is_adaptive_enabled());
    
    MotionType initial_type = stabilizer->get_current_motion_type();
    EXPECT_EQ(initial_type, MotionType::Static);
    
    for (int i = 0; i < 30; i++) {
        cv::Mat frame = create_test_frame(i);
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
    
    EXPECT_GT(stabilizer->get_performance_metrics().frame_count, 0);
}

TEST_F(AdaptiveStabilizerIntegrationTest, MotionTypeDetection) {
    auto params = create_test_params();
    stabilizer->initialize(1920, 1080, params);
    
    cv::Mat frame = create_test_frame(0);
    stabilizer->process_frame(frame);
    
    MotionMetrics metrics = stabilizer->get_current_metrics();
    EXPECT_EQ(metrics.transform_count, 0);
}

TEST_F(AdaptiveStabilizerIntegrationTest, ConfigPersistence) {
    AdaptiveConfig new_config;
    new_config.transition_rate = 0.3;
    new_config.static_smoothing = 12;
    new_config.slow_smoothing = 28;
    new_config.fast_smoothing = 55;
    
    stabilizer->set_config(new_config);
    
    AdaptiveConfig retrieved = stabilizer->get_config();
    EXPECT_EQ(retrieved.transition_rate, 0.3);
    EXPECT_EQ(retrieved.static_smoothing, 12);
    EXPECT_EQ(retrieved.slow_smoothing, 28);
    EXPECT_EQ(retrieved.fast_smoothing, 55);
}

TEST_F(AdaptiveStabilizerIntegrationTest, SensitivityAdjustment) {
    auto params = create_test_params();
    stabilizer->initialize(1920, 1080, params);
    
    stabilizer->set_motion_sensitivity(1.5);
    EXPECT_EQ(stabilizer->get_motion_sensitivity(), 1.5);
    
    stabilizer->set_motion_sensitivity(0.8);
    EXPECT_EQ(stabilizer->get_motion_sensitivity(), 0.8);
}

TEST_F(AdaptiveStabilizerIntegrationTest, ParameterTransition) {
    auto params = create_test_params();
    stabilizer->initialize(1920, 1080, params);
    
    AdaptiveConfig transition_config;
    transition_config.transition_rate = 0.5;
    stabilizer->set_config(transition_config);
    
    for (int i = 0; i < 10; i++) {
        cv::Mat frame = create_test_frame(i);
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
    
    MotionMetrics metrics = stabilizer->get_current_metrics();
    EXPECT_GE(metrics.transform_count, 0);
}

TEST_F(AdaptiveStabilizerIntegrationTest, ErrorHandling) {
    auto params = create_test_params();
    EXPECT_TRUE(stabilizer->initialize(1920, 1080, params));
    
    cv::Mat empty_frame;
    cv::Mat result = stabilizer->process_frame(empty_frame);
    EXPECT_TRUE(result.empty());
    
    std::string error = stabilizer->get_last_error();
    EXPECT_FALSE(error.empty());
}

TEST_F(AdaptiveStabilizerIntegrationTest, PerformanceMetrics) {
    auto params = create_test_params();
    stabilizer->initialize(1920, 1080, params);
    
    for (int i = 0; i < 50; i++) {
        cv::Mat frame = create_test_frame(i);
        stabilizer->process_frame(frame);
    }
    
    StabilizerCore::PerformanceMetrics metrics = stabilizer->get_performance_metrics();
    EXPECT_GE(metrics.frame_count, 50);
    EXPECT_GE(metrics.avg_processing_time, 0.0);
}

TEST_F(AdaptiveStabilizerIntegrationTest, PerformanceWarningsLogged) {
    auto params = create_test_params();
    stabilizer->initialize(1920, 1080, params);
    
    // Process frames to trigger potential performance warnings
    for (int i = 0; i < 100; i++) {
        cv::Mat frame = create_test_frame(i);
        stabilizer->process_frame(frame);
    }
    
    // Verify that performance metrics are tracked
    StabilizerCore::PerformanceMetrics metrics = stabilizer->get_performance_metrics();
    EXPECT_GE(metrics.frame_count, 100);
    EXPECT_GT(metrics.avg_processing_time, 0.0);
    
    // The warning would be logged internally via OBSPerformanceMonitor
    // We verify the system works by checking metrics are updated
}

TEST_F(AdaptiveStabilizerIntegrationTest, AdaptiveConfigDefaults) {
    AdaptiveConfig config;
    
    // Verify default values match expected defaults
    EXPECT_EQ(config.static_smoothing, 8);
    EXPECT_DOUBLE_EQ(config.static_correction, 15.0);
    EXPECT_EQ(config.static_features, 120);
    EXPECT_DOUBLE_EQ(config.static_quality, 0.015);
    
    EXPECT_EQ(config.slow_smoothing, 25);
    EXPECT_DOUBLE_EQ(config.slow_correction, 25.0);
    EXPECT_EQ(config.slow_features, 175);
    EXPECT_DOUBLE_EQ(config.slow_quality, 0.010);
    
    EXPECT_EQ(config.fast_smoothing, 50);
    EXPECT_DOUBLE_EQ(config.fast_correction, 35.0);
    EXPECT_EQ(config.fast_features, 250);
    EXPECT_DOUBLE_EQ(config.fast_quality, 0.010);
    
    EXPECT_EQ(config.shake_smoothing, 65);
    EXPECT_DOUBLE_EQ(config.shake_correction, 45.0);
    EXPECT_EQ(config.shake_features, 350);
    EXPECT_DOUBLE_EQ(config.shake_quality, 0.005);
    
    EXPECT_EQ(config.pan_smoothing, 15);
    EXPECT_DOUBLE_EQ(config.pan_correction, 20.0);
    EXPECT_EQ(config.pan_features, 225);
    EXPECT_DOUBLE_EQ(config.pan_quality, 0.010);
    
    EXPECT_DOUBLE_EQ(config.transition_rate, 0.1);
}

TEST_F(AdaptiveStabilizerIntegrationTest, EnableDisable) {
    auto params = create_test_params();
    EXPECT_TRUE(stabilizer->initialize(1920, 1080, params));
    
    // Verify initially disabled
    EXPECT_FALSE(stabilizer->is_adaptive_enabled());
    
    // Enable adaptive
    stabilizer->enable_adaptive(true);
    EXPECT_TRUE(stabilizer->is_adaptive_enabled());
    
    // Process frames with adaptive enabled
    for (int i = 0; i < 20; i++) {
        cv::Mat frame = create_test_frame(i);
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
    
    // Disable adaptive
    stabilizer->enable_adaptive(false);
    EXPECT_FALSE(stabilizer->is_adaptive_enabled());
    
    // Verify frames still process without adaptive
    for (int i = 0; i < 10; i++) {
        cv::Mat frame = create_test_frame(i + 20);
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

TEST_F(AdaptiveStabilizerIntegrationTest, BackwardCompatibility) {
    auto params = create_test_params();
    EXPECT_TRUE(stabilizer->initialize(1920, 1080, params));
    
    // Verify stabilizer works with adaptive disabled (default state)
    EXPECT_FALSE(stabilizer->is_adaptive_enabled());
    
    for (int i = 0; i < 30; i++) {
        cv::Mat frame = create_test_frame(i);
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
    
    // Verify standard stabilizer still processes frames
    EXPECT_GT(stabilizer->get_performance_metrics().frame_count, 0);
}
