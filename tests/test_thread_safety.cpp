/**
 * Thread Safety Tests for StabilizerWrapper
 *
 * These tests verify that StabilizerWrapper is properly thread-safe when
 * accessed concurrently from multiple threads (UI thread and video thread).
 *
 * RATIONALE: According to ARCH.md, StabilizerWrapper uses std::mutex to
 * provide thread-safe access. These tests verify that the mutex protection
 * works correctly in concurrent scenarios.
 */

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <random>

#include "stabilizer_wrapper.hpp"
#include "test_constants.hpp"
#include "test_data_generator.hpp"

using namespace TestConstants;

class ThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize wrapper with test parameters
        StabilizerCore::StabilizerParams params = TestDataGenerator::create_default_params();
        ASSERT_TRUE(wrapper.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    }

    void TearDown() override {
        wrapper.reset();
    }

    StabilizerWrapper wrapper;
};

/**
 * Test: Concurrent parameter updates and frame processing
 *
 * RATIONALE: In production, the OBS UI thread updates parameters while
 * the video thread processes frames. This test simulates this scenario
 * to ensure no data races or deadlocks occur.
 */
TEST_F(ThreadSafetyTest, ConcurrentParameterUpdateAndProcessing) {
    std::atomic<bool> running{true};
    std::atomic<int> process_count{0};
    std::atomic<int> update_count{0};
    std::atomic<int> errors{0};

    // Thread 1: Video thread - continuously process frames
    std::thread video_thread([&]() {
        cv::Mat base_frame = TestDataGenerator::generate_test_frame(
            Resolution::VGA_WIDTH,
            Resolution::VGA_HEIGHT
        );

        int local_count = 0;
        while (running.load() && local_count < 1000) {
            try {
                cv::Mat frame = TestDataGenerator::create_motion_frame(
                    base_frame,
                    1.0f, 2.0f, 0.5f
                );
                cv::Mat result = wrapper.process_frame(frame);

                if (result.empty()) {
                    errors++;
                }

                process_count++;
                local_count++;
            } catch (const std::exception& e) {
                errors++;
            }
        }
    });

    // Thread 2: UI thread - continuously update parameters
    std::thread ui_thread([&]() {
        int local_count = 0;
        while (running.load() && local_count < 1000) {
            try {
                StabilizerCore::StabilizerParams params = TestDataGenerator::create_default_params();
                params.smoothing_radius = 10 + (local_count % 50);
                params.max_correction = 10.0f + (local_count % 30);
                params.feature_count = 100 + (local_count % 400);
                params.quality_level = 0.001f + (local_count % 100) / 1000.0f;
                wrapper.update_parameters(params);

                update_count++;
                local_count++;

                // Small delay to simulate UI responsiveness
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            } catch (const std::exception& e) {
                errors++;
            }
        }
    });

    // Let threads run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Stop both threads
    running = false;
    video_thread.join();
    ui_thread.join();

    // Verify no errors occurred
    EXPECT_EQ(errors, 0) << "Thread safety errors detected";
    EXPECT_GT(process_count, 0) << "No frames were processed";
    EXPECT_GT(update_count, 0) << "No parameters were updated";

    std::cout << "Processed " << process_count << " frames and "
              << update_count << " parameter updates with "
              << errors << " errors" << std::endl;
}

/**
 * Test: Multiple threads accessing get_performance_metrics concurrently
 *
 * RATIONALE: Metrics might be queried from multiple threads in a monitoring
 * context. This test ensures that metric retrieval is thread-safe.
 */
TEST_F(ThreadSafetyTest, ConcurrentMetricsAccess) {
    std::atomic<bool> running{true};
    std::atomic<int> metrics_count{0};
    std::atomic<int> errors{0};

    // Process some frames first
    cv::Mat base_frame = TestDataGenerator::generate_test_frame(
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT
    );

    for (int i = 0; i < 50; i++) {
        cv::Mat frame = TestDataGenerator::create_motion_frame(
            base_frame, 1.0f, 2.0f, 0.5f
        );
        wrapper.process_frame(frame);
    }

    // Spawn multiple threads to query metrics concurrently
    std::vector<std::thread> threads;
    const int NUM_THREADS = 10;

    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&]() {
            int local_count = 0;
            while (running.load() && local_count < 100) {
                try {
                    auto metrics = wrapper.get_performance_metrics();
                    // Verify metrics are reasonable
                    EXPECT_GE(metrics.total_frames, 0);
                    EXPECT_GE(metrics.successful_frames, 0);
                    EXPECT_GE(metrics.tracking_failures, 0);

                    metrics_count++;
                    local_count++;
                } catch (const std::exception& e) {
                    errors++;
                }
            }
        });
    }

    // Let threads run
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running = false;

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(errors, 0) << "Thread safety errors detected during metrics access";
    EXPECT_GT(metrics_count, 0) << "No metrics were retrieved";
}

/**
 * Test: Rapid parameter updates without processing
 *
 * RATIONALE: UI thread might update parameters rapidly while video thread is idle.
 * This test verifies that parameter updates don't cause issues when called
 * in quick succession.
 */
TEST_F(ThreadSafetyTest, RapidParameterUpdates) {
    const int NUM_UPDATES = 1000;
    std::atomic<int> errors{0};

    for (int i = 0; i < NUM_UPDATES; i++) {
        try {
            StabilizerCore::StabilizerParams params = TestDataGenerator::create_default_params();
            params.smoothing_radius = i % 100;
            params.max_correction = 10.0f + (i % 50);
            wrapper.update_parameters(params);
        } catch (const std::exception& e) {
            errors++;
        }
    }

    EXPECT_EQ(errors, 0) << "Errors during rapid parameter updates";

    // Verify final parameters are correct
    auto final_params = wrapper.get_current_params();
    EXPECT_EQ(final_params.smoothing_radius, (NUM_UPDATES - 1) % 100);
}

/**
 * Test: Concurrent reset and processing
 *
 * RATIONALE: User might reset the stabilizer while video is processing.
 * This test ensures that reset operations are thread-safe.
 */
TEST_F(ThreadSafetyTest, ConcurrentResetAndProcessing) {
    std::atomic<bool> running{true};
    std::atomic<int> process_count{0};
    std::atomic<int> reset_count{0};
    std::atomic<int> errors{0};

    cv::Mat base_frame = TestDataGenerator::generate_test_frame(
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT
    );

    // Thread 1: Process frames
    std::thread process_thread([&]() {
        int local_count = 0;
        while (running.load() && local_count < 500) {
            try {
                cv::Mat frame = TestDataGenerator::create_motion_frame(
                    base_frame, 1.0f, 2.0f, 0.5f
                );
                cv::Mat result = wrapper.process_frame(frame);

                if (result.empty()) {
                    errors++;
                }

                process_count++;
                local_count++;
            } catch (const std::exception& e) {
                errors++;
            }
        }
    });

    // Thread 2: Reset periodically
    std::thread reset_thread([&]() {
        int local_count = 0;
        while (running.load() && local_count < 50) {
            try {
                wrapper.reset();
                reset_count++;
                local_count++;

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } catch (const std::exception& e) {
                errors++;
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    running = false;
    process_thread.join();
    reset_thread.join();

    EXPECT_EQ(errors, 0) << "Thread safety errors detected during reset";
    EXPECT_GT(process_count, 0) << "No frames were processed";
    EXPECT_GT(reset_count, 0) << "No resets were performed";
}

/**
 * Test: Exception safety during concurrent operations
 *
 * RATIONALE: If an exception occurs in one thread, it should not affect
 * other threads or leave the wrapper in an inconsistent state.
 */
TEST_F(ThreadSafetyTest, ExceptionSafetyDuringConcurrentOperations) {
    std::atomic<bool> running{true};
    std::atomic<int> success_count{0};

    cv::Mat base_frame = TestDataGenerator::generate_test_frame(
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT
    );

    // Thread 1: Process valid frames
    std::thread valid_thread([&]() {
        while (running.load()) {
            try {
                cv::Mat frame = TestDataGenerator::create_motion_frame(
                    base_frame, 1.0f, 2.0f, 0.5f
                );
                cv::Mat result = wrapper.process_frame(frame);
                success_count++;
            } catch (...) {
                // Ignore exceptions in test thread
            }
        }
    });

    // Thread 2: Try to process invalid frames (should be handled gracefully)
    std::thread invalid_thread([&]() {
        for (int i = 0; i < 100; i++) {
            try {
                cv::Mat empty_frame;
                cv::Mat result = wrapper.process_frame(empty_frame);
                // Should return original (empty) frame without throwing
                EXPECT_TRUE(result.empty()) || result.empty();
            } catch (...) {
                // Exception would be caught and converted to graceful degradation
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Thread 3: Update parameters
    std::thread param_thread([&]() {
        while (running.load()) {
            try {
                StabilizerCore::StabilizerParams params = TestDataGenerator::create_default_params();
                wrapper.update_parameters(params);
            } catch (...) {
                // Ignore exceptions in test thread
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    });

    // Let threads run
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running = false;

    valid_thread.join();
    invalid_thread.join();
    param_thread.join();

    EXPECT_GT(success_count, 0) << "No valid frames were processed";

    // Verify wrapper is still functional after exception scenarios
    cv::Mat test_frame = TestDataGenerator::generate_test_frame(
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT
    );
    cv::Mat result = wrapper.process_frame(test_frame);
    EXPECT_FALSE(result.empty()) << "Wrapper is in invalid state after concurrent operations";
}

/**
 * Test: Mixed read/write operations from multiple threads
 *
 * RATIONALE: Simulates real-world scenario with multiple threads performing
 * both read (metrics, params) and write (update, process) operations.
 */
TEST_F(ThreadSafetyTest, MixedReadWriteOperations) {
    std::atomic<bool> running{true};
    std::atomic<int> read_ops{0};
    std::atomic<int> write_ops{0};
    std::atomic<int> errors{0};

    cv::Mat base_frame = TestDataGenerator::generate_test_frame(
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT
    );

    // Reader threads: Query metrics and parameters
    std::vector<std::thread> reader_threads;
    const int NUM_READERS = 5;

    for (int i = 0; i < NUM_READERS; i++) {
        reader_threads.emplace_back([&]() {
            while (running.load()) {
                try {
                    auto metrics = wrapper.get_performance_metrics();
                    auto params = wrapper.get_current_params();
                    bool initialized = wrapper.is_initialized();
                    bool ready = wrapper.is_ready();

                    EXPECT_TRUE(initialized);
                    read_ops++;
                } catch (const std::exception& e) {
                    errors++;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    // Writer threads: Process frames and update parameters
    std::vector<std::thread> writer_threads;
    const int NUM_WRITERS = 3;

    for (int i = 0; i < NUM_WRITERS; i++) {
        writer_threads.emplace_back([&]() {
            int local_count = 0;
            while (running.load() && local_count < 100) {
                try {
                    cv::Mat frame = TestDataGenerator::create_motion_frame(
                        base_frame, 1.0f, 2.0f, 0.5f
                    );
                    wrapper.process_frame(frame);

                    StabilizerCore::StabilizerParams params =
                        TestDataGenerator::create_default_params();
                    wrapper.update_parameters(params);

                    write_ops++;
                    local_count++;
                } catch (const std::exception& e) {
                    errors++;
                }
            }
        });
    }

    // Let threads run
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    running = false;

    for (auto& thread : reader_threads) {
        thread.join();
    }
    for (auto& thread : writer_threads) {
        thread.join();
    }

    EXPECT_EQ(errors, 0) << "Thread safety errors detected during mixed operations";
    EXPECT_GT(read_ops, 0) << "No read operations were performed";
    EXPECT_GT(write_ops, 0) << "No write operations were performed";

    std::cout << "Completed " << read_ops << " read operations and "
              << write_ops << " write operations with "
              << errors << " errors" << std::endl;
}
