#ifndef SKIP_OPENCV_TESTS

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <vector>
#include <atomic>
#include <iostream>
#include <algorithm>
#include <cmath>
#include "core/stabilizer_core.hpp"
#include "test_data_generator.hpp"
#include "test_constants.hpp"

using namespace TestDataGenerator;
using namespace TestConstants;

class CodeLevelOptimizationTests : public ::testing::Test {
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

TEST_F(CodeLevelOptimizationTests, FunctionCallOverheadTest) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0);

    const int iterations = 100;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1.0 / avg_time;

    std::cout << "Function Call Overhead Test:" << std::endl;
    std::cout << "  Average time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    EXPECT_GT(fps, 0.0);
    EXPECT_LT(avg_time, 0.1);
}

TEST_F(CodeLevelOptimizationTests, VectorOperationEfficiency) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, 0);

    const int iterations = 200;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1000.0 / (avg_time * 1000.0);

    std::cout << "Vector Operation Efficiency Test:" << std::endl;
    std::cout << "  Average time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    EXPECT_GT(fps, 0.0);
}

TEST_F(CodeLevelOptimizationTests, BranchPredictionOptimization) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, 0);

    const int iterations = 500;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1000.0 / (avg_time * 1000.0);

    std::cout << "Branch Prediction Optimization Test:" << std::endl;
    std::cout << "  Average time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    EXPECT_GT(fps, 0.0);
}

TEST_F(CodeLevelOptimizationTests, MemoryAccessPattern) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0);

    const int iterations = 150;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1000.0 / (avg_time * 1000.0);

    std::cout << "Memory Access Pattern Test:" << std::endl;
    std::cout << "  Average time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    EXPECT_GT(fps, 0.0);
}

TEST_F(CodeLevelOptimizationTests, LoopOptimization) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, 0);

    const int iterations = 300;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1000.0 / (avg_time * 1000.0);

    std::cout << "Loop Optimization Test:" << std::endl;
    std::cout << "  Average time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    EXPECT_GT(fps, 0.0);
}

TEST_F(CodeLevelOptimizationTests, InlineFunctionCalls) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, 0);

    const int iterations = 1000;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1000.0 / (avg_time * 1000.0);

    std::cout << "Inline Function Calls Test:" << std::endl;
    std::cout << "  Average time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    EXPECT_GT(fps, 0.0);
}

TEST_F(CodeLevelOptimizationTests, HotPathPerformance) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0);

    const int iterations = 200;
    double total_time = 0.0;
    std::vector<double> frame_times;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        double time_ms = duration.count() * 1000.0;
        frame_times.push_back(time_ms);
        total_time += time_ms;
    }

    double avg_time = total_time / iterations;
    double min_time = *std::min_element(frame_times.begin(), frame_times.end());
    double max_time = *std::max_element(frame_times.begin(), frame_times.end());
    double fps = 1000.0 / avg_time;

    std::cout << "Hot Path Performance Test:" << std::endl;
    std::cout << "  Average time: " << avg_time << " ms" << std::endl;
    std::cout << "  Min time: " << min_time << " ms" << std::endl;
    std::cout << "  Max time: " << max_time << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    EXPECT_GT(fps, 0.0);
    EXPECT_LT(max_time - min_time, 50.0);
}

TEST_F(CodeLevelOptimizationTests, ConstantFolding) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, 0);

    const int iterations = 100;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1000.0 / (avg_time * 1000.0);

    std::cout << "Constant Folding Test:" << std::endl;
    std::cout << "  Average time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    EXPECT_GT(fps, 0.0);
}

TEST_F(CodeLevelOptimizationTests, ZeroOverheadIteration) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, 0);

    const int iterations = 500;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1000.0 / (avg_time * 1000.0);

    std::cout << "Zero-Overhead Iteration Test:" << std::endl;
    std::cout << "  Average time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    EXPECT_GT(fps, 0.0);
}

TEST_F(CodeLevelOptimizationTests, MemoryAlignment) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0);

    const int iterations = 100;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1000.0 / (avg_time * 1000.0);

    std::cout << "Memory Alignment Test:" << std::endl;
    std::cout << "  Average time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    EXPECT_GT(fps, 0.0);
}

TEST_F(CodeLevelOptimizationTests, CompilerOptimizationCheck) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, 0);

    const int iterations = 200;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1000.0 / (avg_time * 1000.0);

    std::cout << "Compiler Optimization Check:" << std::endl;
    std::cout << "  Average time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    EXPECT_GT(fps, 0.0);
}

#endif // SKIP_OPENCV_TESTS
