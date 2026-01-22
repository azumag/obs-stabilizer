#include <gtest/gtest.h>
#include "../src/core/stabilizer_core.hpp"
#include "test_constants.hpp"
#include "test_data_generator.hpp"

using namespace TestConstants;

class StabilizerCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        stabilizer = std::make_unique<StabilizerCore>();
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
    
    std::unique_ptr<StabilizerCore> stabilizer;
};

TEST_F(StabilizerCoreTest, BasicFunctionality) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    
    bool initialized = stabilizer->initialize(
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        params
    );
    EXPECT_TRUE(initialized);
    EXPECT_TRUE(stabilizer->is_ready());
}

TEST_F(StabilizerCoreTest, InitializationWithDifferentResolutions) {
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

TEST_F(StabilizerCoreTest, ProcessSingleFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    cv::Mat frame = TestDataGenerator::generate_test_frame(
        Resolution::VGA_WIDTH, 
        Resolution::VGA_HEIGHT
    );
    
    cv::Mat processed = stabilizer->process_frame(frame);
    
    EXPECT_FALSE(processed.empty());
    EXPECT_EQ(processed.cols, frame.cols);
    EXPECT_EQ(processed.rows, frame.rows);
}

TEST_F(StabilizerCoreTest, ProcessMultipleFrames) {
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

TEST_F(StabilizerCoreTest, ProcessHorizontalMotion) {
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

TEST_F(StabilizerCoreTest, ProcessVerticalMotion) {
    StabilizerCore stabilizer;
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "vertical"
    );
    
    for (const auto& frame : frames) {
        cv::Mat processed = stabilizer.process_frame(frame);
        EXPECT_FALSE(processed.empty());
    }
}

TEST_F(StabilizerCoreTest, ProcessRotation) {
    StabilizerCore stabilizer;
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "rotation"
    );
    
    for (const auto& frame : frames) {
        cv::Mat processed = stabilizer.process_frame(frame);
        EXPECT_FALSE(processed.empty());
    }
}

TEST_F(StabilizerCoreTest, ProcessZoom) {
    StabilizerCore stabilizer;
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "zoom"
    );
    
    for (const auto& frame : frames) {
        cv::Mat processed = stabilizer.process_frame(frame);
        EXPECT_FALSE(processed.empty());
    }
}

TEST_F(StabilizerCoreTest, ParameterValidation) {
    EXPECT_TRUE(StabilizerCore::validate_parameters(getDefaultParams()));
    
    StabilizerCore::StabilizerParams invalid_params;
    invalid_params.smoothing_radius = -1;
    EXPECT_FALSE(StabilizerCore::validate_parameters(invalid_params));
    
    invalid_params = getDefaultParams();
    invalid_params.max_correction = -1.0f;
    EXPECT_FALSE(StabilizerCore::validate_parameters(invalid_params));
}

TEST_F(StabilizerCoreTest, UpdateParameters) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    StabilizerCore::StabilizerParams new_params = params;
    new_params.smoothing_radius = Processing::LARGE_SMOOTHING_WINDOW;
    new_params.max_correction = 30.0f;
    
    stabilizer->update_parameters(new_params);
    
    StabilizerCore::StabilizerParams current = stabilizer->get_current_params();
    EXPECT_EQ(current.smoothing_radius, Processing::LARGE_SMOOTHING_WINDOW);
    EXPECT_EQ(current.max_correction, 30.0f);
}

TEST_F(StabilizerCoreTest, ResetState) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    cv::Mat frame = TestDataGenerator::generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT);
    stabilizer->process_frame(frame);
    
    EXPECT_FALSE(stabilizer->get_current_transforms().empty());
    
    stabilizer->reset();
    EXPECT_TRUE(stabilizer->get_current_transforms().empty());
}

TEST_F(StabilizerCoreTest, ClearState) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    cv::Mat frame = TestDataGenerator::generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT);
    stabilizer->process_frame(frame);
    
    stabilizer->clear_state();
}

TEST_F(StabilizerCoreTest, PerformanceMetrics) {
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

TEST_F(StabilizerCoreTest, PresetConfigurations) {
    auto gaming = StabilizerCore::get_preset_gaming();
    EXPECT_GT(gaming.smoothing_radius, 0);
    EXPECT_GT(gaming.max_correction, 0);
    EXPECT_GT(gaming.feature_count, 0);
    
    auto streaming = StabilizerCore::get_preset_streaming();
    EXPECT_GT(streaming.smoothing_radius, 0);
    EXPECT_GT(streaming.max_correction, 0);
    EXPECT_GT(streaming.feature_count, 0);
    
    auto recording = StabilizerCore::get_preset_recording();
    EXPECT_GT(recording.smoothing_radius, 0);
    EXPECT_GT(recording.max_correction, 0);
    EXPECT_GT(recording.feature_count, 0);
}

TEST_F(StabilizerCoreTest, ErrorHandling) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    
    cv::Mat empty_frame;
    cv::Mat result = stabilizer->process_frame(empty_frame);
    
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    
    cv::Mat invalid_frame(10, 10, CV_8UC4);
    result = stabilizer->process_frame(invalid_frame);
}

TEST_F(StabilizerCoreTest, DifferentFeatureCounts) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    
    params.feature_count = Features::LOW_COUNT;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    cv::Mat frame = TestDataGenerator::generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT);
    cv::Mat processed = stabilizer->process_frame(frame);
    EXPECT_FALSE(processed.empty());
    
    stabilizer->reset();
    params.feature_count = Features::HIGH_COUNT;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    processed = stabilizer->process_frame(frame);
    EXPECT_FALSE(processed.empty());
}

TEST_F(StabilizerCoreTest, DifferentSmoothingWindows) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    
    params.smoothing_radius = Processing::SMALL_SMOOTHING_WINDOW;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    cv::Mat frame = TestDataGenerator::generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT);
    cv::Mat processed = stabilizer->process_frame(frame);
    EXPECT_FALSE(processed.empty());
    
    stabilizer->reset();
    params.smoothing_radius = Processing::LARGE_SMOOTHING_WINDOW;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    processed = stabilizer->process_frame(frame);
    EXPECT_FALSE(processed.empty());
}
