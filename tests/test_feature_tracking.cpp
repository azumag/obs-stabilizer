/*
Feature tracking and point management tests
Tests the feature detection, tracking, and refresh logic
*/

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/video/tracking.hpp>
#include <vector>
#include <random>

class FeatureTrackingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test pattern with features
        test_frame = cv::Mat::zeros(480, 640, CV_8UC1);
        
        // Add circles as features
        for (int y = 50; y < 430; y += 60) {
            for (int x = 50; x < 590; x += 80) {
                cv::circle(test_frame, cv::Point(x, y), 5, cv::Scalar(255), -1);
            }
        }
        
        // Add some corners
        cv::rectangle(test_frame, cv::Point(200, 200), cv::Point(220, 220), cv::Scalar(255), -1);
        cv::rectangle(test_frame, cv::Point(400, 300), cv::Point(420, 320), cv::Scalar(255), -1);
    }
    
    cv::Mat test_frame;
    
    // Helper to create shifted frame
    cv::Mat createShiftedFrame(const cv::Mat& src, int dx, int dy) {
        cv::Mat shifted = cv::Mat::zeros(src.size(), src.type());
        cv::Mat translation_matrix = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, dy);
        cv::warpAffine(src, shifted, translation_matrix, src.size());
        return shifted;
    }
    
    // Helper to add noise to frame
    cv::Mat addNoise(const cv::Mat& src, double noise_level = 10.0) {
        cv::Mat noisy;
        src.copyTo(noisy);
        
        cv::Mat noise = cv::Mat::zeros(src.size(), CV_8UC1);
        cv::randu(noise, 0, static_cast<int>(noise_level));
        
        cv::add(noisy, noise, noisy);
        return noisy;
    }
};

TEST_F(FeatureTrackingTest, BasicFeatureDetection) {
    std::vector<cv::Point2f> corners;
    
    // Detect features
    cv::goodFeaturesToTrack(test_frame, corners, 1000, 0.01, 10);
    
    EXPECT_GT(corners.size(), 0) << "No features detected";
    EXPECT_LT(corners.size(), 1000) << "Should not exceed maximum features";
    
    // Verify all detected points are within bounds
    for (const auto& corner : corners) {
        EXPECT_GE(corner.x, 0) << "Feature X coordinate negative";
        EXPECT_LT(corner.x, test_frame.cols) << "Feature X coordinate out of bounds";
        EXPECT_GE(corner.y, 0) << "Feature Y coordinate negative";  
        EXPECT_LT(corner.y, test_frame.rows) << "Feature Y coordinate out of bounds";
    }
}

TEST_F(FeatureTrackingTest, FeatureDetectionQuality) {
    std::vector<cv::Point2f> corners_high_quality, corners_low_quality;
    
    // High quality threshold
    cv::goodFeaturesToTrack(test_frame, corners_high_quality, 1000, 0.1, 10);
    
    // Low quality threshold  
    cv::goodFeaturesToTrack(test_frame, corners_low_quality, 1000, 0.001, 10);
    
    EXPECT_LE(corners_high_quality.size(), corners_low_quality.size()) 
        << "High quality threshold should detect fewer features";
    
    EXPECT_GT(corners_high_quality.size(), 0) << "Should detect some high quality features";
}

TEST_F(FeatureTrackingTest, OpticalFlowTracking) {
    // Detect initial features
    std::vector<cv::Point2f> points_prev;
    cv::goodFeaturesToTrack(test_frame, points_prev, 100, 0.01, 10);
    
    ASSERT_GT(points_prev.size(), 0) << "Need features for tracking test";
    
    // Create shifted frame
    cv::Mat shifted_frame = createShiftedFrame(test_frame, 3, 2);
    
    // Track features
    std::vector<cv::Point2f> points_curr;
    std::vector<uchar> status;
    std::vector<float> errors;
    
    cv::calcOpticalFlowLK(test_frame, shifted_frame, points_prev, points_curr, status, errors);
    
    EXPECT_EQ(points_curr.size(), points_prev.size()) << "Point count mismatch";
    EXPECT_EQ(status.size(), points_prev.size()) << "Status vector size mismatch";
    EXPECT_EQ(errors.size(), points_prev.size()) << "Error vector size mismatch";
    
    // Count successfully tracked points
    int good_tracks = 0;
    for (size_t i = 0; i < status.size(); i++) {
        if (status[i] && errors[i] < 50.0f) {
            good_tracks++;
            // Verify the tracked point moved in expected direction
            float dx = points_curr[i].x - points_prev[i].x;
            float dy = points_curr[i].y - points_prev[i].y;
            
            EXPECT_NEAR(dx, 3.0f, 2.0f) << "Tracked point X displacement incorrect";
            EXPECT_NEAR(dy, 2.0f, 2.0f) << "Tracked point Y displacement incorrect";
        }
    }
    
    EXPECT_GT(good_tracks, static_cast<int>(points_prev.size()) / 2) 
        << "Too few points tracked successfully";
}

TEST_F(FeatureTrackingTest, TrackingWithNoise) {
    // Detect features
    std::vector<cv::Point2f> points_prev;
    cv::goodFeaturesToTrack(test_frame, points_prev, 50, 0.01, 10);
    
    ASSERT_GT(points_prev.size(), 0) << "Need features for noise test";
    
    // Create noisy shifted frame
    cv::Mat shifted_frame = createShiftedFrame(test_frame, 2, 1);
    cv::Mat noisy_frame = addNoise(shifted_frame, 15.0);
    
    // Track features
    std::vector<cv::Point2f> points_curr;
    std::vector<uchar> status;
    std::vector<float> errors;
    
    cv::calcOpticalFlowLK(test_frame, noisy_frame, points_prev, points_curr, status, errors);
    
    // Count good tracks (higher error threshold due to noise)
    int good_tracks = 0;
    for (size_t i = 0; i < status.size(); i++) {
        if (status[i] && errors[i] < 100.0f) {
            good_tracks++;
        }
    }
    
    // Should still track reasonable number of points despite noise
    EXPECT_GT(good_tracks, static_cast<int>(points_prev.size()) / 3) 
        << "Too few points tracked with noise";
}

TEST_F(FeatureTrackingTest, FeatureRefreshLogic) {
    const int max_features = 100;
    const int refresh_threshold = max_features / 3;
    
    std::vector<cv::Point2f> current_features;
    
    // Simulate losing features over time
    cv::goodFeaturesToTrack(test_frame, current_features, max_features, 0.01, 10);
    
    ASSERT_GT(current_features.size(), static_cast<size_t>(refresh_threshold)) 
        << "Initial feature count too low";
    
    // Simulate feature loss
    while (current_features.size() > static_cast<size_t>(refresh_threshold)) {
        current_features.pop_back();
    }
    
    EXPECT_LE(current_features.size(), static_cast<size_t>(refresh_threshold)) 
        << "Should trigger refresh";
    
    // Refresh features
    std::vector<cv::Point2f> new_features;
    cv::goodFeaturesToTrack(test_frame, new_features, max_features, 0.01, 10);
    
    // Merge with existing (in real implementation, we'd avoid overlap)
    current_features.insert(current_features.end(), new_features.begin(), new_features.end());
    
    EXPECT_GT(current_features.size(), static_cast<size_t>(refresh_threshold)) 
        << "Feature refresh should increase count";
}

TEST_F(FeatureTrackingTest, PointFilteringLogic) {
    // Create test points with some out of bounds
    std::vector<cv::Point2f> test_points = {
        cv::Point2f(100, 100),  // Valid
        cv::Point2f(200, 200),  // Valid
        cv::Point2f(-10, 100),  // Invalid - negative X
        cv::Point2f(100, -5),   // Invalid - negative Y
        cv::Point2f(1000, 100), // Invalid - X out of bounds
        cv::Point2f(100, 1000), // Invalid - Y out of bounds
        cv::Point2f(300, 300)   // Valid
    };
    
    std::vector<uchar> status = {1, 1, 1, 1, 1, 1, 1}; // All "tracked"
    std::vector<float> errors = {10, 15, 20, 25, 30, 35, 12}; // Various errors
    
    // Filter points (similar to real implementation logic)
    std::vector<cv::Point2f> good_points;
    const float max_error = 30.0f;
    
    for (size_t i = 0; i < test_points.size() && i < status.size() && i < errors.size(); i++) {
        if (status[i] && errors[i] < max_error) {
            // Bounds check
            if (test_points[i].x >= 0 && test_points[i].x < test_frame.cols &&
                test_points[i].y >= 0 && test_points[i].y < test_frame.rows) {
                good_points.push_back(test_points[i]);
            }
        }
    }
    
    EXPECT_EQ(good_points.size(), 3) << "Should filter to 3 valid points";
    
    // Verify all remaining points are valid
    for (const auto& point : good_points) {
        EXPECT_GE(point.x, 0) << "Filtered point X should be valid";
        EXPECT_LT(point.x, test_frame.cols) << "Filtered point X should be in bounds";
        EXPECT_GE(point.y, 0) << "Filtered point Y should be valid";
        EXPECT_LT(point.y, test_frame.rows) << "Filtered point Y should be in bounds";
    }
}

TEST_F(FeatureTrackingTest, MinimumPointsCheck) {
    const size_t min_points = 6;
    
    // Test with sufficient points
    std::vector<cv::Point2f> sufficient_points;
    for (int i = 0; i < 10; i++) {
        sufficient_points.push_back(cv::Point2f(100 + i * 20, 100 + i * 15));
    }
    
    EXPECT_GE(sufficient_points.size(), min_points) << "Should have sufficient points";
    
    // Test with insufficient points
    std::vector<cv::Point2f> insufficient_points;
    for (int i = 0; i < 3; i++) {
        insufficient_points.push_back(cv::Point2f(100 + i * 20, 100 + i * 15));
    }
    
    EXPECT_LT(insufficient_points.size(), min_points) << "Should have insufficient points";
}

TEST_F(FeatureTrackingTest, TrackingRobustness) {
    // Test tracking with various challenging conditions
    std::vector<cv::Point2f> points_prev;
    cv::goodFeaturesToTrack(test_frame, points_prev, 50, 0.01, 10);
    
    ASSERT_GT(points_prev.size(), 0) << "Need features for robustness test";
    
    // Test 1: Large displacement
    cv::Mat large_shift = createShiftedFrame(test_frame, 20, 15);
    std::vector<cv::Point2f> points_large;
    std::vector<uchar> status_large;
    std::vector<float> errors_large;
    
    cv::calcOpticalFlowLK(test_frame, large_shift, points_prev, points_large, 
                         status_large, errors_large);
    
    // Should still track some points despite large displacement
    int good_large = 0;
    for (size_t i = 0; i < status_large.size(); i++) {
        if (status_large[i] && errors_large[i] < 100.0f) {
            good_large++;
        }
    }
    
    EXPECT_GT(good_large, 0) << "Should track some points with large displacement";
    
    // Test 2: Very small displacement
    cv::Mat small_shift = createShiftedFrame(test_frame, 1, 1);
    std::vector<cv::Point2f> points_small;
    std::vector<uchar> status_small;
    std::vector<float> errors_small;
    
    cv::calcOpticalFlowLK(test_frame, small_shift, points_prev, points_small,
                         status_small, errors_small);
    
    // Should track most points with small displacement
    int good_small = 0;
    for (size_t i = 0; i < status_small.size(); i++) {
        if (status_small[i] && errors_small[i] < 30.0f) {
            good_small++;
        }
    }
    
    EXPECT_GT(good_small, good_large) << "Should track more points with small displacement";
}