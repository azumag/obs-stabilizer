#ifndef SKIP_OPENCV_TESTS

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "core/stabilizer_core.hpp"
#include "test_data_generator.hpp"

using namespace TestDataGenerator;

class PerformanceBenchmarks : public ::testing::Test {
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

// Benchmark HD 30fps processing
TEST_F(PerformanceBenchmarks, HD30FpsProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(1920, 1080, test_params);

    cv::Mat frame = generate_test_frame(1920, 1080, 0);

    // Process 100 frames
    int iterations = 100;
    double total_time = 0.0;
    cv::Mat result;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1.0 / avg_time;

    std::cout << "HD 30fps Processing Benchmark:" << std::endl;
    std::cout << "  Average processing time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;

    // Verify processing worked
    EXPECT_FALSE(result.empty());
    EXPECT_GT(fps, 0.0);
}

// Benchmark Memory Usage
TEST_F(PerformanceBenchmarks, MemoryUsage) {
    StabilizerCore stabilizer;
    stabilizer.initialize(1920, 1080, test_params);

    // Get initial memory usage
    auto metrics1 = stabilizer.get_performance_metrics();

    // Process frames
    for (int i = 0; i < 100; i++) {
        cv::Mat frame = generate_test_frame(1920, 1080, i % 3);
        stabilizer.process_frame(frame);
    }

    // Get final memory usage
    auto metrics2 = stabilizer.get_performance_metrics();

    std::cout << "Memory Usage Benchmark:" << std::endl;
    std::cout << "  Frame count: " << metrics2.frame_count << std::endl;
    std::cout << "  Average processing time: " << metrics2.avg_processing_time * 1000 << " ms" << std::endl;

    // Verify processing worked
    EXPECT_GT(metrics2.frame_count, 0);
    EXPECT_LT(metrics2.avg_processing_time, 1000.0);
}

// Benchmark 720p 60fps processing
TEST_F(PerformanceBenchmarks, HD720p60FpsProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(1280, 720, test_params);

    cv::Mat frame = generate_test_frame(1280, 720, 0);

    // Process 100 frames
    int iterations = 100;
    double total_time = 0.0;
    cv::Mat result;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1.0 / avg_time;

    std::cout << "720p 60fps Processing Benchmark:" << std::endl;
    std::cout << "  Average processing time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;

    // Verify processing worked
    EXPECT_FALSE(result.empty());
    EXPECT_GT(fps, 0.0);
}

// Benchmark 480p 60fps processing
TEST_F(PerformanceBenchmarks, HD480p60FpsProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    // Process 100 frames
    int iterations = 100;
    double total_time = 0.0;
    cv::Mat result;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1.0 / avg_time;

    std::cout << "480p 60fps Processing Benchmark:" << std::endl;
    std::cout << "  Average processing time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;

    // Verify processing worked
    EXPECT_FALSE(result.empty());
    EXPECT_GT(fps, 0.0);
}

// Benchmark 360p 60fps processing
TEST_F(PerformanceBenchmarks, HD360p60FpsProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 360, test_params);

    cv::Mat frame = generate_test_frame(640, 360, 0);

    // Process 100 frames
    int iterations = 100;
    double total_time = 0.0;
    cv::Mat result;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1.0 / avg_time;

    std::cout << "360p 60fps Processing Benchmark:" << std::endl;
    std::cout << "  Average processing time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;

    // Verify processing worked
    EXPECT_FALSE(result.empty());
    EXPECT_GT(fps, 0.0);
}

// Benchmark with high feature count
TEST_F(PerformanceBenchmarks, HighFeatureCountPerformance) {
    StabilizerCore stabilizer;
    test_params.feature_count = 2000;
    stabilizer.initialize(1920, 1080, test_params);

    cv::Mat frame = generate_test_frame(1920, 1080, 0);

    // Process 100 frames
    int iterations = 100;
    double total_time = 0.0;
    cv::Mat result;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1.0 / avg_time;

    std::cout << "High Feature Count (2000) Performance:" << std::endl;
    std::cout << "  Average processing time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;

    // Verify processing worked
    EXPECT_FALSE(result.empty());
    EXPECT_GT(fps, 0.0);
}

// Benchmark with high smoothing radius
TEST_F(PerformanceBenchmarks, HighSmoothingRadiusPerformance) {
    StabilizerCore stabilizer;
    test_params.smoothing_radius = 100;
    stabilizer.initialize(1920, 1080, test_params);

    cv::Mat frame = generate_test_frame(1920, 1080, 0);

    // Process 100 frames
    int iterations = 100;
    double total_time = 0.0;
    cv::Mat result;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1.0 / avg_time;

    std::cout << "High Smoothing Radius (100) Performance:" << std::endl;
    std::cout << "  Average processing time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;

    // Verify processing worked
    EXPECT_FALSE(result.empty());
    EXPECT_GT(fps, 0.0);
}

// Benchmark with low quality level
TEST_F(PerformanceBenchmarks, LowQualityLevelPerformance) {
    StabilizerCore stabilizer;
    test_params.quality_level = 0.001f;
    stabilizer.initialize(1920, 1080, test_params);

    cv::Mat frame = generate_test_frame(1920, 1080, 0);

    // Process 100 frames
    int iterations = 100;
    double total_time = 0.0;
    cv::Mat result;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1.0 / avg_time;

    std::cout << "Low Quality Level (0.001) Performance:" << std::endl;
    std::cout << "  Average processing time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;

    // Verify processing worked
    EXPECT_FALSE(result.empty());
    EXPECT_GT(fps, 0.0);
}

// Benchmark with Harris detector
TEST_F(PerformanceBenchmarks, HarrisDetectorPerformance) {
    StabilizerCore stabilizer;
    test_params.use_harris = true;
    stabilizer.initialize(1920, 1080, test_params);

    cv::Mat frame = generate_test_frame(1920, 1080, 0);

    // Process 100 frames
    int iterations = 100;
    double total_time = 0.0;
    cv::Mat result;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        total_time += duration.count();
    }

    double avg_time = total_time / iterations;
    double fps = 1.0 / avg_time;

    std::cout << "Harris Detector Performance:" << std::endl;
    std::cout << "  Average processing time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;

    // Verify processing worked
    EXPECT_FALSE(result.empty());
    EXPECT_GT(fps, 0.0);
}

// Stress test with high frame rate
TEST_F(PerformanceBenchmarks, StressTestHighFrameRate) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat frame = generate_test_frame(640, 480, 0);

    // Process 1000 frames
    int iterations = 1000;
    double total_time = 0.0;
    int successful_frames = 0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        if (!result.empty()) {
            successful_frames++;
            std::chrono::duration<double> duration = end - start;
            total_time += duration.count();
        }
    }

    double avg_time = total_time / successful_frames;
    double fps = 1.0 / avg_time;

    std::cout << "Stress Test (1000 frames) Performance:" << std::endl;
    std::cout << "  Successful frames: " << successful_frames << "/" << iterations << std::endl;
    std::cout << "  Average processing time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    // Verify processing worked
    EXPECT_GT(successful_frames, 0);
    EXPECT_GT(fps, 0.0);
}

// Benchmark with motion patterns
TEST_F(PerformanceBenchmarks, MotionPatternPerformance) {
    StabilizerCore stabilizer;
    stabilizer.initialize(1920, 1080, test_params);

    // Generate frames with motion
    std::vector<cv::Mat> frames = generate_test_sequence(50, 1920, 1080, "horizontal");

    // Process all frames
    double total_time = 0.0;
    int successful_frames = 0;

    for (const auto& frame : frames) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        if (!result.empty()) {
            successful_frames++;
            std::chrono::duration<double> duration = end - start;
            total_time += duration.count();
        }
    }

    double avg_time = total_time / successful_frames;
    double fps = 1.0 / avg_time;

    std::cout << "Motion Pattern Performance (50 frames):" << std::endl;
    std::cout << "  Successful frames: " << successful_frames << "/" << frames.size() << std::endl;
    std::cout << "  Average processing time: " << avg_time * 1000 << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;

    // Verify processing worked
    EXPECT_GT(successful_frames, 0);
    EXPECT_GT(fps, 0.0);
}

// Benchmark with different resolutions
TEST_F(PerformanceBenchmarks, ResolutionComparison) {
    std::vector<std::pair<int, int>> resolutions = {
        {640, 480},
        {1280, 720},
        {1920, 1080}
    };

    std::cout << "Resolution Comparison Benchmark:" << std::endl;

    for (const auto& resolution : resolutions) {
        StabilizerCore stabilizer;
        stabilizer.initialize(resolution.first, resolution.second, test_params);

        cv::Mat frame = generate_test_frame(resolution.first, resolution.second, 0);

        // Process 100 frames
        int iterations = 100;
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

        std::cout << "  " << resolution.first << "x" << resolution.second << ": "
                  << avg_time * 1000 << " ms (" << fps << " FPS)" << std::endl;
    }
}

// Benchmark with different feature configurations
TEST_F(PerformanceBenchmarks, FeatureConfigurationPerformance) {
    std::vector<int> feature_counts = {100, 500, 1000, 2000};

    std::cout << "Feature Configuration Performance:" << std::endl;

    for (int feature_count : feature_counts) {
        StabilizerCore stabilizer;
        test_params.feature_count = feature_count;
        stabilizer.initialize(1920, 1080, test_params);

        cv::Mat frame = generate_test_frame(1920, 1080, 0);

        // Process 100 frames
        int iterations = 100;
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

        std::cout << "  " << feature_count << " features: "
                  << avg_time * 1000 << " ms (" << fps << " FPS)" << std::endl;
    }
}

#endif // SKIP_OPENCV_TESTS
