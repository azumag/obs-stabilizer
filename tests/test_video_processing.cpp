#ifndef SKIP_OPENCV_TESTS

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <random>
#include "core/stabilizer_core.hpp"
#include "test_data_generator.hpp"

using namespace TestDataGenerator;

class VideoProcessingTests : public ::testing::Test {
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

TEST_F(VideoProcessingTests, NV12FrameProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frame in NV12 format
    cv::Mat frame = generate_frame_in_format(640, 480, CV_8UC4);

    // Process the frame
    cv::Mat result = stabilizer.process_frame(frame);

    // Basic validation
    EXPECT_FALSE(result.empty()) << "Processed frame should not be empty";
    EXPECT_EQ(result.rows, frame.rows) << "Output should have same height as input";
    EXPECT_EQ(result.cols, frame.cols) << "Output should have same width as input";
    EXPECT_EQ(result.type(), frame.type()) << "Output should have same type as input";
}

TEST_F(VideoProcessingTests, I420FrameProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frame in I420 format
    cv::Mat frame = generate_frame_in_format(640, 480, CV_8UC4);

    // Process the frame
    cv::Mat result = stabilizer.process_frame(frame);

    // Basic validation
    EXPECT_FALSE(result.empty()) << "Processed frame should not be empty";
    EXPECT_EQ(result.rows, frame.rows) << "Output should have same height as input";
    EXPECT_EQ(result.cols, frame.cols) << "Output should have same width as input";
    EXPECT_EQ(result.type(), frame.type()) << "Output should have same type as input";
}

TEST_F(VideoProcessingTests, InvalidFrameHandling) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Test with empty frame
    cv::Mat empty_frame;
    cv::Mat result = stabilizer.process_frame(empty_frame);
    EXPECT_TRUE(result.empty()) << "Empty frame should return empty result";

    // Test with tiny frame
    cv::Mat tiny_frame = cv::Mat::zeros(10, 10, CV_8UC4);
    result = stabilizer.process_frame(tiny_frame);
    EXPECT_TRUE(result.empty()) << "Tiny frame should return empty result";

    // Test with huge frame
    cv::Mat huge_frame = cv::Mat::zeros(10000, 10000, CV_8UC4);
    result = stabilizer.process_frame(huge_frame);
    EXPECT_TRUE(result.empty()) << "Huge frame should return empty result";

    // Test with NULL frame
    result = stabilizer.process_frame(cv::Mat());
    EXPECT_TRUE(result.empty()) << "NULL frame should return empty result";
}

TEST_F(VideoProcessingTests, FrameSizePreservation) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat original_frame = generate_test_frame(640, 480, 0);

    cv::Mat result = stabilizer.process_frame(original_frame);

    // Verify dimensions are preserved
    EXPECT_EQ(result.rows, original_frame.rows) << "Height should be preserved";
    EXPECT_EQ(result.cols, original_frame.cols) << "Width should be preserved";
    EXPECT_EQ(result.channels(), original_frame.channels()) << "Channels should be preserved";
    EXPECT_EQ(result.depth(), original_frame.depth()) << "Depth should be preserved";
}

TEST_F(VideoProcessingTests, FrameFormatPreservation) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Test with different frame formats
    std::vector<int> formats = {CV_8UC1, CV_8UC3, CV_8UC4};

    for (int format : formats) {
        cv::Mat frame = generate_frame_in_format(640, 480, format);
        cv::Mat result = stabilizer.process_frame(frame);

        EXPECT_FALSE(result.empty()) << "Frame in format " << format << " should be processed";
        EXPECT_EQ(result.rows, frame.rows) << "Height should be preserved for format " << format;
        EXPECT_EQ(result.cols, frame.cols) << "Width should be preserved for format " << format;
    }
}

TEST_F(VideoProcessingTests, MultipleFrameProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    // Process multiple frames
    for (int i = 0; i < 10; i++) {
        cv::Mat result = stabilizer.process_frame(frame);

        EXPECT_FALSE(result.empty()) << "Frame " << i << " should be processed";
        EXPECT_EQ(result.rows, frame.rows) << "Height should be preserved for frame " << i;
        EXPECT_EQ(result.cols, frame.cols) << "Width should be preserved for frame " << i;
    }

    // Check performance metrics
    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0) << "Frame count should be incremented";
}

TEST_F(VideoProcessingTests, SequentialFrameProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate a sequence of frames with different patterns
    std::vector<cv::Mat> frames;
    for (int i = 0; i < 20; i++) {
        cv::Mat frame = generate_test_frame(640, 480, i % 3);
        frames.push_back(frame);
    }

    // Process frames sequentially
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Frame should be processed";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 20) << "All frames should be processed";
}

TEST_F(VideoProcessingTests, RandomFrameProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    std::random_device rd;
    std::mt19937 gen;
    gen.seed(rd());
    std::uniform_int_distribution<> dist(0, 2);

    // Process random frames
    for (int i = 0; i < 30; i++) {
        int pattern = dist(gen);
        cv::Mat frame = generate_test_frame(640, 480, pattern);

        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Random frame should be processed";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0) << "Random frames should be processed";
}

TEST_F(VideoProcessingTests, HighFrequencyFrameProcessing) {
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

TEST_F(VideoProcessingTests, FrameProcessingWithMotion) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frames with motion
    std::vector<cv::Mat> frames = generate_test_sequence(20, 640, 480, "horizontal");

    // Process frames with motion
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Frame with motion should be processed";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 20) << "All motion frames should be processed";
}

TEST_F(VideoProcessingTests, FrameProcessingWithRotation) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frames with rotation
    std::vector<cv::Mat> frames = generate_test_sequence(20, 640, 480, "rotation");

    // Process frames with rotation
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Frame with rotation should be processed";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 20) << "All rotation frames should be processed";
}

TEST_F(VideoProcessingTests, FrameProcessingWithZoom) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frames with zoom
    std::vector<cv::Mat> frames = generate_test_sequence(20, 640, 480, "zoom");

    // Process frames with zoom
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Frame with zoom should be processed";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 20) << "All zoom frames should be processed";
}

TEST_F(VideoProcessingTests, MixedMotionFrameProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frames with mixed motion patterns
    std::vector<cv::Mat> frames;
    for (int i = 0; i < 30; i++) {
        std::string pattern = (i % 3 == 0) ? "horizontal" :
                             (i % 3 == 1) ? "vertical" : "rotation";
        cv::Mat frame = generate_test_frame(640, 480, i % 3);

        if (i % 3 == 0) {
            frame = generate_horizontal_motion_frame(frame, i, 30);
        } else if (i % 3 == 1) {
            frame = generate_vertical_motion_frame(frame, i, 30);
        } else {
            frame = generate_rotation_frame(frame, i, 30, 10.0f);
        }

        frames.push_back(frame);
    }

    // Process mixed motion frames
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Mixed motion frame should be processed";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 30) << "All mixed motion frames should be processed";
}

TEST_F(VideoProcessingTests, FrameProcessingWithDifferentResolutions) {
    StabilizerCore stabilizer;
    test_params.smoothing_radius = 5;

    std::vector<std::pair<int, int>> resolutions = {
        {320, 240},
        {640, 480},
        {1280, 720},
        {1920, 1080}
    };

    for (const auto& resolution : resolutions) {
        stabilizer.initialize(resolution.first, resolution.second, test_params);

        cv::Mat frame = generate_test_frame(resolution.first, resolution.second, 0);
        cv::Mat result = stabilizer.process_frame(frame);

        EXPECT_FALSE(result.empty()) << "Frame in " << resolution.first << "x" << resolution.second << " should be processed";
        EXPECT_EQ(result.rows, resolution.second) << "Height should be preserved";
        EXPECT_EQ(result.cols, resolution.first) << "Width should be preserved";
    }
}

TEST_F(VideoProcessingTests, FrameProcessingWithDifferentColors) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Test with different color schemes
    std::vector<int> color_schemes = {0, 1, 2};  // 0: solid, 1: gradient, 2: checkerboard

    for (int scheme : color_schemes) {
        cv::Mat frame = generate_test_frame(640, 480, scheme);
        cv::Mat result = stabilizer.process_frame(frame);

        EXPECT_FALSE(result.empty()) << "Frame with color scheme " << scheme << " should be processed";
        EXPECT_EQ(result.rows, frame.rows) << "Height should be preserved";
        EXPECT_EQ(result.cols, frame.cols) << "Width should be preserved";
    }
}

TEST_F(VideoProcessingTests, FrameProcessingWithFeatures) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frame with features
    cv::Mat frame = create_frame_with_features(640, 480, 100);
    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "Frame with features should be processed";
    EXPECT_EQ(result.rows, frame.rows) << "Height should be preserved";
    EXPECT_EQ(result.cols, frame.cols) << "Width should be preserved";
}

TEST_F(VideoProcessingTests, FrameProcessingWithComplexPatterns) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frame with complex patterns
    cv::Mat frame = generate_test_frame(640, 480, 0);
    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "Complex pattern frame should be processed";
    EXPECT_EQ(result.rows, frame.rows) << "Height should be preserved";
    EXPECT_EQ(result.cols, frame.cols) << "Width should be preserved";
}

TEST_F(VideoProcessingTests, FrameProcessingWithNoise) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frame with noise
    cv::Mat frame = generate_test_frame(640, 480, 0);
    cv::Mat noisy_frame = frame.clone();

    // Add noise
    cv::Mat noise = cv::Mat::zeros(frame.size(), frame.type());
    cv::randn(noise, cv::Scalar(0), cv::Scalar(10));
    noisy_frame += noise;

    cv::Mat result = stabilizer.process_frame(noisy_frame);

    EXPECT_FALSE(result.empty()) << "Noisy frame should be processed";
    EXPECT_EQ(result.rows, frame.rows) << "Height should be preserved";
    EXPECT_EQ(result.cols, frame.cols) << "Width should be preserved";
}

TEST_F(VideoProcessingTests, FrameProcessingWithContrast) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    // Generate frame with high contrast
    cv::Mat frame = generate_test_frame(640, 480, 0);
    cv::Mat high_contrast = frame.clone();

    // Enhance contrast
    cv::convertScaleAbs(high_contrast, high_contrast, 1.5);

    cv::Mat result = stabilizer.process_frame(high_contrast);

    EXPECT_FALSE(result.empty()) << "High contrast frame should be processed";
    EXPECT_EQ(result.rows, frame.rows) << "Height should be preserved";
    EXPECT_EQ(result.cols, frame.cols) << "Width should be preserved";
}

#endif // SKIP_OPENCV_TESTS
