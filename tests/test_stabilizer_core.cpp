#include <gtest/gtest.h>
#include "../src/core/stabilizer_core.hpp"
#include "../src/core/parameter_validation.hpp"
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
    // Test that VALIDATION::validate_parameters clamps invalid values to valid ranges
    StabilizerCore::StabilizerParams invalid_params;
    invalid_params.smoothing_radius = -1;  // Invalid: below MIN_RADIUS (1)

    StabilizerCore::StabilizerParams clamped = VALIDATION::validate_parameters(invalid_params);

    // Verify clamping occurred
    EXPECT_EQ(clamped.smoothing_radius, StabilizerConstants::Smoothing::MIN_RADIUS);
    EXPECT_NE(clamped.smoothing_radius, -1);  // Should NOT be -1 anymore

    // Test other edge cases
    invalid_params = StabilizerCore::StabilizerParams();
    invalid_params.max_correction = -1.0f;  // Invalid: below MIN_MAX (0.0f)
    clamped = VALIDATION::validate_parameters(invalid_params);
    EXPECT_FLOAT_EQ(clamped.max_correction, StabilizerConstants::Correction::MIN_MAX);

    // Test clamping above maximum
    invalid_params = StabilizerCore::StabilizerParams();
    invalid_params.feature_count = 10000;  // Invalid: above MAX_COUNT (2000)
    clamped = VALIDATION::validate_parameters(invalid_params);
    EXPECT_EQ(clamped.feature_count, StabilizerConstants::Features::MAX_COUNT);

    // Test block_size is forced to be odd
    invalid_params = StabilizerCore::StabilizerParams();
    invalid_params.block_size = 10;  // Even number
    clamped = VALIDATION::validate_parameters(invalid_params);
    EXPECT_EQ(clamped.block_size, 11);  // Should be incremented to odd number
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

TEST_F(StabilizerCoreTest, EdgeModePadding) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.edge_mode = StabilizerCore::EdgeMode::Padding;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat frame = TestDataGenerator::generate_test_frame(
        Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT);
    cv::Mat processed = stabilizer->process_frame(frame);

    EXPECT_EQ(processed.size(), frame.size());
    EXPECT_FALSE(processed.empty());
}

TEST_F(StabilizerCoreTest, EdgeModeCrop) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.edge_mode = StabilizerCore::EdgeMode::Crop;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat frame = TestDataGenerator::generate_test_frame_with_borders(
        Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, 50);
    cv::Mat processed = stabilizer->process_frame(frame);

    EXPECT_FALSE(processed.empty());
    EXPECT_GT(processed.cols, 0);
    EXPECT_GT(processed.rows, 0);
}

TEST_F(StabilizerCoreTest, EdgeModeScale) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.edge_mode = StabilizerCore::EdgeMode::Scale;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat frame = TestDataGenerator::generate_test_frame_with_borders(
        Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, 50);
    cv::Mat processed = stabilizer->process_frame(frame);

    EXPECT_EQ(processed.size(), cv::Size(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT));
    EXPECT_FALSE(processed.empty());
}

TEST_F(StabilizerCoreTest, DetectContentBounds) {
    // Create a frame with black borders and content in the center
    cv::Mat frame(480, 640, CV_8UC4, cv::Scalar(0, 0, 0, 255));
    // Add content in center, leave borders black
    cv::rectangle(frame, cv::Rect(100, 100, 440, 280), cv::Scalar(128, 128, 128, 255), -1);

    StabilizerCore core;
    cv::Rect bounds = core.detect_content_bounds(frame);

    // Verify bounds detected correctly
    EXPECT_EQ(bounds.x, 100);
    EXPECT_EQ(bounds.y, 100);
    EXPECT_EQ(bounds.width, 440);
    EXPECT_EQ(bounds.height, 280);
}

TEST_F(StabilizerCoreTest, DetectContentBoundsFullFrame) {
    // Create a frame without black borders (all content)
    cv::Mat frame(480, 640, CV_8UC4, cv::Scalar(128, 128, 128, 255));

    StabilizerCore core;
    cv::Rect bounds = core.detect_content_bounds(frame);

    // Verify bounds cover the full frame
    EXPECT_EQ(bounds.x, 0);
    EXPECT_EQ(bounds.y, 0);
    EXPECT_EQ(bounds.width, 640);
    EXPECT_EQ(bounds.height, 480);
}

TEST_F(StabilizerCoreTest, DetectContentBoundsExtremeBorder) {
    // Create a frame with extreme borders (> width/2)
    cv::Mat frame(480, 640, CV_8UC4, cv::Scalar(0, 0, 0, 255));
    // Content is smaller than half the frame
    cv::rectangle(frame, cv::Rect(350, 250, 100, 60), cv::Scalar(128, 128, 128, 255), -1);

    StabilizerCore core;
    cv::Rect bounds = core.detect_content_bounds(frame);

    // Verify bounds detected within frame
    EXPECT_GE(bounds.x, 0);
    EXPECT_GE(bounds.y, 0);
    EXPECT_GT(bounds.width, 0);
    EXPECT_GT(bounds.height, 0);
}

TEST_F(StabilizerCoreTest, DetectContentBoundsMinFrameSize) {
    // Create a frame at minimum size (32x32)
    cv::Mat frame(32, 32, CV_8UC4, cv::Scalar(128, 128, 128, 255));

    StabilizerCore core;
    cv::Rect bounds = core.detect_content_bounds(frame);

    // Verify bounds work at minimum frame size
    EXPECT_EQ(bounds.x, 0);
    EXPECT_EQ(bounds.y, 0);
    EXPECT_EQ(bounds.width, 32);
    EXPECT_EQ(bounds.height, 32);
}

TEST_F(StabilizerCoreTest, DetectContentBoundsUltraWideAspect) {
    // Create an ultrawide 21:9 frame (840x360)
    cv::Mat frame(360, 840, CV_8UC4, cv::Scalar(0, 0, 0, 255));
    cv::rectangle(frame, cv::Rect(50, 40, 740, 280), cv::Scalar(128, 128, 128, 255), -1);

    StabilizerCore core;
    cv::Rect bounds = core.detect_content_bounds(frame);

    // Verify bounds detected correctly for ultrawide
    EXPECT_EQ(bounds.x, 50);
    EXPECT_EQ(bounds.y, 40);
    EXPECT_EQ(bounds.width, 740);
    EXPECT_EQ(bounds.height, 280);
}

TEST_F(StabilizerCoreTest, DetectContentBoundsSquareAspect) {
    // Create a square 1:1 frame (480x480)
    cv::Mat frame(480, 480, CV_8UC4, cv::Scalar(0, 0, 0, 255));
    cv::rectangle(frame, cv::Rect(80, 80, 320, 320), cv::Scalar(128, 128, 128, 255), -1);

    StabilizerCore core;
    cv::Rect bounds = core.detect_content_bounds(frame);

    // Verify bounds detected correctly for square aspect
    EXPECT_EQ(bounds.x, 80);
    EXPECT_EQ(bounds.y, 80);
    EXPECT_EQ(bounds.width, 320);
    EXPECT_EQ(bounds.height, 320);
}

TEST_F(StabilizerCoreTest, DetectContentBoundsAllBlack) {
    // Create a completely black frame (no content)
    cv::Mat frame(480, 640, CV_8UC4, cv::Scalar(0, 0, 0, 255));

    StabilizerCore core;
    cv::Rect bounds = core.detect_content_bounds(frame);

    // For all-black frames, expect full frame bounds or minimal valid bounds
    EXPECT_GT(bounds.width, 0);
    EXPECT_GT(bounds.height, 0);
    EXPECT_LE(bounds.x, 640);
    EXPECT_LE(bounds.y, 480);
}

TEST_F(StabilizerCoreTest, EdgeModeWithMinimalBorders) {
    // Test EdgeMode with full frame content (no borders)
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.edge_mode = StabilizerCore::EdgeMode::Crop;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat frame = TestDataGenerator::generate_test_frame(
        Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT);
    cv::Mat processed = stabilizer->process_frame(frame);

    // With full frame content, output should match input size
    EXPECT_EQ(processed.size(), frame.size());
    EXPECT_FALSE(processed.empty());
}

TEST_F(StabilizerCoreTest, EdgeModeWithLargeBorders) {
    // Test EdgeMode with frame containing large black borders (> width/4)
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.edge_mode = StabilizerCore::EdgeMode::Crop;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat frame = TestDataGenerator::generate_test_frame_with_borders(
        Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, 200);
    cv::Mat processed = stabilizer->process_frame(frame);

    // With large borders, output should be smaller but valid
    EXPECT_FALSE(processed.empty());
    EXPECT_GT(processed.cols, 0);
    EXPECT_GT(processed.rows, 0);
}
