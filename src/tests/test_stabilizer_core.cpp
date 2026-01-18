#ifndef SKIP_OPENCV_TESTS

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "core/stabilizer_core.hpp"

class StabilizerCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test parameters
        params.enabled = true;
        params.smoothing_radius = 10;
        params.max_correction = 20.0f;
        params.feature_count = 200;
        params.quality_level = 0.01f;
        params.min_distance = 10.0f;
        params.block_size = 3;
        params.use_harris = false;
        params.k = 0.04f;
        params.debug_mode = false;
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    StabilizerCore::StabilizerParams params;
};

TEST_F(StabilizerCoreTest, Initialization) {
    StabilizerCore stabilizer;
    
    // Test successful initialization
    bool result = stabilizer.initialize(640, 480, params);
    EXPECT_TRUE(result) << "Failed to initialize stabilizer";
    
    // Test initialization with invalid parameters
    StabilizerCore::StabilizerParams invalid_params = params;
    invalid_params.smoothing_radius = 0; // Invalid
    bool invalid_result = stabilizer.initialize(640, 480, invalid_params);
    EXPECT_FALSE(invalid_result) << "Should fail with invalid parameters";
}

TEST_F(StabilizerCoreTest, ParameterValidation) {
    // Test valid parameters
    EXPECT_TRUE(StabilizerCore::validate_parameters(params)) << "Valid parameters should pass validation";
    
    // Test invalid parameters
    StabilizerCore::StabilizerParams invalid_params = params;
    invalid_params.smoothing_radius = 0;
    EXPECT_FALSE(StabilizerCore::validate_parameters(invalid_params)) << "Invalid smoothing radius should fail";
    
    invalid_params = params;
    invalid_params.feature_count = 10; // Too low
    EXPECT_FALSE(StabilizerCore::validate_parameters(invalid_params)) << "Invalid feature count should fail";
}

TEST_F(StabilizerCoreTest, FrameValidation) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, params);
    
    // Test valid frame
    cv::Mat valid_frame = cv::Mat::zeros(480, 640, CV_8UC4);
    EXPECT_TRUE(stabilizer.validate_frame(valid_frame)) << "Valid frame should pass validation";
    
    // Test invalid frames
    cv::Mat empty_frame;
    EXPECT_FALSE(stabilizer.validate_frame(empty_frame)) << "Empty frame should fail validation";
    
    cv::Mat tiny_frame = cv::Mat::zeros(10, 10, CV_8UC4);
    EXPECT_FALSE(stabilizer.validate_frame(tiny_frame)) << "Tiny frame should fail validation";
    
    cv::Mat huge_frame = cv::Mat::zeros(10000, 10000, CV_8UC4);
    EXPECT_FALSE(stabilizer.validate_frame(huge_frame)) << "Huge frame should fail validation";
}

TEST_F(StabilizerCoreTest, FrameProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, params);
    
    // Create a test frame with some features
    cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
    
    // Add some simple features (white squares)
    cv::rectangle(test_frame, cv::Point(100, 100), cv::Point(150, 150), cv::Scalar(255, 255, 255, 255), -1);
    cv::rectangle(test_frame, cv::Point(200, 200), cv::Point(250, 250), cv::Scalar(255, 255, 255, 255), -1);
    cv::rectangle(test_frame, cv::Point(300, 300), cv::Point(350, 350), cv::Scalar(255, 255, 255, 255), -1);
    
    // Process the frame
    cv::Mat result = stabilizer.process_frame(test_frame);
    
    // Basic validation
    EXPECT_FALSE(result.empty()) << "Processed frame should not be empty";
    EXPECT_EQ(result.rows, test_frame.rows) << "Output should have same height as input";
    EXPECT_EQ(result.cols, test_frame.cols) << "Output should have same width as input";
    EXPECT_EQ(result.type(), test_frame.type()) << "Output should have same type as input";
}

TEST_F(StabilizerCoreTest, PresetConfiguration) {
    // Test gaming preset
    auto gaming_params = StabilizerCore::get_preset_gaming();
    EXPECT_TRUE(StabilizerCore::validate_parameters(gaming_params)) << "Gaming preset should be valid";
    EXPECT_GT(gaming_params.smoothing_radius, 20) << "Gaming preset should have higher smoothing";
    
    // Test streaming preset
    auto streaming_params = StabilizerCore::get_preset_streaming();
    EXPECT_TRUE(StabilizerCore::validate_parameters(streaming_params)) << "Streaming preset should be valid";
    EXPECT_LT(streaming_params.smoothing_radius, gaming_params.smoothing_radius) << "Streaming preset should have lower smoothing than gaming";
    
    // Test recording preset
    auto recording_params = StabilizerCore::get_preset_recording();
    EXPECT_TRUE(StabilizerCore::validate_parameters(recording_params)) << "Recording preset should be valid";
    EXPECT_GT(recording_params.feature_count, streaming_params.feature_count) << "Recording preset should have more features";
}

TEST_F(StabilizerCoreTest, ThreadSafety) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, params);
    
    // Create multiple threads that access the stabilizer
    // This is a basic test - more comprehensive thread safety testing would be needed
    std::vector<std::thread> threads;
    std::vector<cv::Mat> results(10);
    
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&stabilizer, i, &results]() {
            cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
            results[i] = stabilizer.process_frame(test_frame);
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Check that all threads completed successfully
    for (const auto& result : results) {
        EXPECT_FALSE(result.empty()) << "All threads should complete successfully";
    }
}

TEST_F(StabilizerCoreTest, PerformanceMetrics) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, params);
    
    // Process a few frames
    for (int i = 0; i < 5; i++) {
        cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
        stabilizer.process_frame(test_frame);
    }
    
    // Get performance metrics
    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0) << "Frame count should be incremented";
    EXPECT_GE(metrics.avg_processing_time, 0) << "Average processing time should be non-negative";
}

#endif // SKIP_OPENCV_TESTS