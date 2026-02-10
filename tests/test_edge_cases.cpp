#include <gtest/gtest.h>
#include "../src/core/stabilizer_core.hpp"
#include "../src/core/parameter_validation.hpp"
#include "../src/core/frame_utils.hpp"
#include "test_constants.hpp"
#include "test_data_generator.hpp"

using namespace TestConstants;

class EdgeCaseTest : public ::testing::Test {
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

// ============================================================================
// Empty Frame Tests
// ============================================================================

TEST_F(EdgeCaseTest, EmptyFrameHandling) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat empty_frame;
    cv::Mat result = stabilizer->process_frame(empty_frame);

    // Empty frame should be handled gracefully
    EXPECT_TRUE(result.empty());
}

TEST_F(EdgeCaseTest, NullFramePointerHandling) {
    // This test verifies that the stabilizer can handle null frame pointers
    // The current implementation uses reference parameters, so this test
    // validates the frame validation logic
    cv::Mat empty_frame;
    EXPECT_TRUE(FRAME_UTILS::Validation::validate_cv_mat(empty_frame) == false);
}

// ============================================================================
// Invalid Frame Size Tests
// ============================================================================

TEST_F(EdgeCaseTest, ZeroWidthFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    cv::Mat zero_width_frame(Resolution::VGA_HEIGHT, 0, CV_8UC4);

    EXPECT_FALSE(stabilizer->validate_frame(zero_width_frame));
}

TEST_F(EdgeCaseTest, ZeroHeightFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    cv::Mat zero_height_frame(0, Resolution::VGA_WIDTH, CV_8UC4);

    EXPECT_FALSE(stabilizer->validate_frame(zero_height_frame));
}

TEST_F(EdgeCaseTest, VerySmallFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    cv::Mat tiny_frame(1, 1, CV_8UC4);

    EXPECT_FALSE(stabilizer->validate_frame(tiny_frame));
}

TEST_F(EdgeCaseTest, VeryLargeFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    cv::Mat huge_frame(10000, 10000, CV_8UC4);

    EXPECT_FALSE(stabilizer->validate_frame(huge_frame));
}

// ============================================================================
// Invalid Frame Format Tests
// ============================================================================

TEST_F(EdgeCaseTest, SingleChannelFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat single_channel_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC1);
    cv::Mat result = stabilizer->process_frame(single_channel_frame);

    // Single channel frames should be processed correctly (grayscale)
    EXPECT_FALSE(result.empty());
}

TEST_F(EdgeCaseTest, TwoChannelFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat two_channel_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC2);
    cv::Mat result = stabilizer->process_frame(two_channel_frame);

    // Two channel frames are not supported, should return empty
    EXPECT_TRUE(result.empty());
}

TEST_F(EdgeCaseTest, SixteenBitDepthFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat sixteen_bit_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_16UC4);
    cv::Mat result = stabilizer->process_frame(sixteen_bit_frame);

    // 16-bit frames are not supported, should return empty or converted
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// Boundary Value Tests
// ============================================================================

TEST_F(EdgeCaseTest, MinimumValidFrameSize) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(32, 32, params));

    cv::Mat min_frame = TestDataGenerator::generate_test_frame(32, 32);
    cv::Mat result = stabilizer->process_frame(min_frame);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.cols, 32);
    EXPECT_EQ(result.rows, 32);
}

TEST_F(EdgeCaseTest, SmoothingRadiusBoundary) {
    StabilizerCore::StabilizerParams params = getDefaultParams();

    // Test minimum smoothing radius (1)
    params.smoothing_radius = 1;
    EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Test maximum smoothing radius (1000)
    params.smoothing_radius = 1000;
    EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Test invalid smoothing radius (0)
    params.smoothing_radius = 0;
    params = VALIDATION::validate_parameters(params);
    EXPECT_GT(params.smoothing_radius, 0);

    // Test invalid smoothing radius (negative)
    params.smoothing_radius = -10;
    params = VALIDATION::validate_parameters(params);
    EXPECT_GT(params.smoothing_radius, 0);
}

TEST_F(EdgeCaseTest, FeatureCountBoundary) {
    StabilizerCore::StabilizerParams params = getDefaultParams();

    // Test minimum feature count (1)
    params.feature_count = 1;
    EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Test maximum feature count (10000)
    params.feature_count = 10000;
    EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Test invalid feature count (0)
    params.feature_count = 0;
    params = VALIDATION::validate_parameters(params);
    EXPECT_GT(params.feature_count, 0);
}

TEST_F(EdgeCaseTest, QualityLevelBoundary) {
    StabilizerCore::StabilizerParams params = getDefaultParams();

    // Test minimum quality level (very small)
    params.quality_level = 0.0001f;
    EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Test maximum quality level (1.0)
    params.quality_level = 1.0f;
    EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Test invalid quality level (negative)
    params.quality_level = -0.01f;
    params = VALIDATION::validate_parameters(params);
    EXPECT_GE(params.quality_level, 0.0f);

    // Test invalid quality level (> 1.0)
    params.quality_level = 1.1f;
    params = VALIDATION::validate_parameters(params);
    EXPECT_LE(params.quality_level, 1.0f);
}

// ============================================================================
// Reset and Reinitialization Tests
// ============================================================================

TEST_F(EdgeCaseTest, ResetDuringProcessing) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "static"
    );

    // Process some frames
    for (int i = 0; i < 10; i++) {
        stabilizer->process_frame(frames[i]);
    }

    // Reset stabilizer
    stabilizer->reset();

    // Should work normally after reset
    cv::Mat result = stabilizer->process_frame(frames[0]);
    EXPECT_FALSE(result.empty());
}

TEST_F(EdgeCaseTest, ReinitializeWithDifferentParams) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Process some frames
    auto frames = TestDataGenerator::generate_test_sequence(
        5, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );
    for (const auto& frame : frames) {
        stabilizer->process_frame(frame);
    }

    // Reinitialize with different parameters
    params.smoothing_radius = 50;
    params.feature_count = 200;
    stabilizer->update_parameters(params);

    // Should work normally after parameter update
    cv::Mat result = stabilizer->process_frame(frames[0]);
    EXPECT_FALSE(result.empty());
}

// ============================================================================
// All Black Frame Tests
// ============================================================================

TEST_F(EdgeCaseTest, AllBlackFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat black_frame = cv::Mat::zeros(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC4);

    cv::Mat result = stabilizer->process_frame(black_frame);

    // Should handle all-black frames gracefully
    EXPECT_FALSE(result.empty());
}

TEST_F(EdgeCaseTest, AllWhiteFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat white_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC4, cv::Scalar(255, 255, 255, 255));

    cv::Mat result = stabilizer->process_frame(white_frame);

    // Should handle all-white frames gracefully
    EXPECT_FALSE(result.empty());
}

// ============================================================================
// Frame Conversion Edge Cases
// ============================================================================

TEST_F(EdgeCaseTest, FrameUtilsEmptyMatValidation) {
    cv::Mat empty_mat;
    EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(empty_mat));
}

TEST_F(EdgeCaseTest, FrameUtilsInvalidDimensions) {
    // Test zero dimensions - this should fail validation
    cv::Mat invalid_frame(0, 0, CV_8UC4);
    EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(invalid_frame));

    // OpenCV throws an exception when creating a Mat with negative dimensions
    // This is expected behavior - OpenCV validates dimensions at construction time
    // We test that our code handles this gracefully by catching the exception
    try {
        cv::Mat invalid_frame2(100, -50, CV_8UC4);
        // If we get here, verify the validation catches it
        EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(invalid_frame2));
    } catch (const cv::Exception& e) {
        // OpenCV threw an exception during Mat construction, which is expected
        // This is acceptable behavior - the invalid frame is rejected before validation
        SUCCEED() << "OpenCV correctly rejected negative dimensions at construction time";
    }
}

TEST_F(EdgeCaseTest, FrameUtilsInvalidChannels) {
    cv::Mat invalid_frame(100, 100, CV_8UC2);
    EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(invalid_frame));
}

TEST_F(EdgeCaseTest, ColorConversionGrayscale) {
    cv::Mat bgra_frame(100, 100, CV_8UC4);
    cv::Mat gray = FRAME_UTILS::ColorConversion::convert_to_grayscale(bgra_frame);
    EXPECT_FALSE(gray.empty());
    EXPECT_EQ(gray.channels(), 1);
}

TEST_F(EdgeCaseTest, ColorConversionEmptyFrame) {
    cv::Mat empty_frame;
    cv::Mat gray = FRAME_UTILS::ColorConversion::convert_to_grayscale(empty_frame);
    EXPECT_TRUE(gray.empty());
}
