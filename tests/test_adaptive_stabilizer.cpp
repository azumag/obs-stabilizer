#include <gtest/gtest.h>
#include "core/adaptive_stabilizer.hpp"
#include "core/stabilizer_constants.hpp"
#include <opencv2/opencv.hpp>
#include <string>

using namespace AdaptiveStabilization;

class AdaptiveStabilizerTest : public ::testing::Test {
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
};

TEST_F(AdaptiveStabilizerTest, Initialize) {
    auto params = create_test_params();
    
    bool result = stabilizer->initialize(1920, 1080, params);
    EXPECT_TRUE(result);
    EXPECT_TRUE(stabilizer->is_ready());
}

TEST_F(AdaptiveStabilizerTest, ProcessFrame) {
    auto params = create_test_params();
    stabilizer->initialize(1920, 1080, params);
    
    cv::Mat frame = cv::Mat::zeros(1080, 1920, CV_8UC3);
    cv::Mat result = stabilizer->process_frame(frame);
    
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.rows, 1080);
    EXPECT_EQ(result.cols, 1920);
}

TEST_F(AdaptiveStabilizerTest, AdaptiveEnabled) {
    stabilizer->enable_adaptive(true);
    EXPECT_TRUE(stabilizer->is_adaptive_enabled());
    
    stabilizer->enable_adaptive(false);
    EXPECT_FALSE(stabilizer->is_adaptive_enabled());
}

TEST_F(AdaptiveStabilizerTest, UpdateParameters) {
    auto params = create_test_params();
    stabilizer->initialize(1920, 1080, params);
    
    StabilizerCore::StabilizerParams new_params = create_test_params();
    new_params.smoothing_radius = 40;
    
    stabilizer->update_parameters(new_params);
    
    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_GE(metrics.frame_count, 0);
}

TEST_F(AdaptiveStabilizerTest, Reset) {
    auto params = create_test_params();
    stabilizer->initialize(1920, 1080, params);
    
    cv::Mat frame = cv::Mat::zeros(1080, 1920, CV_8UC3);
    stabilizer->process_frame(frame);
    
    stabilizer->reset();
    
    cv::Mat result = stabilizer->process_frame(frame);
    
    EXPECT_FALSE(result.empty());
}

TEST_F(AdaptiveStabilizerTest, GetMotionType) {
    MotionType type = stabilizer->get_current_motion_type();
    EXPECT_EQ(type, MotionType::Static);
}

TEST_F(AdaptiveStabilizerTest, GetMetrics) {
    auto params = create_test_params();
    stabilizer->initialize(1920, 1080, params);
    
    MotionMetrics metrics = stabilizer->get_current_metrics();
    EXPECT_EQ(metrics.transform_count, 0);
}

TEST_F(AdaptiveStabilizerTest, SetConfig) {
    AdaptiveConfig new_config;
    new_config.transition_rate = 0.2;
    new_config.static_smoothing = 10;
    
    stabilizer->set_config(new_config);
    
    AdaptiveConfig retrieved = stabilizer->get_config();
    EXPECT_EQ(retrieved.transition_rate, 0.2);
    EXPECT_EQ(retrieved.static_smoothing, 10);
}

TEST_F(AdaptiveStabilizerTest, MotionSensitivity) {
    stabilizer->set_motion_sensitivity(2.0);
    EXPECT_EQ(stabilizer->get_motion_sensitivity(), 2.0);
}

TEST_F(AdaptiveStabilizerTest, EmptyFrame) {
    auto params = create_test_params();
    stabilizer->initialize(1920, 1080, params);
    
    cv::Mat empty_frame;
    cv::Mat result = stabilizer->process_frame(empty_frame);
    
    EXPECT_TRUE(result.empty());
}

TEST_F(AdaptiveStabilizerTest, GetPerformanceMetrics) {
    auto params = create_test_params();
    stabilizer->initialize(1920, 1080, params);
    
    StabilizerCore::PerformanceMetrics metrics = stabilizer->get_performance_metrics();
    EXPECT_GE(metrics.avg_processing_time, 0.0);
}
