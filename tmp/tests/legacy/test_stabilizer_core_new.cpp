/*
Updated Core stabilizer functionality tests
Tests the actual StabilizerCore class with modular architecture
*/

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/video/tracking.hpp>
#include <vector>
#include <memory>

// Include test mocks first
#include "test_mocks.hpp"

// Include the core module
#include "../src/core/stabilizer_core.hpp"

// Use the namespace for our tests
using namespace obs_stabilizer;

class StabilizerCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize stabilizer core with test configuration
        config.smoothing_radius = 10;
        config.max_features = 100;
        config.enable_stabilization = true;
        config.error_threshold = 30.0f;
        config.min_feature_quality = 0.01f;

        stabilizer = std::make_unique<StabilizerCore>();
        ASSERT_TRUE(stabilizer->initialize(config));
    }

    StabilizerConfig config;
    std::unique_ptr<StabilizerCore> stabilizer;
};

TEST_F(StabilizerCoreTest, InitializationTest) {
    EXPECT_EQ(stabilizer->get_status(), StabilizerStatus::INITIALIZING);

    StabilizerMetrics metrics = stabilizer->get_metrics();
    EXPECT_EQ(metrics.tracked_features, 0);
    EXPECT_EQ(metrics.error_count, 0);
    EXPECT_EQ(metrics.status, StabilizerStatus::INITIALIZING);
}

TEST_F(StabilizerCoreTest, ConfigurationUpdateTest) {
    // Test configuration update
    StabilizerConfig new_config = config;
    new_config.smoothing_radius = 20;
    new_config.max_features = 300;

    stabilizer->update_configuration(new_config);

    // Status should remain the same
    EXPECT_EQ(stabilizer->get_status(), StabilizerStatus::INITIALIZING);
}

TEST_F(StabilizerCoreTest, ResetTest) {
    // Reset the stabilizer
    stabilizer->reset();

    EXPECT_EQ(stabilizer->get_status(), StabilizerStatus::INACTIVE);

    StabilizerMetrics metrics = stabilizer->get_metrics();
    EXPECT_EQ(metrics.tracked_features, 0);
    EXPECT_EQ(metrics.error_count, 0);
}

TEST_F(StabilizerCoreTest, InvalidFrameTest) {
    // Test with null frame
    TransformResult result = stabilizer->process_frame(nullptr);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.metrics.status, StabilizerStatus::INITIALIZING);
}

// Basic OpenCV functionality tests (independent of StabilizerCore)
class OpenCVFunctionalityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test frames with known features
        test_frame_1 = cv::Mat::zeros(480, 640, CV_8UC1);
        test_frame_2 = cv::Mat::zeros(480, 640, CV_8UC1);

        // Add some features to track
        for (int y = 50; y < 430; y += 50) {
            for (int x = 50; x < 590; x += 50) {
                cv::circle(test_frame_1, cv::Point(x, y), 3, cv::Scalar(255), -1);
                cv::circle(test_frame_2, cv::Point(x + 2, y + 1), 3, cv::Scalar(255), -1); // Slightly shifted
            }
        }
    }

    cv::Mat test_frame_1;
    cv::Mat test_frame_2;
};

TEST_F(OpenCVFunctionalityTest, FeatureDetectionTest) {
    // Test feature detection
    ASSERT_FALSE(test_frame_1.empty());

    std::vector<cv::Point2f> detected_points;
    cv::goodFeaturesToTrack(test_frame_1, detected_points, 200, 0.01, 10);

    EXPECT_GT(detected_points.size(), 0) << "No features detected in test frame";
    EXPECT_LE(detected_points.size(), 200) << "Too many features detected";

    // Verify detected points are within frame bounds
    for (const auto& point : detected_points) {
        EXPECT_GE(point.x, 0) << "Feature point X coordinate out of bounds";
        EXPECT_LT(point.x, test_frame_1.cols) << "Feature point X coordinate out of bounds";
        EXPECT_GE(point.y, 0) << "Feature point Y coordinate out of bounds";
        EXPECT_LT(point.y, test_frame_1.rows) << "Feature point Y coordinate out of bounds";
    }
}

TEST_F(OpenCVFunctionalityTest, OpticalFlowTrackingTest) {
    // Detect features on first frame
    std::vector<cv::Point2f> points_1, points_2;
    cv::goodFeaturesToTrack(test_frame_1, points_1, 100, 0.01, 10);

    ASSERT_GT(points_1.size(), 0) << "No features detected for tracking test";

    // Track features to second frame
    std::vector<uchar> status;
    std::vector<float> errors;

    cv::calcOpticalFlowPyrLK(test_frame_1, test_frame_2, points_1, points_2, status, errors);

    EXPECT_EQ(points_2.size(), points_1.size()) << "Tracked points size mismatch";
    EXPECT_EQ(status.size(), points_1.size()) << "Status vector size mismatch";
    EXPECT_EQ(errors.size(), points_1.size()) << "Error vector size mismatch";

    // Count successfully tracked points
    int tracked_count = 0;
    for (size_t i = 0; i < status.size(); i++) {
        if (status[i]) {
            tracked_count++;
            EXPECT_LT(errors[i], 100.0f) << "Tracking error too high for point " << i;
        }
    }

    EXPECT_GT(tracked_count, 0) << "No points successfully tracked";
}

TEST_F(OpenCVFunctionalityTest, TransformEstimationTest) {
    // Create two sets of corresponding points with known transformation
    std::vector<cv::Point2f> points_1, points_2;

    // Add corresponding points with a simple translation
    for (int i = 0; i < 10; i++) {
        cv::Point2f p1(100 + i * 50, 100 + i * 30);
        cv::Point2f p2(p1.x + 5, p1.y + 3); // Translation of (5, 3)
        points_1.push_back(p1);
        points_2.push_back(p2);
    }

    // Estimate transformation
    cv::Mat transform = cv::estimateAffinePartial2D(points_2, points_1,
                                                   cv::noArray(), cv::RANSAC, 3.0);

    ASSERT_FALSE(transform.empty()) << "Failed to estimate transformation";
    EXPECT_EQ(transform.rows, 2) << "Transform matrix wrong number of rows";
    EXPECT_EQ(transform.cols, 3) << "Transform matrix wrong number of columns";

    // Verify the transformation is close to expected translation
    double tx = transform.at<double>(0, 2);
    double ty = transform.at<double>(1, 2);

    EXPECT_NEAR(tx, -5.0, 1.0) << "Translation X not as expected";
    EXPECT_NEAR(ty, -3.0, 1.0) << "Translation Y not as expected";
}

TEST_F(OpenCVFunctionalityTest, TransformSmoothingTest) {
    // Test transform history management and smoothing
    std::vector<cv::Mat> transform_history;

    cv::Mat transform1 = cv::Mat::eye(2, 3, CV_64F);
    transform1.at<double>(0, 2) = 5.0; // Translation X
    transform1.at<double>(1, 2) = 3.0; // Translation Y

    cv::Mat transform2 = cv::Mat::eye(2, 3, CV_64F);
    transform2.at<double>(0, 2) = 7.0;
    transform2.at<double>(1, 2) = 4.0;

    cv::Mat transform3 = cv::Mat::eye(2, 3, CV_64F);
    transform3.at<double>(0, 2) = 6.0;
    transform3.at<double>(1, 2) = 2.0;

    // Add transforms to history
    transform_history.push_back(transform1.clone());
    transform_history.push_back(transform2.clone());
    transform_history.push_back(transform3.clone());

    // Calculate smoothed transform (moving average)
    cv::Mat smoothed = cv::Mat::zeros(2, 3, CV_64F);
    for (const auto& hist_transform : transform_history) {
        smoothed += hist_transform;
    }
    smoothed /= static_cast<double>(transform_history.size());

    // Verify smoothed values
    double smoothed_tx = smoothed.at<double>(0, 2);
    double smoothed_ty = smoothed.at<double>(1, 2);

    EXPECT_NEAR(smoothed_tx, 6.0, 0.1) << "Smoothed translation X incorrect"; // (5+7+6)/3 = 6
    EXPECT_NEAR(smoothed_ty, 3.0, 0.1) << "Smoothed translation Y incorrect"; // (3+4+2)/3 = 3
}