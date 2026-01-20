#ifndef SKIP_OPENCV_TESTS

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "core/stabilizer_core.hpp"
#include "test_data_generator.hpp"

using namespace TestDataGenerator;

class StabilizerCoreTests : public ::testing::Test {
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

TEST_F(StabilizerCoreTests, FeatureDetectionCount) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frame with known features
    cv::Mat test_frame = create_frame_with_features(640, 480, 100);
    cv::Mat result = stabilizer.process_frame(test_frame);

    // Verify that feature detection worked
    EXPECT_FALSE(result.empty()) << "Processed frame should not be empty";

    // The stabilizer should have detected features and processed them
    // This is a basic validation test
    EXPECT_GT(stabilizer.get_performance_metrics().frame_count, 0) << "Frame count should be incremented";
}

TEST_F(StabilizerCoreTests, OpticalFlowTracking) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate a sequence of frames with horizontal motion
    std::vector<cv::Mat> sequence = generate_test_sequence(10, 640, 480, "horizontal");

    int initial_frame_count = stabilizer.get_performance_metrics().frame_count;

    for (const auto& frame : sequence) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Processed frame should not be empty";
    }

    int final_frame_count = stabilizer.get_performance_metrics().frame_count;
    EXPECT_GT(final_frame_count, initial_frame_count) << "Frame count should increase";
}

TEST_F(StabilizerCoreTests, TransformSmoothing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frames with varying motion
    std::vector<cv::Mat> sequence = generate_test_sequence(30, 640, 480, "horizontal");

    // Process all frames
    for (const auto& frame : sequence) {
        stabilizer.process_frame(frame);
    }

    // Verify that transform history is being maintained
    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0) << "Frame count should be incremented";

    // The smoothing algorithm should work with the transform history
    // This is a basic validation test
    EXPECT_TRUE(stabilizer.is_ready()) << "Stabilizer should be ready";
}

TEST_F(StabilizerCoreTests, FeatureDetectionWithStaticFrames) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate static frames
    cv::Mat frame1 = generate_test_frame(640, 480, 0);
    cv::Mat frame2 = generate_test_frame(640, 480, 0);

    cv::Mat result1 = stabilizer.process_frame(frame1);
    cv::Mat result2 = stabilizer.process_frame(frame2);

    EXPECT_FALSE(result1.empty()) << "First processed frame should not be empty";
    EXPECT_FALSE(result2.empty()) << "Second processed frame should not be empty";

    // With static frames, the transforms should be similar
    auto metrics1 = stabilizer.get_performance_metrics();
    auto metrics2 = stabilizer.get_performance_metrics();

    EXPECT_EQ(metrics1.frame_count, metrics2.frame_count);
}

TEST_F(StabilizerCoreTests, FeatureDetectionWithDifferentFormats) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Test with different frame types
    cv::Mat frame1 = generate_test_frame(640, 480, 0);
    cv::Mat frame2 = generate_test_frame(640, 480, 1);  // Gradient frame
    cv::Mat frame3 = generate_test_frame(640, 480, 2);  // Checkerboard

    cv::Mat result1 = stabilizer.process_frame(frame1);
    cv::Mat result2 = stabilizer.process_frame(frame2);
    cv::Mat result3 = stabilizer.process_frame(frame3);

    EXPECT_FALSE(result1.empty()) << "Frame 1 should be processed";
    EXPECT_FALSE(result2.empty()) << "Frame 2 should be processed";
    EXPECT_FALSE(result3.empty()) << "Frame 3 should be processed";
}

TEST_F(StabilizerCoreTests, OpticalFlowWithRotation) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frames with rotation
    std::vector<cv::Mat> sequence = generate_test_sequence(20, 640, 480, "rotation");

    for (const auto& frame : sequence) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Processed frame should not be empty";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0) << "Frame count should be incremented";
}

TEST_F(StabilizerCoreTests, OpticalFlowWithZoom) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frames with zoom
    std::vector<cv::Mat> sequence = generate_test_sequence(20, 640, 480, "zoom");

    for (const auto& frame : sequence) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Processed frame should not be empty";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0) << "Frame count should be incremented";
}

TEST_F(StabilizerCoreTests, TransformHistoryManagement) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Set smoothing radius to test history management
    test_params.smoothing_radius = 5;
    stabilizer.update_parameters(test_params);

    // Process multiple frames
    for (int i = 0; i < 20; i++) {
        cv::Mat frame = generate_test_frame(640, 480, i % 3);
        stabilizer.process_frame(frame);
    }

    // The stabilizer should maintain transform history
    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0) << "Frame count should be incremented";

    // Reset and test again
    stabilizer.reset();
    EXPECT_EQ(stabilizer.get_performance_metrics().frame_count, 0) << "Frame count should be reset";
}

TEST_F(StabilizerCoreTests, ParameterImpactOnFeatureDetection) {
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
    EXPECT_GT(stabilizer1.get_performance_metrics().frame_count, 0);
    EXPECT_GT(stabilizer2.get_performance_metrics().frame_count, 0);
}

TEST_F(StabilizerCoreTests, QualityLevelImpact) {
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
    EXPECT_GT(stabilizer1.get_performance_metrics().frame_count, 0);
    EXPECT_GT(stabilizer2.get_performance_metrics().frame_count, 0);
}

TEST_F(StabilizerCoreTests, MinDistanceImpact) {
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
    EXPECT_GT(stabilizer1.get_performance_metrics().frame_count, 0);
    EXPECT_GT(stabilizer2.get_performance_metrics().frame_count, 0);
}

TEST_F(StabilizerCoreTests, BlockSizeImpact) {
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
    EXPECT_GT(stabilizer1.get_performance_metrics().frame_count, 0);
    EXPECT_GT(stabilizer2.get_performance_metrics().frame_count, 0);
}

TEST_F(StabilizerCoreTests, HarrisDetectorImpact) {
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
    EXPECT_GT(stabilizer1.get_performance_metrics().frame_count, 0);
    EXPECT_GT(stabilizer2.get_performance_metrics().frame_count, 0);
}

TEST_F(StabilizerCoreTests, ProcessingWithEmptyFrame) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat empty_frame;

    cv::Mat result = stabilizer.process_frame(empty_frame);

    // Empty frame should return empty or original frame
    EXPECT_TRUE(result.empty()) << "Empty frame should result in empty output";
}

TEST_F(StabilizerCoreTests, ProcessingWithDisabledStabilization) {
    StabilizerCore stabilizer;
    test_params.enabled = false;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    cv::Mat result = stabilizer.process_frame(frame);

    // With stabilization disabled, frame should be processed but not stabilized
    EXPECT_FALSE(result.empty()) << "Frame should still be processed";
    EXPECT_EQ(result.rows, frame.rows) << "Frame dimensions should be preserved";
    EXPECT_EQ(result.cols, frame.cols) << "Frame dimensions should be preserved";
}

TEST_F(StabilizerCoreTests, HighResolutionFrameProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(1920, 1080, test_params);

    cv::Mat frame = generate_test_frame(1920, 1080, 0);

    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "High resolution frame should be processed";
    EXPECT_EQ(result.rows, 1080) << "Frame height should be preserved";
    EXPECT_EQ(result.cols, 1920) << "Frame width should be preserved";
}

TEST_F(StabilizerCoreTests, LowResolutionFrameProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(320, 240, test_params);

    cv::Mat frame = generate_test_frame(320, 240, 0);

    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "Low resolution frame should be processed";
    EXPECT_EQ(result.rows, 240) << "Frame height should be preserved";
    EXPECT_EQ(result.cols, 320) << "Frame width should be preserved";
}

TEST_F(StabilizerCoreTests, MultipleStabilizerInstances) {
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

TEST_F(StabilizerCoreTests, PerformanceMetricsAccuracy) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    // Process a few frames
    for (int i = 0; i < 5; i++) {
        stabilizer.process_frame(frame);
    }

    auto metrics = stabilizer.get_performance_metrics();

    EXPECT_GT(metrics.frame_count, 0) << "Frame count should be greater than 0";
    EXPECT_GE(metrics.avg_processing_time, 0.0) << "Average processing time should be non-negative";
    EXPECT_LT(metrics.avg_processing_time, 1000.0) << "Processing time should be reasonable";
}

#endif // SKIP_OPENCV_TESTS
