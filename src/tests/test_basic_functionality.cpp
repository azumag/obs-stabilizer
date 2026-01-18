/*
OBS Stabilizer Plugin - Simple Test Suite
Tests for the actual stabilizer_opencv.cpp implementation
*/

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <memory>

// Forward declarations from stabilizer_opencv.cpp
extern "C" {
    void* stabilizer_filter_create(obs_data_t* settings, obs_source_t* source);
    void stabilizer_filter_destroy(void* data);
    const char* stabilizer_filter_get_name(void* unused);
}

class StabilizerBasicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }

    void TearDown() override {
        // Cleanup
    }
};

TEST_F(StabilizerBasicTest, FilterCreateDestroy) {
    // Test basic create/destroy cycle
    void* filter = stabilizer_filter_create(nullptr, nullptr);
    EXPECT_NE(filter, nullptr);
    
    if (filter) {
        stabilizer_filter_destroy(filter);
    }
}

TEST_F(StabilizerBasicTest, FilterGetName) {
    // Test filter name
    const char* name = stabilizer_filter_get_name(nullptr);
    EXPECT_STREQ(name, "OpenCV Stabilizer");
}

TEST_F(StabilizerBasicTest, OpenCVMatOperations) {
    // Test basic OpenCV operations used by stabilizer
    cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
    cv::Mat gray;
    
    // Convert to grayscale (used in stabilizer)
    cv::cvtColor(test_frame, gray, cv::COLOR_BGRA2GRAY);
    EXPECT_EQ(gray.channels(), 1);
    EXPECT_EQ(gray.rows, 480);
    EXPECT_EQ(gray.cols, 640);
}

TEST_F(StabilizerBasicTest, TransformMatrixOperations) {
    // Test transform matrix operations
    cv::Mat identity = cv::Mat::eye(3, 3, CV_64F);
    EXPECT_EQ(identity.rows, 3);
    EXPECT_EQ(identity.cols, 3);
    EXPECT_EQ(identity.type(), CV_64F);
    
    // Test matrix inversion (used in stabilizer)
    cv::Mat transform = cv::Mat::zeros(3, 3, CV_64F);
    transform.at<double>(0, 0) = 1.0;
    transform.at<double>(1, 1) = 1.0;
    transform.at<double>(2, 2) = 1.0;
    
    cv::Mat inverse;
    cv::invert(transform, inverse);
    EXPECT_FALSE(inverse.empty());
}