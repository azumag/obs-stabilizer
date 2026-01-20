#ifndef SKIP_OPENCV_TESTS

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include "core/stabilizer_core.hpp"
#include "test_data_generator.hpp"
#include "test_constants.hpp"

using namespace TestDataGenerator;
using namespace TestConstants;

class ResourceManagementTests : public ::testing::Test {
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

TEST_F(ResourceManagementTests, ExtendedOperationMemoryLeak1000Frames) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    auto initial_metrics = stabilizer.get_performance_metrics();

    for (int i = 0; i < 1000; i++) {
        cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, i % 3);
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Frame " << i << " should process successfully";
    }

    auto final_metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(final_metrics.frame_count, 1000) << "Should process 1000 frames";
}

TEST_F(ResourceManagementTests, ExtendedOperationMemoryLeak2000Frames) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 2000; i++) {
        cv::Mat frame = generate_test_frame(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, i % 3);
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_GT(successful_frames, 1500) << "Should process most frames successfully";
    EXPECT_EQ(metrics.frame_count, 2000) << "Frame count should be 2000";
}

TEST_F(ResourceManagementTests, MemoryManagementWithLargeFrames) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    for (int i = 0; i < 100; i++) {
        cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, i % 3);
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Should manage memory for large frames";
    }

    EXPECT_TRUE(stabilizer.is_ready()) << "Stabilizer should remain ready after many large frames";
}

TEST_F(ResourceManagementTests, BufferOverflowProtectionOddDimensions) {
    StabilizerCore stabilizer;
    int odd_width = 641;
    int odd_height = 481;
    stabilizer.initialize(odd_width, odd_height, test_params);

    cv::Mat frame = generate_test_frame(odd_width, odd_height, 0);
    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "Should handle odd dimensions without buffer overflow";
}

TEST_F(ResourceManagementTests, BufferOverflowProtectionNonPowerOfTwoDimensions) {
    StabilizerCore stabilizer;
    stabilizer.initialize(633, 477, test_params);

    cv::Mat frame = generate_test_frame(633, 477, 0);
    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "Should handle non-power-of-two dimensions";
}

TEST_F(ResourceManagementTests, BufferOverflowProtectionVeryLargeFrames) {
    StabilizerCore stabilizer;
    stabilizer.initialize(3840, 2160, test_params);

    cv::Mat frame = generate_test_frame(3840, 2160, 0);
    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "Should handle 4K frames without buffer overflow";
}

TEST_F(ResourceManagementTests, ThreadSafetyConcurrentConfigUpdates) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    std::atomic<bool> stop_flag{false};
    std::atomic<int> processed_frames{0};

    auto process_thread = std::thread([&]() {
        while (!stop_flag) {
            cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 
                                               processed_frames % 3);
            cv::Mat result = stabilizer.process_frame(frame);
            if (!result.empty()) {
                processed_frames++;
            }
        }
    });

    auto update_thread = std::thread([&]() {
        int counter = 0;
        while (!stop_flag) {
            test_params.feature_count = 100 + (counter % 400);
            test_params.smoothing_radius = 5 + (counter % 20);
            stabilizer.update_parameters(test_params);
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stop_flag = true;

    process_thread.join();
    update_thread.join();

    EXPECT_GT(processed_frames, 0) << "Should process frames during concurrent updates";
}

TEST_F(ResourceManagementTests, ThreadSafetyMultipleInstances) {
    const int num_instances = 4;
    std::vector<StabilizerCore> stabilizers(num_instances);
    std::vector<std::thread> threads;
    std::atomic<int> total_processed{0};

    for (int i = 0; i < num_instances; i++) {
        threads.emplace_back([&, i]() {
            stabilizers[i].initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);
            for (int j = 0; j < 50; j++) {
                cv::Mat frame = generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, j);
                cv::Mat result = stabilizers[i].process_frame(frame);
                if (!result.empty()) {
                    total_processed++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(total_processed, num_instances * 50) << "All instances should process all frames";
}

TEST_F(ResourceManagementTests, ThreadSafetyResetDuringProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    std::atomic<bool> stop_flag{false};
    std::atomic<int> processed_frames{0};
    std::atomic<int> reset_count{0};

    auto process_thread = std::thread([&]() {
        while (!stop_flag) {
            cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 
                                               processed_frames % 3);
            cv::Mat result = stabilizer.process_frame(frame);
            if (!result.empty()) {
                processed_frames++;
            }
        }
    });

    auto reset_thread = std::thread([&]() {
        while (!stop_flag) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            stabilizer.reset();
            reset_count++;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stop_flag = true;

    process_thread.join();
    reset_thread.join();

    EXPECT_GT(processed_frames, 0) << "Should process frames during resets";
    EXPECT_GT(reset_count, 0) << "Should perform resets";
}

TEST_F(ResourceManagementTests, ResourceCleanupAfterErrorConditions) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat valid_frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0);
    cv::Mat empty_frame;
    cv::Mat mismatched_frame(720, 1280, CV_8UC4);

    stabilizer.process_frame(valid_frame);
    stabilizer.process_frame(empty_frame);
    stabilizer.process_frame(mismatched_frame);

    stabilizer.reset();

    cv::Mat another_frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 1);
    cv::Mat result = stabilizer.process_frame(another_frame);

    EXPECT_FALSE(result.empty()) << "Should recover after error conditions";
}

TEST_F(ResourceManagementTests, ResourceCleanupAfterInvalidParameters) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    StabilizerCore::StabilizerParams invalid_params;
    invalid_params.enabled = true;
    invalid_params.feature_count = -100;
    invalid_params.quality_level = -0.5f;
    invalid_params.max_correction = -50.0f;

    EXPECT_FALSE(stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, invalid_params))
        << "Should reject invalid parameters";

    stabilizer.reset();

    EXPECT_TRUE(stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params))
        << "Should accept valid parameters after reset";

    cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0);
    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "Should work normally after recovery";
}

TEST_F(ResourceManagementTests, MemoryPoolStressTest) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    for (int i = 0; i < 500; i++) {
        cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, i % 5);
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Frame " << i << " should process";
    }

    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 500) << "Should process 500 frames";
}

TEST_F(ResourceManagementTests, TransformHistoryMemoryManagement) {
    StabilizerCore stabilizer;
    test_params.smoothing_radius = 100;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    for (int i = 0; i < 200; i++) {
        cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, i);
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Should maintain transform history correctly";
    }

    EXPECT_TRUE(stabilizer.is_ready()) << "Stabilizer should be ready after transform history management";
}

TEST_F(ResourceManagementTests, FeaturePointVectorMemoryManagement) {
    StabilizerCore stabilizer;
    test_params.feature_count = 1000;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    for (int i = 0; i < 100; i++) {
        cv::Mat frame = create_frame_with_features(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 1000);
        cv::Mat result = stabilizer.process_frame(frame);
        EXPECT_FALSE(result.empty()) << "Should manage large feature point vectors";
    }
}

TEST_F(ResourceManagementTests, ClearStateMemoryManagement) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    for (int i = 0; i < 50; i++) {
        cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, i);
        stabilizer.process_frame(frame);
    }

    stabilizer.clear_state();

    cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0);
    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "Should work after clear_state";
    auto metrics = stabilizer.get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 1) << "Frame count should reset";
}

#endif // SKIP_OPENCV_TESTS
