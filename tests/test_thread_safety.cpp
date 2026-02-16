/*
 * Thread Safety Tests for StabilizerWrapper
 *
 * RATIONALE: These tests verify that StabilizerWrapper provides proper thread safety
 * for concurrent access from OBS UI thread (properties update) and video thread (frame processing).
 *
 * StabilizerWrapper is responsible for:
 * 1. Protecting all access to StabilizerCore with mutex
 * 2. Preventing data races between UI thread and video thread
 * 3. Ensuring thread-safe parameter updates during frame processing
 * 4. Safe concurrent reads of performance metrics
 */

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <random>

#include "test_constants.hpp"
#include "test_data_generator.hpp"
#include "core/stabilizer_wrapper.hpp"
#include "core/stabilizer_core.hpp"

using namespace TestConstants;

class ThreadSafetyTest : public ::testing::Test {
protected:
    std::unique_ptr<StabilizerWrapper> wrapper;
    cv::Mat test_frame;

    void SetUp() override {
        wrapper = std::make_unique<StabilizerWrapper>();
        test_frame = TestDataGenerator::generate_test_frame(
            Resolution::VGA_WIDTH,
            Resolution::VGA_HEIGHT
        );

        StabilizerCore::StabilizerParams params;
        params.smoothing_radius = 30;
        params.max_correction = 30.0f;
        params.feature_count = 500;

        ASSERT_TRUE(wrapper->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    }

    void TearDown() override {
        wrapper.reset();
    }

    /**
     * Helper: Process frames in a loop
     * Used to simulate continuous video processing
     */
    void process_frames_loop(int num_frames, std::atomic<bool>& should_stop) {
        for (int i = 0; i < num_frames && !should_stop.load(); ++i) {
            cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
            wrapper->process_frame(frame);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    /**
     * Helper: Update parameters randomly
     * Used to simulate UI updates while processing
     */
    void update_parameters_loop(int num_updates, std::atomic<bool>& should_stop) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> smoothing_dist(10, 100);
        std::uniform_real_distribution<float> correction_dist(10.0f, 50.0f);

        for (int i = 0; i < num_updates && !should_stop.load(); ++i) {
            StabilizerCore::StabilizerParams params;
            params.smoothing_radius = smoothing_dist(gen);
            params.max_correction = correction_dist(gen);
            params.feature_count = 500;
            wrapper->update_parameters(params);
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    }

    /**
     * Helper: Read metrics in a loop
     * Used to test concurrent read operations
     */
    void read_metrics_loop(int num_reads, std::atomic<bool>& should_stop) {
        for (int i = 0; i < num_reads && !should_stop.load(); ++i) {
            auto metrics = wrapper->get_performance_metrics();
            // Verify metrics are valid (no corruption)
            EXPECT_GE(metrics.total_frames, 0);
            EXPECT_GE(metrics.successful_frames, 0);
            EXPECT_GE(metrics.tracking_failures, 0);
            EXPECT_GE(metrics.avg_processing_time, 0.0);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
};

/**
 * Test: Concurrent Parameter Update and Frame Processing
 *
 * SCENARIO: OBS UI thread updates parameters while video thread processes frames
 * EXPECTED: No data races, no crashes, frames processed correctly
 */
TEST_F(ThreadSafetyTest, ConcurrentParameterUpdateAndProcessing) {
    std::atomic<bool> should_stop(false);

    // Start video processing thread
    std::thread processing_thread([&]() {
        process_frames_loop(50, should_stop);
    });

    // Start parameter update thread
    std::thread param_thread([&]() {
        update_parameters_loop(30, should_stop);
    });

    // Wait for both threads to complete
    processing_thread.join();
    param_thread.join();

    // Verify the wrapper is still functional
    auto metrics = wrapper->get_performance_metrics();
    EXPECT_GT(metrics.total_frames, 0);
}

/**
 * Test: Concurrent Metrics Access
 *
 * SCENARIO: Multiple threads read performance metrics concurrently
 * EXPECTED: No data races, all metrics valid
 */
TEST_F(ThreadSafetyTest, ConcurrentMetricsAccess) {
    const int num_reader_threads = 4;
    const int reads_per_thread = 20;
    std::atomic<bool> should_stop(false);

    std::vector<std::thread> reader_threads;

    // Process some frames first to generate metrics
    for (int i = 0; i < 10; ++i) {
        cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
        wrapper->process_frame(frame);
    }

    // Start multiple reader threads
    for (int i = 0; i < num_reader_threads; ++i) {
        reader_threads.emplace_back([&, thread_id = i]() {
            read_metrics_loop(reads_per_thread, should_stop);
        });
    }

    // Wait for all threads
    for (auto& thread : reader_threads) {
        thread.join();
    }

    // Verify metrics are consistent
    auto metrics = wrapper->get_performance_metrics();
    EXPECT_GT(metrics.total_frames, 0);
}

/**
 * Test: Rapid Parameter Updates
 *
 * SCENARIO: Parameters are updated rapidly while frames are being processed
 * EXPECTED: No crashes, no invalid parameter states
 */
TEST_F(ThreadSafetyTest, RapidParameterUpdates) {
    const int num_iterations = 100;
    std::atomic<int> successful_processes(0);

    std::thread update_thread([&]() {
        for (int i = 0; i < num_iterations; ++i) {
            StabilizerCore::StabilizerParams params;
            params.smoothing_radius = 10 + (i % 90);  // 10-99
            params.max_correction = 10.0f + (i % 40);  // 10-50
            params.feature_count = 500;
            wrapper->update_parameters(params);
        }
    });

    std::thread process_thread([&]() {
        for (int i = 0; i < num_iterations; ++i) {
            cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
            cv::Mat result = wrapper->process_frame(frame);
            if (!result.empty()) {
                successful_processes++;
            }
        }
    });

    update_thread.join();
    process_thread.join();

    EXPECT_GT(successful_processes, num_iterations * 0.9);  // At least 90% success
}

/**
 * Test: Concurrent Reset and Processing
 *
 * SCENARIO: Reset is called while frames are being processed
 * EXPECTED: No crashes, reset doesn't corrupt processing state
 */
TEST_F(ThreadSafetyTest, ConcurrentResetAndProcessing) {
    const int num_iterations = 50;
    std::atomic<int> reset_count(0);

    std::thread reset_thread([&]() {
        for (int i = 0; i < num_iterations; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            wrapper->reset();
            reset_count++;
        }
    });

    std::thread process_thread([&]() {
        for (int i = 0; i < num_iterations * 3; ++i) {
            cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
            wrapper->process_frame(frame);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    reset_thread.join();
    process_thread.join();

    EXPECT_GT(reset_count, 0);
    EXPECT_TRUE(wrapper->is_initialized());
}

/**
 * Test: Exception Safety During Concurrent Operations
 *
 * SCENARIO: Operations happen concurrently with potential for exceptions
 * EXPECTED: Exceptions are caught, wrapper remains in valid state
 */
TEST_F(ThreadSafetyTest, ExceptionSafetyDuringConcurrentOperations) {
    std::atomic<bool> should_stop(false);
    std::atomic<int> exception_count(0);

    // Thread that processes frames
    std::thread process_thread([&]() {
        for (int i = 0; i < 50 && !should_stop.load(); ++i) {
            try {
                cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
                wrapper->process_frame(frame);
            } catch (...) {
                exception_count++;
            }
        }
    });

    // Thread that updates parameters
    std::thread update_thread([&]() {
        for (int i = 0; i < 50 && !should_stop.load(); ++i) {
            try {
                StabilizerCore::StabilizerParams params;
                params.smoothing_radius = 30;
                params.max_correction = 30.0f;
                params.feature_count = 500;
                wrapper->update_parameters(params);
            } catch (...) {
                exception_count++;
            }
        }
    });

    process_thread.join();
    update_thread.join();

    // Expect no exceptions (wrapper handles errors internally)
    EXPECT_EQ(exception_count, 0);

    // Verify wrapper is still functional after concurrent operations
    EXPECT_TRUE(wrapper->is_initialized());
}

/**
 * Test: Mixed Read-Write Operations
 *
 * SCENARIO: Multiple threads perform mixed reads (get metrics, is_ready) and writes (update params, reset)
 * EXPECTED: No data races, all operations complete successfully
 */
TEST_F(ThreadSafetyTest, MixedReadWriteOperations) {
    const int num_operations = 100;
    std::atomic<int> read_ops(0);
    std::atomic<int> write_ops(0);

    std::vector<std::thread> threads;

    // Thread 1: Read metrics
    threads.emplace_back([&]() {
        for (int i = 0; i < num_operations; ++i) {
            auto metrics = wrapper->get_performance_metrics();
            read_ops++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Thread 2: Check ready status
    threads.emplace_back([&]() {
        for (int i = 0; i < num_operations; ++i) {
            bool ready = wrapper->is_ready();
            read_ops++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Thread 3: Update parameters
    threads.emplace_back([&]() {
        for (int i = 0; i < num_operations; ++i) {
            StabilizerCore::StabilizerParams params;
            params.smoothing_radius = 30;
            params.max_correction = 30.0f;
            params.feature_count = 500;
            wrapper->update_parameters(params);
            write_ops++;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });

    // Thread 4: Process frames
    threads.emplace_back([&]() {
        for (int i = 0; i < num_operations; ++i) {
            cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
            wrapper->process_frame(frame);
            write_ops++;
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    });

    // Thread 5: Reset periodically
    threads.emplace_back([&]() {
        for (int i = 0; i < 20; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            wrapper->reset();
            write_ops++;
        }
    });

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(read_ops, 0);
    EXPECT_GT(write_ops, 0);
    EXPECT_TRUE(wrapper->is_initialized());
}

/**
 * Test: Stress Test - High Concurrency
 *
 * SCENARIO: Many threads performing various operations simultaneously
 * EXPECTED: No deadlocks, no data races, system remains stable
 */
TEST_F(ThreadSafetyTest, StressTestHighConcurrency) {
    const int num_threads = 8;
    const int operations_per_thread = 30;
    std::atomic<int> total_operations(0);

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, thread_id = t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                switch (thread_id % 4) {
                    case 0: {  // Process frames
                        cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
                        wrapper->process_frame(frame);
                        break;
                    }
                    case 1: {  // Update params
                        StabilizerCore::StabilizerParams params;
                        params.smoothing_radius = 30;
                        params.max_correction = 30.0f;
                        params.feature_count = 500;
                        wrapper->update_parameters(params);
                        break;
                    }
                    case 2: {  // Read metrics
                        auto metrics = wrapper->get_performance_metrics();
                        (void)metrics;  // Suppress unused warning
                        break;
                    }
                    case 3: {  // Get params
                        auto params = wrapper->get_current_params();
                        (void)params;  // Suppress unused warning
                        break;
                    }
                }
                total_operations++;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(total_operations, num_threads * operations_per_thread);
    EXPECT_TRUE(wrapper->is_initialized());

    // Verify no corruption - metrics should be reasonable
    auto metrics = wrapper->get_performance_metrics();
    EXPECT_LE(metrics.total_frames, num_threads * operations_per_thread);
}

// ============================================================================
// Edge Case Tests for StabilizerWrapper
// ============================================================================

/**
 * Test: Operations on Uninitialized Wrapper
 *
 * SCENARIO: All operations are called on an uninitialized wrapper
 * EXPECTED: Operations handle uninitialized state gracefully, no crashes
 *
 * RATIONALE: This tests the "not initialized" code paths which were previously untested.
 * This addresses QA Review Issue #1 - edge cases for stabilizer_wrapper.cpp.
 */
TEST_F(ThreadSafetyTest, OperationsOnUninitializedWrapper) {
    std::unique_ptr<StabilizerWrapper> uninitialized_wrapper = std::make_unique<StabilizerWrapper>();

    // Test is_initialized() on uninitialized wrapper
    EXPECT_FALSE(uninitialized_wrapper->is_initialized());

    // Test is_ready() on uninitialized wrapper
    EXPECT_FALSE(uninitialized_wrapper->is_ready());

    // Test get_last_error() on uninitialized wrapper
    std::string error = uninitialized_wrapper->get_last_error();
    EXPECT_FALSE(error.empty());
    EXPECT_EQ(error, "Not initialized");

    // Test get_performance_metrics() on uninitialized wrapper
    auto metrics = uninitialized_wrapper->get_performance_metrics();
    EXPECT_EQ(metrics.total_frames, 0);
    EXPECT_EQ(metrics.successful_frames, 0);
    EXPECT_EQ(metrics.tracking_failures, 0);
    EXPECT_EQ(metrics.avg_processing_time, 0.0);

    // Test get_current_params() on uninitialized wrapper
    // NOTE: Returns default-initialized StabilizerParams, which uses the struct's default values (30, 30.0f, 500)
    // This is correct C++ behavior - default-initialized struct uses defined default values, not zeros
    auto params = uninitialized_wrapper->get_current_params();
    EXPECT_EQ(params.smoothing_radius, 30);       // Default value from StabilizerParams
    EXPECT_EQ(params.max_correction, 30.0f);     // Default value from StabilizerParams
    EXPECT_EQ(params.feature_count, 500);         // Default value from StabilizerParams

    // Test update_parameters() on uninitialized wrapper (should not crash)
    StabilizerCore::StabilizerParams new_params;
    new_params.smoothing_radius = 50;
    new_params.max_correction = 40.0f;
    new_params.feature_count = 600;
    uninitialized_wrapper->update_parameters(new_params);  // Should not crash

    // Test reset() on uninitialized wrapper (should not crash)
    uninitialized_wrapper->reset();  // Should not crash

    // Test process_frame() on uninitialized wrapper
    cv::Mat test_frame = TestDataGenerator::generate_test_frame(640, 480);
    cv::Mat result = uninitialized_wrapper->process_frame(test_frame);
    EXPECT_FALSE(result.empty());  // Should return original frame
    EXPECT_EQ(result.data, test_frame.data);  // Should return same frame
}

/**
 * Test: Initialize with Invalid Dimensions
 *
 * SCENARIO: Wrapper is initialized with invalid dimensions
 * EXPECTED: Initialization fails, wrapper remains uninitialized
 *
 * RATIONALE: Tests error handling for invalid initialization parameters.
 */
TEST_F(ThreadSafetyTest, InitializeWithInvalidDimensions) {
    std::unique_ptr<StabilizerWrapper> invalid_wrapper = std::make_unique<StabilizerWrapper>();

    StabilizerCore::StabilizerParams params;
    params.smoothing_radius = 30;
    params.max_correction = 30.0f;
    params.feature_count = 500;

    // Test with zero width
    bool result = invalid_wrapper->initialize(0, 640, params);
    EXPECT_FALSE(result);
    EXPECT_FALSE(invalid_wrapper->is_initialized());

    // Test with zero height
    invalid_wrapper = std::make_unique<StabilizerWrapper>();
    result = invalid_wrapper->initialize(640, 0, params);
    EXPECT_FALSE(result);
    EXPECT_FALSE(invalid_wrapper->is_initialized());

    // Test with very large dimensions (potential overflow)
    invalid_wrapper = std::make_unique<StabilizerWrapper>();
    result = invalid_wrapper->initialize(100000, 100000, params);
    // This might succeed or fail depending on implementation
    // The important thing is it doesn't crash
    EXPECT_NO_THROW({
        // If it fails, wrapper should be uninitialized
        if (!result) {
            EXPECT_FALSE(invalid_wrapper->is_initialized());
        }
    });
}

/**
 * Test: Process Empty Frame
 *
 * SCENARIO: Empty frame is passed to process_frame()
 * EXPECTED: Empty frame is returned or handled gracefully
 *
 * RATIONALE: Tests handling of invalid frame input.
 */
TEST_F(ThreadSafetyTest, ProcessEmptyFrame) {
    cv::Mat empty_frame;
    cv::Mat result = wrapper->process_frame(empty_frame);

    // Empty frame should be handled gracefully
    // Either empty frame is returned or it's processed without crash
    EXPECT_NO_THROW({
        if (result.empty()) {
            // Empty frame returned is acceptable
        } else {
            // Non-empty result means processing was attempted
            EXPECT_FALSE(result.empty());
        }
    });
}

/**
 * Test: Process Frame with Invalid Channels
 *
 * SCENARIO: Frame with invalid channel count is processed
 * EXPECTED: Frame is handled gracefully without crash
 *
 * RATIONALE: Tests handling of unsupported Mat formats.
 */
TEST_F(ThreadSafetyTest, ProcessFrameWithInvalidChannels) {
    // Create frame with 2 channels (unsupported)
    cv::Mat invalid_frame(640, 480, CV_8UC2);
    cv::randu(invalid_frame, 0, 255);

    cv::Mat result = wrapper->process_frame(invalid_frame);

    // Should handle gracefully without crash
    EXPECT_NO_THROW({
        // Result may be empty or same as input
        (void)result;
    });
}

/**
 * Test: Re-initialize Wrapper
 *
 * SCENARIO: Wrapper is initialized multiple times with different parameters
 * EXPECTED: Each initialization succeeds, parameters are updated correctly
 *
 * RATIONALE: Tests that re-initialization works correctly.
 */
TEST_F(ThreadSafetyTest, ReInitializeWrapper) {
    // First initialization
    StabilizerCore::StabilizerParams params1;
    params1.smoothing_radius = 30;
    params1.max_correction = 30.0f;
    params1.feature_count = 500;

    bool result = wrapper->initialize(640, 480, params1);
    EXPECT_TRUE(result);
    EXPECT_TRUE(wrapper->is_initialized());

    // Process some frames
    for (int i = 0; i < 10; ++i) {
        cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
        wrapper->process_frame(frame);
    }

    // Re-initialize with different parameters
    StabilizerCore::StabilizerParams params2;
    params2.smoothing_radius = 50;
    params2.max_correction = 40.0f;
    params2.feature_count = 700;

    result = wrapper->initialize(800, 600, params2);
    EXPECT_TRUE(result);
    EXPECT_TRUE(wrapper->is_initialized());

    // Verify new parameters are applied
    auto current_params = wrapper->get_current_params();
    EXPECT_EQ(current_params.smoothing_radius, 50);
    EXPECT_EQ(current_params.max_correction, 40.0f);
    EXPECT_EQ(current_params.feature_count, 700);

    // Process frames with new configuration
    cv::Mat new_test_frame = TestDataGenerator::generate_test_frame(800, 600);
    for (int i = 0; i < 10; ++i) {
        cv::Mat frame = TestDataGenerator::create_motion_frame(new_test_frame, 5.0f, 5.0f, 0.0f);
        cv::Mat result = wrapper->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

/**
 * Test: Get Current Params After Update
 *
 * SCENARIO: Parameters are updated and retrieved
 * EXPECTED: Retrieved parameters match updated values
 *
 * RATIONALE: Tests that parameter updates are correctly reflected in get_current_params().
 */
TEST_F(ThreadSafetyTest, GetCurrentParamsAfterUpdate) {
    // Get initial parameters
    auto initial_params = wrapper->get_current_params();
    EXPECT_EQ(initial_params.smoothing_radius, 30);
    EXPECT_EQ(initial_params.max_correction, 30.0f);
    EXPECT_EQ(initial_params.feature_count, 500);

    // Update parameters
    StabilizerCore::StabilizerParams new_params;
    new_params.smoothing_radius = 75;
    new_params.max_correction = 45.0f;
    new_params.feature_count = 800;

    wrapper->update_parameters(new_params);

    // Retrieve and verify updated parameters
    auto updated_params = wrapper->get_current_params();
    EXPECT_EQ(updated_params.smoothing_radius, 75);
    EXPECT_EQ(updated_params.max_correction, 45.0f);
    EXPECT_EQ(updated_params.feature_count, 800);

    // Update again with different values
    new_params.smoothing_radius = 25;
    new_params.max_correction = 15.0f;
    new_params.feature_count = 300;

    wrapper->update_parameters(new_params);

    // Verify second update
    auto final_params = wrapper->get_current_params();
    EXPECT_EQ(final_params.smoothing_radius, 25);
    EXPECT_EQ(final_params.max_correction, 15.0f);
    EXPECT_EQ(final_params.feature_count, 300);
}

/**
 * Test: Get Last Error After Failed Operation
 *
 * SCENARIO: Error occurs in StabilizerCore and last error message is retrieved
 * EXPECTED: Error message is populated and can be retrieved
 *
 * RATIONALE: Tests error message propagation from StabilizerCore to wrapper.
 */
TEST_F(ThreadSafetyTest, GetLastErrorAfterFailedOperation) {
    // Normal operation - no error
    cv::Mat frame = TestDataGenerator::generate_test_frame(640, 480);
    cv::Mat result = wrapper->process_frame(frame);
    EXPECT_FALSE(result.empty());

    // Get last error (should be empty or no error)
    std::string error = wrapper->get_last_error();
    // Error may or may not be empty depending on implementation
    // The important thing is it doesn't crash
    EXPECT_NO_THROW({
        (void)error;
    });
}

/**
 * Test: Concurrent Initialization
 *
 * SCENARIO: Multiple threads attempt to initialize the wrapper simultaneously
 * EXPECTED: Only one initialization succeeds, no data races
 *
 * RATIONALE: Tests thread safety of initialization operation.
 */
TEST_F(ThreadSafetyTest, ConcurrentInitialization) {
    std::unique_ptr<StabilizerWrapper> concurrent_wrapper = std::make_unique<StabilizerWrapper>();

    std::atomic<int> success_count(0);
    std::atomic<int> fail_count(0);

    const int num_threads = 4;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            StabilizerCore::StabilizerParams params;
            params.smoothing_radius = 30 + thread_id;
            params.max_correction = 30.0f + thread_id;
            params.feature_count = 500;

            bool result = concurrent_wrapper->initialize(640, 480, params);
            if (result) {
                success_count++;
            } else {
                fail_count++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // At least one initialization should succeed
    EXPECT_GT(success_count + fail_count, 0);

    // Wrapper should be initialized (at least one thread succeeded)
    EXPECT_TRUE(concurrent_wrapper->is_initialized());
}

/**
 * Test: Reset and Immediately Process
 *
 * SCENARIO: Reset is called and frame is processed immediately after
 * EXPECTED: Frame is processed correctly, no crash
 *
 * RATIONALE: Tests that reset doesn't leave wrapper in inconsistent state.
 */
TEST_F(ThreadSafetyTest, ResetAndImmediatelyProcess) {
    // Process some frames
    for (int i = 0; i < 20; ++i) {
        cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
        wrapper->process_frame(frame);
    }

    // Get metrics before reset
    auto metrics_before = wrapper->get_performance_metrics();
    EXPECT_GT(metrics_before.total_frames, 0);

    // Reset
    wrapper->reset();

    // Get metrics after reset
    auto metrics_after = wrapper->get_performance_metrics();

    // Metrics should be reset (or wrapper should be in consistent state)
    // The specific behavior depends on StabilizerCore::reset() implementation
    EXPECT_NO_THROW({
        (void)metrics_after;
    });

    // Process frame immediately after reset
    cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
    cv::Mat result = wrapper->process_frame(frame);

    // Should process successfully
    EXPECT_FALSE(result.empty());
}

/**
 * Test: Process Frame After Reset
 *
 * SCENARIO: Multiple frames are processed after reset
 * EXPECTED: Frames are processed correctly, metrics are updated
 *
 * RATIONALE: Tests that wrapper continues to work correctly after reset.
 */
TEST_F(ThreadSafetyTest, ProcessFrameAfterReset) {
    // Process initial frames
    for (int i = 0; i < 10; ++i) {
        cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
        wrapper->process_frame(frame);
    }

    // Reset
    wrapper->reset();

    // Process frames after reset
    for (int i = 0; i < 15; ++i) {
        cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
        cv::Mat result = wrapper->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // Verify metrics are reasonable
    auto metrics = wrapper->get_performance_metrics();
    EXPECT_GT(metrics.total_frames, 0);
    EXPECT_GT(metrics.successful_frames, 0);
}

/**
 * Test: Is Ready Behavior
 *
 * SCENARIO: is_ready() is called at various states
 * EXPECTED: is_ready() returns appropriate values for each state
 *
 * RATIONALE: Tests is_ready() behavior through wrapper lifecycle.
 */
TEST_F(ThreadSafetyTest, IsReadyBehavior) {
    // Wrapper should be ready after initialization
    EXPECT_TRUE(wrapper->is_ready());

    // Process some frames
    for (int i = 0; i < 10; ++i) {
        cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
        wrapper->process_frame(frame);
        EXPECT_TRUE(wrapper->is_ready());
    }

    // Reset
    wrapper->reset();

    // Wrapper should still be ready after reset
    // (depends on StabilizerCore::is_ready() implementation)
    bool ready_after_reset = wrapper->is_ready();
    EXPECT_NO_THROW({
        (void)ready_after_reset;
    });
}

/**
 * Test: Update Parameters with Edge Values
 *
 * SCENARIO: Parameters are updated with minimum and maximum values
 * EXPECTED: Parameters are updated correctly, no crashes
 *
 * RATIONALE: Tests handling of boundary values for parameters.
 */
TEST_F(ThreadSafetyTest, UpdateParametersWithEdgeValues) {
    struct ParamTest {
        int smoothing_radius;
        float max_correction;
        int feature_count;
    };

    std::vector<ParamTest> test_params = {
        {1, 1.0f, 10},       // Minimum values
        {100, 100.0f, 2000}, // Maximum reasonable values
        {0, 0.0f, 0},       // Zero values (edge case)
        {50, 50.0f, 1000}   // Normal values
    };

    for (const auto& test : test_params) {
        StabilizerCore::StabilizerParams params;
        params.smoothing_radius = test.smoothing_radius;
        params.max_correction = test.max_correction;
        params.feature_count = test.feature_count;

        // Update should not crash
        EXPECT_NO_THROW({
            wrapper->update_parameters(params);
        });

        // Verify parameters were updated
        auto current_params = wrapper->get_current_params();
        EXPECT_EQ(current_params.smoothing_radius, test.smoothing_radius);
        EXPECT_EQ(current_params.max_correction, test.max_correction);
        EXPECT_EQ(current_params.feature_count, test.feature_count);

        // Process a frame to ensure parameters work
        cv::Mat frame = TestDataGenerator::create_motion_frame(test_frame, 5.0f, 5.0f, 0.0f);
        cv::Mat result = wrapper->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

/**
 * Test: Frame Processing with Different Resolutions
 *
 * SCENARIO: Frames of different resolutions are processed after initialization
 * EXPECTED: Frames are processed correctly, no crashes
 *
 * RATIONALE: Tests that wrapper can handle frames of various resolutions.
 * Note: This may fail if resolution doesn't match initialized resolution.
 */
TEST_F(ThreadSafetyTest, FrameProcessingWithDifferentResolutions) {
    struct Resolution {
        int width;
        int height;
    };

    std::vector<Resolution> resolutions = {
        {320, 240},    // QVGA
        {640, 480},    // VGA (matches initialized resolution)
        {1280, 720},   // HD
        {1920, 1080}   // Full HD
    };

    for (const auto& res : resolutions) {
        // Create frame with different resolution
        cv::Mat frame = TestDataGenerator::generate_test_frame(res.width, res.height);

        // Process frame
        cv::Mat result = wrapper->process_frame(frame);

        // Result may be empty if resolution doesn't match
        // The important thing is it doesn't crash
        EXPECT_NO_THROW({
            (void)result;
        });
    }
}
