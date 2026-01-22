#include <gtest/gtest.h>
#include "../src/core/adaptive_stabilizer.hpp"
#include "../src/core/stabilizer_core.hpp"
#include "test_constants.hpp"
#include "test_data_generator.hpp"

using namespace TestConstants;
using namespace AdaptiveStabilization;

class AdaptiveStabilizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        stabilizer = std::make_unique<AdaptiveStabilizer>();
    }
    
    void TearDown() override {
        stabilizer.reset();
    }
    
    StabilizerCore::StabilizerParams getDefaultParams() {
        StabilizerCore::StabilizerParams params;
        params.smoothing_radius = Processing::MEDIUM_SMOOTHING_WINDOW;
        params.max_correction = 50.0f;
        params.feature_count = Features::DEFAULT_COUNT;
        params.quality_level = Processing::DEFAULT_QUALITY_LEVEL;
        params.min_distance = Processing::DEFAULT_MIN_DISTANCE;
        return params;
    }
    
    std::unique_ptr<AdaptiveStabilizer> stabilizer;
};

TEST_F(AdaptiveStabilizerTest, Initialization) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    
    bool initialized = stabilizer->initialize(
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        params
    );
    EXPECT_TRUE(initialized);
    EXPECT_TRUE(stabilizer->is_ready());
}

TEST_F(AdaptiveStabilizerTest, InitializationWithDifferentResolutions) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    
    EXPECT_TRUE(stabilizer->initialize(Resolution::QVGA_WIDTH, Resolution::QVGA_HEIGHT, params));
    EXPECT_TRUE(stabilizer->is_ready());
    
    stabilizer->reset();
    EXPECT_TRUE(stabilizer->initialize(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, params));
    EXPECT_TRUE(stabilizer->is_ready());
    
    stabilizer->reset();
    EXPECT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));
    EXPECT_TRUE(stabilizer->is_ready());
}

TEST_F(AdaptiveStabilizerTest, ProcessStaticSequence) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "static"
    );
    
    for (const auto& frame : frames) {
        cv::Mat processed = stabilizer->process_frame(frame);
        EXPECT_FALSE(processed.empty());
        EXPECT_EQ(processed.size(), frame.size());
    }
}

TEST_F(AdaptiveStabilizerTest, ProcessHorizontalMotion) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "horizontal"
    );
    
    for (const auto& frame : frames) {
        cv::Mat processed = stabilizer->process_frame(frame);
        EXPECT_FALSE(processed.empty());
    }
}

TEST_F(AdaptiveStabilizerTest, ProcessVerticalMotion) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "vertical"
    );
    
    for (const auto& frame : frames) {
        cv::Mat processed = stabilizer->process_frame(frame);
        EXPECT_FALSE(processed.empty());
    }
}

TEST_F(AdaptiveStabilizerTest, ProcessRotation) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "rotation"
    );
    
    for (const auto& frame : frames) {
        cv::Mat processed = stabilizer->process_frame(frame);
        EXPECT_FALSE(processed.empty());
    }
}

TEST_F(AdaptiveStabilizerTest, ProcessZoom) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "zoom"
    );
    
    for (const auto& frame : frames) {
        cv::Mat processed = stabilizer->process_frame(frame);
        EXPECT_FALSE(processed.empty());
    }
}

TEST_F(AdaptiveStabilizerTest, AdaptiveEnableDisable) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    stabilizer->enable_adaptive(true);
    EXPECT_TRUE(stabilizer->is_adaptive_enabled());
    
    stabilizer->enable_adaptive(false);
    EXPECT_FALSE(stabilizer->is_adaptive_enabled());
}

TEST_F(AdaptiveStabilizerTest, MotionTypeClassification) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "static"
    );
    
    for (const auto& frame : frames) {
        stabilizer->process_frame(frame);
    }
    
    MotionType type = stabilizer->get_current_motion_type();
    EXPECT_EQ(type, MotionType::Static);
}

TEST_F(AdaptiveStabilizerTest, MotionMetrics) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "static"
    );
    
    for (const auto& frame : frames) {
        stabilizer->process_frame(frame);
    }
    
    MotionMetrics metrics = stabilizer->get_current_metrics();
    EXPECT_GE(metrics.mean_magnitude, 0.0);
    EXPECT_GE(metrics.transform_count, 0);
}

TEST_F(AdaptiveStabilizerTest, UpdateParameters) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    StabilizerCore::StabilizerParams new_params = params;
    new_params.smoothing_radius = Processing::LARGE_SMOOTHING_WINDOW;
    new_params.max_correction = 30.0f;
    
    stabilizer->update_parameters(new_params);
    
    cv::Mat frame = TestDataGenerator::generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT);
    cv::Mat processed = stabilizer->process_frame(frame);
    EXPECT_FALSE(processed.empty());
}

TEST_F(AdaptiveStabilizerTest, ResetState) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    cv::Mat frame = TestDataGenerator::generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT);
    stabilizer->process_frame(frame);
    
    stabilizer->reset();
    
    cv::Mat processed = stabilizer->process_frame(frame);
    EXPECT_FALSE(processed.empty());
}

TEST_F(AdaptiveStabilizerTest, PerformanceMetrics) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "static"
    );
    
    for (const auto& frame : frames) {
        stabilizer->process_frame(frame);
    }
    
    StabilizerCore::PerformanceMetrics metrics = stabilizer->get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0);
}

TEST_F(AdaptiveStabilizerTest, MotionSensitivity) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    stabilizer->set_motion_sensitivity(1.5);
    EXPECT_EQ(stabilizer->get_motion_sensitivity(), 1.5);
    
    stabilizer->set_motion_sensitivity(0.5);
    EXPECT_EQ(stabilizer->get_motion_sensitivity(), 0.5);
}

TEST_F(AdaptiveStabilizerTest, AdaptiveConfig) {
    AdaptiveConfig config;
    config.static_smoothing = 10;
    config.static_correction = 20.0;
    config.static_features = 150;
    
    stabilizer->set_config(config);
    
    AdaptiveConfig retrieved = stabilizer->get_config();
    EXPECT_EQ(retrieved.static_smoothing, 10);
    EXPECT_EQ(retrieved.static_correction, 20.0);
    EXPECT_EQ(retrieved.static_features, 150);
}

TEST_F(AdaptiveStabilizerTest, ErrorHandling) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    
    cv::Mat empty_frame;
    cv::Mat result = stabilizer->process_frame(empty_frame);
    
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    cv::Mat invalid_frame(10, 10, CV_8UC4);
    result = stabilizer->process_frame(invalid_frame);
}

TEST_F(AdaptiveStabilizerTest, ComprehensiveMotionPattern) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    stabilizer->enable_adaptive(true);
    
    auto data = TestDataGenerator::generate_comprehensive_test_data(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT
    );
    
    for (const auto& frame : data.frames) {
        cv::Mat processed = stabilizer->process_frame(frame);
        EXPECT_FALSE(processed.empty());
    }
    
    MotionType type = stabilizer->get_current_motion_type();
    EXPECT_TRUE(type == MotionType::Static || type == MotionType::SlowMotion || type == MotionType::PanZoom);
}

TEST_F(AdaptiveStabilizerTest, LongSequenceProcessing) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    stabilizer->enable_adaptive(true);
    
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::LONG_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "horizontal"
    );
    
    for (const auto& frame : frames) {
        cv::Mat processed = stabilizer->process_frame(frame);
        EXPECT_FALSE(processed.empty());
    }
    
    EXPECT_TRUE(stabilizer->is_ready());
}
