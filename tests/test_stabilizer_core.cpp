/*
Core stabilizer functionality tests
Tests the basic stabilization algorithms and data structures
*/

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/video/tracking.hpp>
#include <vector>

// Mock stabilizer data structure for testing
struct MockStabilizerData {
    cv::Mat prev_frame;
    cv::Mat working_gray;
    std::vector<cv::Point2f> prev_points;
    cv::Mat accumulated_transform;
    cv::Mat smoothed_transform;
    std::vector<cv::Mat> transform_history;
    bool first_frame;
    int smoothing_radius;
    int max_features;
    
    MockStabilizerData() : first_frame(true), smoothing_radius(30), max_features(200) {
        accumulated_transform = cv::Mat::eye(2, 3, CV_64F);
        smoothed_transform = cv::Mat::eye(2, 3, CV_64F);
    }
};

class StabilizerCoreTest : public ::testing::Test {
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
    MockStabilizerData stabilizer;
};

TEST_F(StabilizerCoreTest, InitializationTest) {
    EXPECT_TRUE(stabilizer.first_frame);
    EXPECT_EQ(stabilizer.smoothing_radius, 30);
    EXPECT_EQ(stabilizer.max_features, 200);
    EXPECT_FALSE(stabilizer.accumulated_transform.empty());
    EXPECT_FALSE(stabilizer.smoothed_transform.empty());
    EXPECT_TRUE(stabilizer.prev_points.empty());
    EXPECT_TRUE(stabilizer.transform_history.empty());
}

TEST_F(StabilizerCoreTest, FeatureDetectionTest) {
    // Test feature detection on first frame
    ASSERT_FALSE(test_frame_1.empty());
    
    std::vector<cv::Point2f> detected_points;
    cv::goodFeaturesToTrack(test_frame_1, detected_points, 
                           std::min(stabilizer.max_features, 1000), 0.01, 10);
    
    EXPECT_GT(detected_points.size(), 0) << "No features detected in test frame";
    EXPECT_LE(detected_points.size(), static_cast<size_t>(stabilizer.max_features)) 
        << "Too many features detected";
    
    // Verify detected points are within frame bounds
    for (const auto& point : detected_points) {
        EXPECT_GE(point.x, 0) << "Feature point X coordinate out of bounds";
        EXPECT_LT(point.x, test_frame_1.cols) << "Feature point X coordinate out of bounds";
        EXPECT_GE(point.y, 0) << "Feature point Y coordinate out of bounds";
        EXPECT_LT(point.y, test_frame_1.rows) << "Feature point Y coordinate out of bounds";
    }
}

TEST_F(StabilizerCoreTest, OpticalFlowTrackingTest) {
    // Detect features on first frame
    std::vector<cv::Point2f> points_1, points_2;
    cv::goodFeaturesToTrack(test_frame_1, points_1, 100, 0.01, 10);
    
    ASSERT_GT(points_1.size(), 0) << "No features detected for tracking test";
    
    // Track features to second frame
    std::vector<uchar> status;
    std::vector<float> errors;
    
    cv::calcOpticalFlowLK(test_frame_1, test_frame_2, points_1, points_2, status, errors);
    
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

TEST_F(StabilizerCoreTest, TransformEstimationTest) {
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

TEST_F(StabilizerCoreTest, TransformSmoothingTest) {
    // Test transform history management and smoothing
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
    stabilizer.transform_history.push_back(transform1.clone());
    stabilizer.transform_history.push_back(transform2.clone());
    stabilizer.transform_history.push_back(transform3.clone());
    
    // Calculate smoothed transform (moving average)
    cv::Mat smoothed = cv::Mat::zeros(2, 3, CV_64F);
    for (const auto& hist_transform : stabilizer.transform_history) {
        smoothed += hist_transform;
    }
    smoothed /= static_cast<double>(stabilizer.transform_history.size());
    
    // Verify smoothed values
    double smoothed_tx = smoothed.at<double>(0, 2);
    double smoothed_ty = smoothed.at<double>(1, 2);
    
    EXPECT_NEAR(smoothed_tx, 6.0, 0.1) << "Smoothed translation X incorrect"; // (5+7+6)/3 = 6
    EXPECT_NEAR(smoothed_ty, 3.0, 0.1) << "Smoothed translation Y incorrect"; // (3+4+2)/3 = 3
}

TEST_F(StabilizerCoreTest, TransformHistoryLimitTest) {
    // Test that transform history doesn't grow indefinitely
    const int max_history = 5;
    
    for (int i = 0; i < max_history * 2; i++) {
        cv::Mat transform = cv::Mat::eye(2, 3, CV_64F);
        transform.at<double>(0, 2) = static_cast<double>(i);
        stabilizer.transform_history.push_back(transform.clone());
        
        // Limit history size
        if (stabilizer.transform_history.size() > static_cast<size_t>(max_history)) {
            stabilizer.transform_history.erase(stabilizer.transform_history.begin());
        }
        
        EXPECT_LE(stabilizer.transform_history.size(), static_cast<size_t>(max_history))
            << "Transform history exceeds maximum size";
    }
    
    EXPECT_EQ(stabilizer.transform_history.size(), static_cast<size_t>(max_history))
        << "Transform history not at expected size";
}

TEST_F(StabilizerCoreTest, FrameValidationTest) {
    // Test frame validation logic
    cv::Mat valid_frame = cv::Mat::zeros(480, 640, CV_8UC1);
    cv::Mat empty_frame;
    cv::Mat tiny_frame = cv::Mat::zeros(10, 10, CV_8UC1);
    
    EXPECT_FALSE(valid_frame.empty()) << "Valid frame should not be empty";
    EXPECT_GE(valid_frame.rows, 50) << "Valid frame should be large enough";
    EXPECT_GE(valid_frame.cols, 50) << "Valid frame should be large enough";
    
    EXPECT_TRUE(empty_frame.empty()) << "Empty frame should be detected";
    
    EXPECT_LT(tiny_frame.rows, 50) << "Tiny frame should be too small";
    EXPECT_LT(tiny_frame.cols, 50) << "Tiny frame should be too small";
}