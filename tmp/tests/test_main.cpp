/*
Main test runner for OBS Stabilizer Plugin
Uses Google Test framework for unit testing
*/

#include <gtest/gtest.h>
#include <iostream>

#ifdef SKIP_OPENCV_TESTS
// Test environment setup without OpenCV
class StabilizerTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        std::cout << "=== OBS Stabilizer Test Suite (Basic) ===" << std::endl;
        std::cout << "OpenCV tests SKIPPED - OpenCV not found" << std::endl;
        std::cout << "Running basic tests only..." << std::endl;
    }

    void TearDown() override {
        std::cout << "=== Basic Test Suite Completed ===" << std::endl;
    }
};

// Basic functionality test when OpenCV is not available
TEST(BasicTest, EnvironmentTest) {
    EXPECT_TRUE(true) << "Basic test environment is functional";
}

#else
#include <opencv2/opencv.hpp>

// Test environment setup with OpenCV
class StabilizerTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        std::cout << "=== OBS Stabilizer Test Suite (Full) ===" << std::endl;
        std::cout << "OpenCV Version: " << CV_VERSION << std::endl;

        // Verify OpenCV components are available
        std::cout << "Verifying OpenCV components..." << std::endl;

        // Test basic OpenCV functionality
        cv::Mat test_image = cv::Mat::zeros(100, 100, CV_8UC1);
        ASSERT_FALSE(test_image.empty()) << "Failed to create test image";

        std::cout << "OpenCV components verified successfully." << std::endl;
    }

    void TearDown() override {
        std::cout << "=== Full Test Suite Completed ===" << std::endl;
    }
};
#endif

int main(int argc, char **argv) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);

    // Add test environment
    ::testing::AddGlobalTestEnvironment(new StabilizerTestEnvironment);

    // Run all tests
    return RUN_ALL_TESTS();
}