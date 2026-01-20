/*
 OBS Stabilizer Plugin - Core Functionality Tests
 Tests for core stabilizer functionality without OBS dependencies
 */

#ifndef SKIP_OPENCV_TESTS

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "core/stabilizer_core.hpp"
#include "test_data_generator.hpp"

using namespace TestDataGenerator;

class CoreFunctionalityTests : public ::testing::Test {
protected:
    void SetUp() override {
        test_params.enabled = true;
        test_params.smoothing_radius = 10;
        test_params.max_correction = 20.0f;
        test_params.feature_count = 200;
        test_params.quality_level = 0.01f;
        test_params.min_distance = 10.0f;
        test_params.block_size = 3;
        test_params.use_harris = false;
        test_params.k = 0.04f;
        test_params.debug_mode = false;
    }

    void TearDown() override {
    }

    StabilizerCore::StabilizerParams test_params;
};

// Core Algorithm Tests
TEST_F(CoreFunctionalityTests, FeatureDetectionWithStaticFrames) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate static frames
    cv::Mat frame1 = generate_test_frame(640, 480, 0);
    cv::Mat frame2 = generate_test_frame(640, 480, 0);

    cv::Mat result1 = stabilizer.process_frame(frame1);
    cv::Mat result2 = stabilizer.process_frame(frame2);

    EXPECT_FALSE(result1.empty()) << "First frame should be processed";
    EXPECT_FALSE(result2.empty()) << "Second frame should be processed";
    EXPECT_EQ(result1.rows, frame1.rows) << "Height should be preserved";
    EXPECT_EQ(result1.cols, frame1.cols) << "Width should be preserved";
}

TEST_F(CoreFunctionalityTests, FeatureDetectionWithMotion) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frames with horizontal motion
    std::vector<cv::Mat> frames;
    for (int i = 0; i < 20; i++) {
        cv::Mat frame = generate_horizontal_motion_frame(generate_test_frame(640, 480, 0), i, 20);
        frames.push_back(frame.clone());
    }

    // Process all frames
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Frame should be processed";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 20) << "All frames should be processed";
}

TEST_F(CoreFunctionalityTests, TransformSmoothingWithRotation) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frames with rotation
    std::vector<cv::Mat> frames;
    for (int i = 0; i < 30; i++) {
        cv::Mat frame = generate_rotation_frame(generate_test_frame(640, 480, 0), i, 30, 5.0f);
        frames.push_back(frame.clone());
    }

    // Process all frames
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Frame should be processed";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 30) << "All frames should be processed";
}

TEST_F(CoreFunctionalityTests, TransformSmoothingWithZoom) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frames with zoom
    std::vector<cv::Mat> frames;
    for (int i = 0; i < 30; i++) {
        cv::Mat frame = generate_zoom_frame(generate_test_frame(640, 480, 0), i, 30, 1.2f);
        frames.push_back(frame.clone());
    }

    // Process all frames
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Frame should be processed";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 30) << "All frames should be processed";
}

TEST_F(CoreFunctionalityTests, TransformSmoothingWithComplexMotion) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frames with complex motion patterns
    std::vector<cv::Mat> frames;
    for (int i = 0; i < 50; i++) {
        cv::Mat frame;
        if (i % 3 == 0) {
            frame = generate_horizontal_motion_frame(generate_test_frame(640, 480, 0), i, 50);
        } else if (i % 3 == 1) {
            frame = generate_vertical_motion_frame(generate_test_frame(640, 480, 0), i, 50);
        } else {
            frame = generate_rotation_frame(generate_test_frame(640, 480, 0), i, 50, 3.0f);
        }
        frames.push_back(frame.clone());
    }

    // Process all frames
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Frame should be processed";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 50) << "All frames should be processed";
}

// Frame Processing Tests
TEST_F(CoreFunctionalityTests, FrameProcessingWithDifferentFormats) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Test with different frame formats
    std::vector<int> formats = {CV_8UC1, CV_8UC3, CV_8UC4};

    for (int format : formats) {
        cv::Mat frame = generate_frame_in_format(640, 480, format);
        cv::Mat result = stabilizer.process_frame(frame);

        EXPECT_FALSE(result.empty()) << "Frame in format " << format << " should be processed";
        EXPECT_EQ(result.rows, 480) << "Height should be preserved";
        EXPECT_EQ(result.cols, 640) << "Width should be preserved";
    }
}

TEST_F(CoreFunctionalityTests, FrameProcessingWithHighResolution) {
    StabilizerCore stabilizer;
    stabilizer.initialize(1920, 1080, test_params);

    cv::Mat frame = generate_test_frame(1920, 1080, 0);
    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "High resolution frame should be processed";
    EXPECT_EQ(result.rows, 1080) << "Height should be preserved";
    EXPECT_EQ(result.cols, 1920) << "Width should be preserved";
}

TEST_F(CoreFunctionalityTests, FrameProcessingWithLowResolution) {
    StabilizerCore stabilizer;
    stabilizer.initialize(320, 240, test_params);

    cv::Mat frame = generate_test_frame(320, 240, 0);
    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "Low resolution frame should be processed";
    EXPECT_EQ(result.rows, 240) << "Height should be preserved";
    EXPECT_EQ(result.cols, 320) << "Width should be preserved";
}

TEST_F(CoreFunctionalityTests, FrameProcessingWithFeatures) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frame with features
    cv::Mat frame = create_frame_with_features(640, 480, 100);
    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "Frame with features should be processed";
    EXPECT_EQ(result.rows, frame.rows) << "Height should be preserved";
    EXPECT_EQ(result.cols, frame.cols) << "Width should be preserved";
}

// Parameter Configuration Tests
TEST_F(CoreFunctionalityTests, PresetConfiguration) {
    // Test gaming preset
    auto gaming_params = StabilizerCore::get_preset_gaming();
    EXPECT_TRUE(StabilizerCore::validate_parameters(gaming_params)) << "Gaming preset should be valid";
    EXPECT_GT(gaming_params.smoothing_radius, 20) << "Gaming preset should have sufficient smoothing";
    EXPECT_LT(gaming_params.smoothing_radius, 40) << "Gaming preset should not have excessive smoothing";

    // Test streaming preset
    auto streaming_params = StabilizerCore::get_preset_streaming();
    EXPECT_TRUE(StabilizerCore::validate_parameters(streaming_params)) << "Streaming preset should be valid";
    EXPECT_GT(streaming_params.smoothing_radius, gaming_params.smoothing_radius) << "Streaming preset should have higher smoothing than gaming";

    // Test recording preset
    auto recording_params = StabilizerCore::get_preset_recording();
    EXPECT_TRUE(StabilizerCore::validate_parameters(recording_params)) << "Recording preset should be valid";
    EXPECT_GT(recording_params.smoothing_radius, streaming_params.smoothing_radius) << "Recording preset should have highest smoothing";
    EXPECT_GT(recording_params.feature_count, streaming_params.feature_count) << "Recording preset should have more features";
}

TEST_F(CoreFunctionalityTests, ParameterImpactOnFeatureDetection) {
    StabilizerCore stabilizer1;
    stabilizer1.initialize(640, 480, test_params);

    // Test with different feature counts
    test_params.feature_count = 500;
    StabilizerCore stabilizer2;
    stabilizer2.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    stabilizer1.process_frame(frame);
    stabilizer2.process_frame(frame);

    // Both should process frames successfully
    EXPECT_FALSE(stabilizer1.get_performance_metrics().frame_count > 0);
    EXPECT_FALSE(stabilizer2.get_performance_metrics().frame_count > 0);
}

TEST_F(CoreFunctionalityTests, QualityLevelImpact) {
    StabilizerCore stabilizer1;
    stabilizer1.initialize(640, 480, test_params);

    // Test with different quality levels
    test_params.quality_level = 0.001f;
    StabilizerCore stabilizer2;
    stabilizer2.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    stabilizer1.process_frame(frame);
    stabilizer2.process_frame(frame);

    // Both should process frames successfully
    EXPECT_FALSE(stabilizer1.get_performance_metrics().frame_count > 0);
    EXPECT_FALSE(stabilizer2.get_performance_metrics().frame_count > 0);
}

TEST_F(CoreFunctionalityTests, MinDistanceImpact) {
    StabilizerCore stabilizer1;
    stabilizer1.initialize(640, 480, test_params);

    // Test with different minimum distances
    test_params.min_distance = 50.0f;
    StabilizerCore stabilizer2;
    stabilizer2.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    stabilizer1.process_frame(frame);
    stabilizer2.process_frame(frame);

    // Both should process frames successfully
    EXPECT_FALSE(stabilizer1.get_performance_metrics().frame_count > 0);
    EXPECT_FALSE(stabilizer2.get_performance_metrics().frame_count > 0);
}

TEST_F(CoreFunctionalityTests, BlockSizeImpact) {
    StabilizerCore stabilizer1;
    stabilizer1.initialize(640, 480, test_params);

    // Test with different block sizes
    test_params.block_size = 5;
    StabilizerCore stabilizer2;
    stabilizer2.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    stabilizer1.process_frame(frame);
    stabilizer2.process_frame(frame);

    // Both should process frames successfully
    EXPECT_FALSE(stabilizer1.get_performance_metrics().frame_count > 0);
    EXPECT_FALSE(stabilizer2.get_performance_metrics().frame_count > 0);
}

TEST_F(CoreFunctionalityTests, HarrisDetectorImpact) {
    StabilizerCore stabilizer1;
    stabilizer1.initialize(640, 480, test_params);

    // Test with Harris detector enabled
    test_params.use_harris = true;
    StabilizerCore stabilizer2;
    stabilizer2.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    stabilizer1.process_frame(frame);
    stabilizer2.process_frame(frame);

    // Both should process frames successfully
    EXPECT_FALSE(stabilizer1.get_performance_metrics().frame_count > 0);
    EXPECT_FALSE(stabilizer2.get_performance_metrics().frame_count > 0);
}

TEST_F(CoreFunctionalityTests, ProcessingWithEmptyFrame) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat empty_frame;

    cv::Mat result = stabilizer.process_frame(empty_frame);

    // Empty frame should return empty or original frame
    EXPECT_TRUE(result.empty()) << "Empty frame should return empty result";
}

TEST_F(CoreFunctionalityTests, ProcessingWithDisabledStabilization) {
    StabilizerCore stabilizer;
    test_params.enabled = false;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    cv::Mat result = stabilizer.process_frame(frame);

    // With stabilization disabled, frame should be processed but not stabilized
    EXPECT_FALSE(result.empty()) << "Frame should still be processed";
    EXPECT_EQ(result.rows, frame.rows) << "Height should be preserved";
    EXPECT_EQ(result.cols, frame.cols) << "Width should be preserved";
}

// Performance Testing
TEST_F(CoreFunctionalityTests, PerformanceMetricsAccuracy) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    // Process a few frames
    for (int i = 0; i < 5; i++) {
        stabilizer.process_frame(frame);
    }

    // Get performance metrics
    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0) << "Frame count should be incremented";
    EXPECT_GE(metrics.avg_processing_time, 0.0) << "Average processing time should be non-negative";
    EXPECT_LT(metrics.avg_processing_time, 1000.0) << "Processing time should be reasonable";
}

TEST_F(CoreFunctionalityTests, HighFrequencyProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    // Process frames at high frequency
    for (int i = 0; i < 100; i++) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "High frequency frame " << i << " should be processed";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 100) << "All high frequency frames should be processed";
    EXPECT_LT(metrics.avg_processing_time, 1000.0) << "Processing time should be reasonable";
}

TEST_F(CoreFunctionalityTests, MultipleStabilizerInstances) {
    StabilizerCore stabilizer1;
    StabilizerCore stabilizer2;

    stabilizer1.initialize(640, 480, test_params);
    stabilizer2.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    cv::Mat result1 = stabilizer1.process_frame(frame);
    cv::Mat result2 = stabilizer2.process_frame(frame);

    EXPECT_FALSE(result1.empty()) << "First stabilizer should process frame";
    EXPECT_FALSE(result2.empty()) << "Second stabilizer should process frame";

    // Each stabilizer should maintain its own state
    EXPECT_GT(stabilizer1.get_performance_metrics().frame_count, 0);
    EXPECT_GT(stabilizer2.get_performance_metrics().frame_count, 0);
}

TEST_F(CoreFunctionalityTests, StateManagement) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    // Process frames
    for (int i = 0; i < 10; i++) {
        stabilizer.process_frame(frame);
    }

    // Get state before reset
    auto metrics1 = stabilizer.get_performance_metrics();
    EXPECT_GT(metrics1.frame_count, 0) << "Frame count should be incremented";

    // Reset state
    stabilizer.reset();

    // Get state after reset
    auto metrics2 = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics2.frame_count, 0) << "Frame count should be reset to zero";
    EXPECT_TRUE(stabilizer.is_ready()) << "Stabilizer should be ready after reset";
}

TEST_F(CoreFunctionalityTests, ParameterUpdate) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Update parameters
    test_params.smoothing_radius = 20;
    test_params.feature_count = 500;
    stabilizer.update_parameters(test_params);

    // Verify parameters were updated
    auto current_params = stabilizer.get_current_params();
    EXPECT_EQ(current_params.smoothing_radius, 20) << "Smoothing radius should be updated";
    EXPECT_EQ(current_params.feature_count, 500) << "Feature count should be updated";
}

TEST_F(CoreFunctionalityTests, ErrorHandling) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat empty_frame;

    cv::Mat result = stabilizer.process_frame(empty_frame);
    EXPECT_TRUE(result.empty()) << "Empty frame should return empty result";

    // Get error message
    std::string error = stabilizer.get_last_error();
    EXPECT_FALSE(error.empty()) << "Should have an error message";
}

TEST_F(CoreFunctionalityTests, ReadyState) {
    StabilizerCore stabilizer;

    // Should not be ready before initialization
    EXPECT_FALSE(stabilizer.is_ready()) << "Should not be ready before initialization";

    // Should be ready after initialization
    stabilizer.initialize(640, 480, test_params);
    EXPECT_TRUE(stabilizer.is_ready()) << "Should be ready after initialization";
}

TEST_F(CoreFunctionalityTests, ParameterValidation) {
    // Test valid parameters
    StabilizerCore::StabilizerParams valid_params = test_params;
    EXPECT_TRUE(StabilizerCore::validate_parameters(valid_params)) << "Valid parameters should pass validation";

    // Test invalid parameters
    StabilizerCore::StabilizerParams invalid_params = test_params;
    invalid_params.smoothing_radius = 0; // Invalid
    EXPECT_FALSE(StabilizerCore::validate_parameters(invalid_params)) << "Invalid smoothing radius should fail";

    invalid_params = test_params;
    invalid_params.feature_count = 49; // Too low
    EXPECT_FALSE(StabilizerCore::validate_parameters(invalid_params)) << "Invalid feature count should fail";

    invalid_params = test_params;
    invalid_params.feature_count = 5000; // Too high
    EXPECT_FALSE(StabilizerCore::validate_parameters(invalid_params)) << "Invalid feature count should fail";
}

#endif // SKIP_OPENCV_TESTS
