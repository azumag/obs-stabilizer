#ifndef SKIP_OPENCV_TESTS

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "core/stabilizer_core.hpp"
#include "core/stabilizer_wrapper.hpp"

class OBSIntegrationTests : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(OBSIntegrationTests, FilterPropertyHandling) {
    StabilizerWrapper wrapper;

    // Test that filter can be created
    EXPECT_TRUE(wrapper.initialize(640, 480, 30.0f));

    // Test that properties can be retrieved
    auto props = wrapper.get_properties();
    EXPECT_NE(props, nullptr);

    // Test that filter is ready
    EXPECT_TRUE(wrapper.is_initialized());
}

TEST_F(OBSIntegrationTests, ConfigurationUpdates) {
    StabilizerWrapper wrapper;

    // Initialize with default parameters
    EXPECT_TRUE(wrapper.initialize(640, 480, 30.0f));

    // Update parameters
    StabilizerCore::StabilizerParams params;
    params.enabled = true;
    params.smoothing_radius = 20;
    params.max_correction = 30.0f;
    params.feature_count = 500;

    wrapper.update_parameters(params);

    // Verify parameters were updated
    auto current_params = wrapper.get_current_params();
    EXPECT_EQ(current_params.smoothing_radius, 20);
    EXPECT_FLOAT_EQ(current_params.max_correction, 30.0f);
    EXPECT_EQ(current_params.feature_count, 500);
}

TEST_F(OBSIntegrationTests, PluginLifecycle) {
    // Test filter creation
    StabilizerWrapper wrapper;
    EXPECT_TRUE(wrapper.initialize(640, 480, 30.0f));

    // Test filter is ready
    EXPECT_TRUE(wrapper.is_initialized());

    // Test filter can process frames
    cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
    cv::Mat result = wrapper.process_frame(test_frame);
    EXPECT_FALSE(result.empty());

    // Test filter reset
    wrapper.reset();
    EXPECT_FALSE(wrapper.is_initialized());

    // Test filter can be re-initialized
    EXPECT_TRUE(wrapper.initialize(640, 480, 30.0f));
    EXPECT_TRUE(wrapper.is_initialized());
}

// TEST_F(OBSIntegrationTests, ParameterValidation) {
//     StabilizerWrapper wrapper;
// 
//     // Test valid parameters
//     StabilizerCore::StabilizerParams valid_params;
//     valid_params.enabled = true;
//     valid_params.smoothing_radius = 10;
//     valid_params.max_correction = 20.0f;
//     valid_params.feature_count = 200;
//     valid_params.quality_level = 0.01f;
//     valid_params.min_distance = 10.0f;
//     valid_params.block_size = 3;
//     valid_params.use_harris = false;
//     valid_params.k = 0.04f;
//     valid_params.debug_mode = false;
// 
//     EXPECT_TRUE(wrapper.validate_parameters(valid_params));
// 
//     // Test invalid parameters
//     StabilizerCore::StabilizerParams invalid_params = valid_params;
//     invalid_params.smoothing_radius = 0; // Invalid
//     EXPECT_FALSE(wrapper.validate_parameters(invalid_params));
// 
//     invalid_params = valid_params;
//     invalid_params.feature_count = 10; // Too low
//     EXPECT_FALSE(wrapper.validate_parameters(invalid_params));
// 
//     invalid_params = valid_params;
//     invalid_params.feature_count = 5000; // Too high
//     EXPECT_FALSE(wrapper.validate_parameters(invalid_params));
// }

// TEST_F(OBSIntegrationTests, PresetConfiguration) {
//     StabilizerWrapper wrapper;
//     wrapper.initialize(640, 480, 30.0f);
// 
//     // Test gaming preset
//     auto gaming_params = wrapper.get_preset_gaming();
//     EXPECT_TRUE(wrapper.validate_parameters(gaming_params));
//     EXPECT_GT(gaming_params.smoothing_radius, 20) << "Gaming preset should have higher smoothing";
// 
//     // Test streaming preset
//     auto streaming_params = wrapper.get_preset_streaming();
//     EXPECT_TRUE(wrapper.validate_parameters(streaming_params));
//     EXPECT_LT(streaming_params.smoothing_radius, gaming_params.smoothing_radius) << "Streaming preset should have lower smoothing than gaming";
// 
//     // Test recording preset
//     auto recording_params = wrapper.get_preset_recording();
//     EXPECT_TRUE(wrapper.validate_parameters(recording_params));
//     EXPECT_GT(recording_params.feature_count, streaming_params.feature_count) << "Recording preset should have more features";
// }

TEST_F(OBSIntegrationTests, MultipleInstances) {
    StabilizerWrapper wrapper1;
    StabilizerWrapper wrapper2;

    wrapper1.initialize(640, 480, 30.0f);
    wrapper2.initialize(640, 480, 30.0f);

    cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);

    wrapper1.process_frame(test_frame);
    wrapper2.process_frame(test_frame);

    // Each instance should maintain its own state
    EXPECT_TRUE(wrapper1.is_ready());
    EXPECT_TRUE(wrapper2.is_ready());

    // Reset one instance
    wrapper1.reset();
    EXPECT_FALSE(wrapper1.is_ready());

    // Other instance should still work
    EXPECT_TRUE(wrapper2.is_ready());
}

TEST_F(OBSIntegrationTests, PerformanceMetrics) {
    StabilizerWrapper wrapper;
    wrapper.initialize(640, 480, 30.0f);

    cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);

    // Process a few frames
    for (int i = 0; i < 5; i++) {
        wrapper.process_frame(test_frame);
    }

    // Get performance metrics
    auto metrics = wrapper.get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0) << "Frame count should be incremented";
    EXPECT_GE(metrics.avg_processing_time, 0.0) << "Average processing time should be non-negative";
    EXPECT_LT(metrics.avg_processing_time, 1000.0) << "Processing time should be reasonable";
}

TEST_F(OBSIntegrationTests, FrameProcessingWithDifferentFormats) {
    StabilizerWrapper wrapper;
    wrapper.initialize(640, 480, 30.0f);

    // Test with different frame types
    std::vector<int> formats = {CV_8UC1, CV_8UC3, CV_8UC4};

    for (int format : formats) {
        cv::Mat frame = cv::Mat::zeros(480, 640, format);
        cv::Mat result = wrapper.process_frame(frame);

        EXPECT_FALSE(result.empty()) << "Frame in format " << format << " should be processed";
        EXPECT_EQ(result.rows, 480) << "Height should be preserved";
        EXPECT_EQ(result.cols, 640) << "Width should be preserved";
    }
}

TEST_F(OBSIntegrationTests, ErrorHandling) {
    StabilizerWrapper wrapper;

    // Test processing with empty frame
    cv::Mat empty_frame;
    cv::Mat result = wrapper.process_frame(empty_frame);
    EXPECT_TRUE(result.empty()) << "Empty frame should return empty result";

    // Test processing with NULL frame
    result = wrapper.process_frame(nullptr);
    EXPECT_TRUE(result.empty()) << "NULL frame should return empty result";

    // Test processing with invalid parameters
    StabilizerCore::StabilizerParams invalid_params;
    invalid_params.smoothing_radius = 0;
    wrapper.update_parameters(invalid_params);

    cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
    result = wrapper.process_frame(test_frame);
    EXPECT_FALSE(result.empty()) << "Frame should still be processed even with invalid parameters";
}

TEST_F(OBSIntegrationTests, FrameSizeAdaptation) {
    StabilizerWrapper wrapper;

    // Test with different resolutions
    std::vector<std::pair<int, int>> resolutions = {
        {320, 240},
        {640, 480},
        {1280, 720},
        {1920, 1080}
    };

    for (const auto& resolution : resolutions) {
        wrapper.initialize(resolution.first, resolution.second, 30.0f);
        EXPECT_TRUE(wrapper.is_initialized());

        cv::Mat frame = cv::Mat::zeros(resolution.second, resolution.first, CV_8UC4);
        cv::Mat result = wrapper.process_frame(frame);

        EXPECT_FALSE(result.empty()) << "Frame in " << resolution.first << "x" << resolution.second << " should be processed";
        EXPECT_EQ(result.rows, resolution.second) << "Height should be preserved";
        EXPECT_EQ(result.cols, resolution.first) << "Width should be preserved";
    }
}

TEST_F(OBSIntegrationTests, ParameterPersistence) {
    StabilizerWrapper wrapper;

    // Set parameters
    StabilizerCore::StabilizerParams params;
    params.enabled = true;
    params.smoothing_radius = 15;
    params.max_correction = 25.0f;
    params.feature_count = 300;

    wrapper.update_parameters(params);

    // Verify parameters are preserved
    auto current_params = wrapper.get_current_params();
    EXPECT_EQ(current_params.enabled, true);
    EXPECT_EQ(current_params.smoothing_radius, 15);
    EXPECT_FLOAT_EQ(current_params.max_correction, 25.0f);
    EXPECT_EQ(current_params.feature_count, 300);
}

TEST_F(OBSIntegrationTests, DebugMode) {
    StabilizerWrapper wrapper;
    wrapper.initialize(640, 480, 30.0f);

    // Test debug mode
    StabilizerCore::StabilizerParams params;
    params.debug_mode = true;
    wrapper.update_parameters(params);

    cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
    cv::Mat result = wrapper.process_frame(test_frame);

    EXPECT_FALSE(result.empty()) << "Frame should be processed in debug mode";
    EXPECT_EQ(result.rows, 480) << "Height should be preserved";
    EXPECT_EQ(result.cols, 640) << "Width should be preserved";
}

TEST_F(OBSIntegrationTests, FeatureDetectionImpact) {
    StabilizerWrapper wrapper;
    wrapper.initialize(640, 480, 30.0f);

    // Test with different feature counts
    std::vector<int> feature_counts = {100, 200, 500, 1000};

    for (int count : feature_counts) {
        StabilizerCore::StabilizerParams params;
        params.feature_count = count;
        wrapper.update_parameters(params);

        cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
        cv::Mat result = wrapper.process_frame(test_frame);

        EXPECT_FALSE(result.empty()) << "Frame should be processed with " << count << " features";
    }
}

TEST_F(OBSIntegrationTests, QualityLevelImpact) {
    StabilizerWrapper wrapper;
    wrapper.initialize(640, 480, 30.0f);

    // Test with different quality levels
    std::vector<float> quality_levels = {0.001f, 0.01f, 0.05f, 0.1f};

    for (float quality : quality_levels) {
        StabilizerCore::StabilizerParams params;
        params.quality_level = quality;
        wrapper.update_parameters(params);

        cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
        cv::Mat result = wrapper.process_frame(test_frame);

        EXPECT_FALSE(result.empty()) << "Frame should be processed with quality level " << quality;
    }
}

TEST_F(OBSIntegrationTests, MinDistanceImpact) {
    StabilizerWrapper wrapper;
    wrapper.initialize(640, 480, 30.0f);

    // Test with different minimum distances
    std::vector<float> min_distances = {10.0f, 30.0f, 50.0f, 100.0f};

    for (float distance : min_distances) {
        StabilizerCore::StabilizerParams params;
        params.min_distance = distance;
        wrapper.update_parameters(params);

        cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
        cv::Mat result = wrapper.process_frame(test_frame);

        EXPECT_FALSE(result.empty()) << "Frame should be processed with min distance " << distance;
    }
}

TEST_F(OBSIntegrationTests, BlockSizeImpact) {
    StabilizerWrapper wrapper;
    wrapper.initialize(640, 480, 30.0f);

    // Test with different block sizes
    std::vector<int> block_sizes = {3, 5, 7, 11};

    for (int size : block_sizes) {
        StabilizerCore::StabilizerParams params;
        params.block_size = size;
        wrapper.update_parameters(params);

        cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
        cv::Mat result = wrapper.process_frame(test_frame);

        EXPECT_FALSE(result.empty()) << "Frame should be processed with block size " << size;
    }
}

TEST_F(OBSIntegrationTests, HarrisDetectorImpact) {
    StabilizerWrapper wrapper;
    wrapper.initialize(640, 480, 30.0f);

    // Test with and without Harris detector
    StabilizerCore::StabilizerParams params;
    params.use_harris = false;
    wrapper.update_parameters(params);

    cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
    cv::Mat result = wrapper.process_frame(test_frame);

    EXPECT_FALSE(result.empty()) << "Frame should be processed without Harris detector";

    // Test with Harris detector
    params.use_harris = true;
    wrapper.update_parameters(params);

    result = wrapper.process_frame(test_frame);
    EXPECT_FALSE(result.empty()) << "Frame should be processed with Harris detector";
}

TEST_F(OBSIntegrationTests, ProcessingWithoutFeatures) {
    StabilizerWrapper wrapper;
    wrapper.initialize(640, 480, 30.0f);

    // Test processing with frames that might not have detectable features
    cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);

    // Fill with uniform color (no features)
    test_frame = cv::Scalar(128, 128, 128, 255);

    cv::Mat result = wrapper.process_frame(test_frame);
    EXPECT_FALSE(result.empty()) << "Frame should be processed even without features";
    EXPECT_EQ(result.rows, 480) << "Height should be preserved";
    EXPECT_EQ(result.cols, 640) << "Width should be preserved";
}

TEST_F(OBSIntegrationTests, HighFrequencyProcessing) {
    StabilizerWrapper wrapper;
    wrapper.initialize(640, 480, 60.0f);

    cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);

    // Process frames at high frequency
    for (int i = 0; i < 100; i++) {
        cv::Mat result = wrapper.process_frame(test_frame);
        EXPECT_FALSE(result.empty()) << "High frequency frame " << i << " should be processed";
    }

    auto metrics = wrapper.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 100) << "All high frequency frames should be processed";
    EXPECT_LT(metrics.avg_processing_time, 1000.0) << "Processing time should be reasonable";
}

TEST_F(OBSIntegrationTests, LastErrorMessage) {
    StabilizerWrapper wrapper;

    // Test getting last error message
    std::string error = wrapper.get_last_error();
    EXPECT_FALSE(error.empty()) << "Should have an error message when not initialized";

    // Initialize and process frames
    wrapper.initialize(640, 480, 30.0f);
    cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
    wrapper.process_frame(test_frame);

    // Error message should be empty or different after successful processing
    error = wrapper.get_last_error();
    EXPECT_TRUE(error.empty()) << "Error message should be empty after successful processing";
}

#endif // SKIP_OPENCV_TESTS
