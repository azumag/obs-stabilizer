/**
 * Visual Stabilization Quality Tests
 *
 * This file contains tests to verify the visual quality of stabilization.
 * Tests measure shake magnitude before and after stabilization, ensuring
 * that the stabilizer effectively reduces unwanted camera motion.
 *
 * Critical acceptance criteria:
 * - 手振れ補正が視覚的に確認できる（明らかな揺れの低減）
 * - Shake reduction should be >50% for typical cases
 */

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "../src/core/stabilizer_core.hpp"
#include "../src/core/stabilizer_wrapper.hpp"
#include "test_constants.hpp"
#include "test_data_generator.hpp"
#include <cmath>
#include <numeric>
#include <vector>

using namespace TestConstants;

// ============================================================================
// Helper Functions for Shake Measurement
// ============================================================================

/**
 * Calculate motion vectors between two frames using optical flow
 * Returns a vector of (dx, dy) motion components for tracked points
 */
std::vector<cv::Point2f> calculate_motion_vectors(
    const cv::Mat& prev_frame,
    const cv::Mat& curr_frame
) {
    std::vector<cv::Point2f> prev_pts;
    std::vector<cv::Point2f> curr_pts;
    std::vector<uint8_t> status;
    std::vector<float> err;

    // Detect features in previous frame
    cv::goodFeaturesToTrack(
        prev_frame, prev_pts,
        Features::DEFAULT_COUNT,
        Processing::DEFAULT_QUALITY_LEVEL,
        Processing::DEFAULT_MIN_DISTANCE
    );

    if (prev_pts.empty()) {
        return {};
    }

    // Track features to current frame
    cv::calcOpticalFlowPyrLK(
        prev_frame, curr_frame,
        prev_pts, curr_pts,
        status, err
    );

    // Filter successful tracks and calculate motion vectors
    std::vector<cv::Point2f> motion_vectors;
    for (size_t i = 0; i < status.size(); i++) {
        if (status[i] && err[i] < 30.0f) {
            // Motion vector = current position - previous position
            cv::Point2f motion(
                curr_pts[i].x - prev_pts[i].x,
                curr_pts[i].y - prev_pts[i].y
            );
            motion_vectors.push_back(motion);
        }
    }

    return motion_vectors;
}

/**
 * Calculate average shake magnitude from motion vectors
 * Shake = sqrt(dx^2 + dy^2) averaged over all points
 */
double calculate_shake_magnitude(const std::vector<cv::Point2f>& motion_vectors) {
    if (motion_vectors.empty()) {
        return 0.0;
    }

    double total_magnitude = 0.0;
    for (const auto& motion : motion_vectors) {
        double magnitude = std::sqrt(
            motion.x * motion.x + motion.y * motion.y
        );
        total_magnitude += magnitude;
    }

    return total_magnitude / motion_vectors.size();
}

/**
 * Calculate shake magnitude variance (measures instability)
 */
double calculate_shake_variance(const std::vector<double>& magnitudes) {
    if (magnitudes.empty()) {
        return 0.0;
    }

    double mean = std::accumulate(magnitudes.begin(), magnitudes.end(), 0.0) / magnitudes.size();
    double variance = 0.0;
    for (double mag : magnitudes) {
        variance += (mag - mean) * (mag - mean);
    }
    return variance / magnitudes.size();
}

/**
 * Compare frame stability by measuring edge movement
 * Returns a stability score (lower = more stable)
 */
double calculate_edge_movement(const cv::Mat& frame1, const cv::Mat& frame2) {
    // Detect edges using Canny
    cv::Mat edges1, edges2;
    cv::Canny(frame1, edges1, 50, 150);
    cv::Canny(frame2, edges2, 50, 150);

    // Calculate difference in edge positions
    cv::Mat diff;
    cv::absdiff(edges1, edges2, diff);

    // Count non-zero pixels (edge movement)
    int movement = cv::countNonZero(diff);

    // Normalize by frame area
    double frame_area = frame1.cols * frame1.rows;
    return static_cast<double>(movement) / frame_area;
}

// ============================================================================
// Visual Quality Test Class
// ============================================================================

class VisualStabilizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        stabilizer = std::make_unique<StabilizerCore>();
    }

    void TearDown() override {
        stabilizer.reset();
    }

    StabilizerCore::StabilizerParams getDefaultParams() {
        StabilizerCore::StabilizerParams params;
        params.smoothing_radius = Processing::MEDIUM_SMOOTHING_WINDOW;
        params.max_correction = 50.0f;
        params.feature_count = Features::DEFAULT_COUNT;
        params.quality_level = Processing::DEFAULT_QUALITY_LEVEL;
        params.min_distance = Processing::DEFAULT_MIN_DISTANCE;
        return params;
    }

    /**
     * Process a sequence and calculate shake magnitudes
     * Returns pair of (before_shake, after_shake)
     */
    std::pair<double, double> calculate_shake_reduction(
        const std::vector<cv::Mat>& frames,
        const StabilizerCore::StabilizerParams& params
    ) {
        if (frames.empty()) {
            return {0.0, 0.0};
        }

        // Calculate shake magnitudes from original sequence (before stabilization)
        std::vector<double> before_shakes;
        for (size_t i = 1; i < frames.size(); i++) {
            // Convert to grayscale for optical flow
            cv::Mat gray_prev, gray_curr;
            cv::cvtColor(frames[i-1], gray_prev, cv::COLOR_BGRA2GRAY);
            cv::cvtColor(frames[i], gray_curr, cv::COLOR_BGRA2GRAY);

            auto motion_vectors = calculate_motion_vectors(gray_prev, gray_curr);
            double shake = calculate_shake_magnitude(motion_vectors);
            before_shakes.push_back(shake);
        }

        // Initialize stabilizer
        ASSERT_TRUE(stabilizer->initialize(frames[0].cols, frames[0].rows, params));

        // Process frames and calculate shake magnitudes (after stabilization)
        std::vector<cv::Mat> stabilized_frames;
        for (const auto& frame : frames) {
            cv::Mat result = stabilizer->process_frame(frame);
            if (!result.empty()) {
                stabilized_frames.push_back(result);
            }
        }

        std::vector<double> after_shakes;
        for (size_t i = 1; i < stabilized_frames.size(); i++) {
            cv::Mat gray_prev, gray_curr;
            cv::cvtColor(stabilized_frames[i-1], gray_prev, cv::COLOR_BGRA2GRAY);
            cv::cvtColor(stabilized_frames[i], gray_curr, cv::COLOR_BGRA2GRAY);

            auto motion_vectors = calculate_motion_vectors(gray_prev, gray_curr);
            double shake = calculate_shake_magnitude(motion_vectors);
            after_shakes.push_back(shake);
        }

        // Calculate average shake magnitudes
        double avg_before = 0.0, avg_after = 0.0;
        if (!before_shakes.empty()) {
            avg_before = std::accumulate(before_shakes.begin(), before_shakes.end(), 0.0) / before_shakes.size();
        }
        if (!after_shakes.empty()) {
            avg_after = std::accumulate(after_shakes.begin(), after_shakes.end(), 0.0) / after_shakes.size();
        }

        return {avg_before, avg_after};
    }

    std::unique_ptr<StabilizerCore> stabilizer;
};

// ============================================================================
// Visual Shake Reduction Tests
// ============================================================================

/**
 * Test: Shake reduction for camera shake motion
 * Acceptance Criteria: 手振れ補正が視覚的に確認できる
 * Requirement: Shake should be reduced by >50% for typical shake
 */
TEST_F(VisualStabilizationTest, ShakeReductionForCameraShake) {
    // Generate frames with known camera shake pattern
    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    StabilizerCore::StabilizerParams params = getDefaultParams();
    auto [before_shake, after_shake] = calculate_shake_reduction(frames, params);

    // Shake should be reduced by at least 50%
    double reduction = (before_shake - after_shake) / before_shake;
    EXPECT_GT(reduction, 0.50) << "Expected >50% shake reduction, got: "
        << (reduction * 100.0) << "% (before: " << before_shake
        << ", after: " << after_shake << ")";

    // After shake should be significantly less than before shake
    EXPECT_LT(after_shake, before_shake * 0.5) << "After shake should be <50% of before shake";
}

/**
 * Test: Shake reduction for hand tremor motion
 * Simulates natural hand tremor (small, high-frequency movements)
 */
TEST_F(VisualStabilizationTest, ShakeReductionForHandTremor) {
    // Generate frames with hand tremor pattern (small, rapid movements)
    std::vector<cv::Mat> tremor_frames;
    cv::Mat base_frame = TestDataGenerator::generate_test_frame(
        Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT
    );

    for (int i = 0; i < 50; i++) {
        // Add small random tremor (0-3 pixels)
        float dx = (rand() % 60 - 30) / 10.0f;
        float dy = (rand() % 60 - 30) / 10.0f;

        cv::Mat tremor_frame = TestDataGenerator::create_motion_frame(
            base_frame, dx, dy, 0.0f
        );
        tremor_frames.push_back(tremor_frame);
    }

    StabilizerCore::StabilizerParams params = getDefaultParams();
    auto [before_shake, after_shake] = calculate_shake_reduction(tremor_frames, params);

    // Hand tremor should be significantly reduced
    double reduction = (before_shake - after_shake) / std::max(before_shake, 0.001);
    EXPECT_GT(reduction, 0.60) << "Hand tremor should be reduced by >60%, got: "
        << (reduction * 100.0) << "%";
}

/**
 * Test: Shake reduction with strong smoothing
 * Tests that increasing smoothing radius improves shake reduction
 */
TEST_F(VisualStabilizationTest, StrongSmoothingReducesMoreShake) {
    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    // Test with light smoothing
    StabilizerCore::StabilizerParams light_params = getDefaultParams();
    light_params.smoothing_radius = Processing::SMALL_SMOOTHING_WINDOW;
    auto [before_shake, light_after_shake] = calculate_shake_reduction(frames, light_params);

    // Test with strong smoothing
    StabilizerCore::StabilizerParams strong_params = getDefaultParams();
    strong_params.smoothing_radius = Processing::LARGE_SMOOTHING_WINDOW;
    stabilizer->reset();
    auto [_, strong_after_shake] = calculate_shake_reduction(frames, strong_params);

    // Strong smoothing should reduce shake more than light smoothing
    EXPECT_LT(strong_after_shake, light_after_shake)
        << "Strong smoothing should reduce shake more than light smoothing";
}

// ============================================================================
// Stability Metrics Tests
// ============================================================================

/**
 * Test: Shake variance reduction
 * Measures not just average shake, but variance (consistency of stabilization)
 */
TEST_F(VisualStabilizationTest, ShakeVarianceReduction) {
    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    // Calculate shake magnitudes for original sequence
    std::vector<double> before_shakes;
    for (size_t i = 1; i < frames.size(); i++) {
        cv::Mat gray_prev, gray_curr;
        cv::cvtColor(frames[i-1], gray_prev, cv::COLOR_BGRA2GRAY);
        cv::cvtColor(frames[i], gray_curr, cv::COLOR_BGRA2GRAY);

        auto motion_vectors = calculate_motion_vectors(gray_prev, gray_curr);
        double shake = calculate_shake_magnitude(motion_vectors);
        before_shakes.push_back(shake);
    }

    // Stabilize frames
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(frames[0].cols, frames[0].rows, params));

    std::vector<cv::Mat> stabilized_frames;
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        if (!result.empty()) {
            stabilized_frames.push_back(result);
        }
    }

    // Calculate shake magnitudes for stabilized sequence
    std::vector<double> after_shakes;
    for (size_t i = 1; i < stabilized_frames.size(); i++) {
        cv::Mat gray_prev, gray_curr;
        cv::cvtColor(stabilized_frames[i-1], gray_prev, cv::COLOR_BGRA2GRAY);
        cv::cvtColor(stabilized_frames[i], gray_curr, cv::COLOR_BGRA2GRAY);

        auto motion_vectors = calculate_motion_vectors(gray_prev, gray_curr);
        double shake = calculate_shake_magnitude(motion_vectors);
        after_shakes.push_back(shake);
    }

    // Calculate variances
    double before_variance = calculate_shake_variance(before_shakes);
    double after_variance = calculate_shake_variance(after_shakes);

    // Variance should be reduced (more consistent motion)
    EXPECT_LT(after_variance, before_variance)
        << "Stabilized video should have more consistent (lower variance) shake";

    // Variance reduction should be significant
    double variance_reduction = (before_variance - after_variance) / std::max(before_variance, 0.001);
    EXPECT_GT(variance_reduction, 0.30)
        << "Variance should be reduced by >30%, got: " << (variance_reduction * 100.0) << "%";
}

/**
 * Test: Edge movement reduction
 * Measures how much edge movement is reduced (alternative metric)
 */
TEST_F(VisualStabilizationTest, EdgeMovementReduction) {
    auto frames = TestDataGenerator::generate_test_sequence(
        30, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    // Calculate edge movement for original sequence
    std::vector<double> before_edge_movement;
    for (size_t i = 1; i < frames.size(); i++) {
        double movement = calculate_edge_movement(frames[i-1], frames[i]);
        before_edge_movement.push_back(movement);
    }

    // Stabilize frames
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(frames[0].cols, frames[0].rows, params));

    std::vector<cv::Mat> stabilized_frames;
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        if (!result.empty()) {
            stabilized_frames.push_back(result);
        }
    }

    // Calculate edge movement for stabilized sequence
    std::vector<double> after_edge_movement;
    for (size_t i = 1; i < stabilized_frames.size(); i++) {
        double movement = calculate_edge_movement(stabilized_frames[i-1], stabilized_frames[i]);
        after_edge_movement.push_back(movement);
    }

    // Calculate averages
    double avg_before = 0.0, avg_after = 0.0;
    if (!before_edge_movement.empty()) {
        avg_before = std::accumulate(before_edge_movement.begin(), before_edge_movement.end(), 0.0)
            / before_edge_movement.size();
    }
    if (!after_edge_movement.empty()) {
        avg_after = std::accumulate(after_edge_movement.begin(), after_edge_movement.end(), 0.0)
            / after_edge_movement.size();
    }

    // Edge movement should be reduced
    EXPECT_LT(avg_after, avg_before)
        << "Stabilized video should have less edge movement";

    // Reduction should be at least 30%
    double reduction = (avg_before - avg_after) / std::max(avg_before, 0.001);
    EXPECT_GT(reduction, 0.30) << "Edge movement should be reduced by >30%, got: "
        << (reduction * 100.0) << "%";
}

// ============================================================================
// Different Motion Type Tests
// ============================================================================

/**
 * Test: Pan motion should not be over-stabilized
 * Stabilizer should reduce shake but preserve intentional pan motion
 */
TEST_F(VisualStabilizationTest, PanMotionPreserved) {
    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "pan_right"
    );

    StabilizerCore::StabilizerParams params = getDefaultParams();
    auto [before_shake, after_shake] = calculate_shake_reduction(frames, params);

    // For pan motion, we expect some reduction but not as much as for shake
    // The stabilizer should smooth out jitter but preserve the overall pan
    double reduction = (before_shake - after_shake) / std::max(before_shake, 0.001);

    // Pan motion should not be eliminated (reduction should be <80%)
    EXPECT_LT(reduction, 0.80)
        << "Pan motion should be preserved (not over-stabilized), got reduction: "
        << (reduction * 100.0) << "%";

    // But shake should still be reduced somewhat
    EXPECT_GT(reduction, 0.20)
        << "Pan jitter should still be reduced by >20%, got: " << (reduction * 100.0) << "%";
}

/**
 * Test: Static scene should remain stable
 * When there's no motion, stabilizer should not introduce artificial motion
 */
TEST_F(VisualStabilizationTest, StaticSceneRemainsStable) {
    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    StabilizerCore::StabilizerParams params = getDefaultParams();
    auto [before_shake, after_shake] = calculate_shake_reduction(frames, params);

    // Static scene should have minimal shake
    EXPECT_LT(before_shake, 1.0) << "Static scene should have minimal natural shake";

    // Stabilizer should not introduce additional shake
    EXPECT_LT(after_shake, before_shake * 1.5)
        << "Stabilizer should not introduce artificial shake in static scenes";
}

/**
 * Test: Zoom motion handling
 * Stabilizer should handle zoom motion gracefully
 */
TEST_F(VisualStabilizationTest, ZoomMotionHandling) {
    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "zoom_in"
    );

    StabilizerCore::StabilizerParams params = getDefaultParams();
    auto [before_shake, after_shake] = calculate_shake_reduction(frames, params);

    // Stabilizer should handle zoom without excessive artifacts
    // Shake should not increase after stabilization
    EXPECT_LE(after_shake, before_shake * 1.2)
        << "Zoom motion should not cause excessive shake after stabilization";
}

// ============================================================================
// Performance-Quality Tradeoff Tests
// ============================================================================

/**
 * Test: Higher feature count improves shake reduction
 * More features should lead to better tracking and better stabilization
 */
TEST_F(VisualStabilizationTest, MoreFeaturesImprovesQuality) {
    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    // Test with low feature count
    StabilizerCore::StabilizerParams low_params = getDefaultParams();
    low_params.feature_count = Features::LOW_COUNT;
    auto [_, low_after_shake] = calculate_shake_reduction(frames, low_params);

    // Test with high feature count
    StabilizerCore::StabilizerParams high_params = getDefaultParams();
    high_params.feature_count = Features::HIGH_COUNT;
    stabilizer->reset();
    auto [__, high_after_shake] = calculate_shake_reduction(frames, high_params);

    // More features should reduce shake better (or at least not worse)
    EXPECT_LE(high_after_shake, low_after_shake)
        << "High feature count should improve or maintain shake reduction quality";
}

/**
 * Test: Adaptive mode quality
 * Tests that adaptive stabilization maintains good quality across motion types
 */
TEST_F(VisualStabilizationTest, AdaptiveModeQuality) {
    // Create mixed motion sequence
    std::vector<cv::Mat> mixed_frames;

    // Static frames
    auto static_frames = TestDataGenerator::generate_test_sequence(
        10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );
    mixed_frames.insert(mixed_frames.end(), static_frames.begin(), static_frames.end());

    // Shake frames
    auto shake_frames = TestDataGenerator::generate_test_sequence(
        15, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );
    mixed_frames.insert(mixed_frames.end(), shake_frames.begin(), shake_frames.end());

    // Pan frames
    auto pan_frames = TestDataGenerator::generate_test_sequence(
        15, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "pan_right"
    );
    mixed_frames.insert(mixed_frames.end(), pan_frames.begin(), pan_frames.end());

    // Test with adaptive stabilization enabled
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.adaptive_enabled = true;
    auto [before_shake, after_shake] = calculate_shake_reduction(mixed_frames, params);

    // Adaptive mode should still provide shake reduction
    double reduction = (before_shake - after_shake) / std::max(before_shake, 0.001);
    EXPECT_GT(reduction, 0.30)
        << "Adaptive mode should reduce shake by >30%, got: " << (reduction * 100.0) << "%";
}

// ============================================================================
// Integration with Realistic Scenarios
// ============================================================================

/**
 * Test: Gaming scenario shake reduction
 * Tests stabilization quality with gaming preset on fast motion
 */
TEST_F(VisualStabilizationTest, GamingScenarioShakeReduction) {
    StabilizerCore::StabilizerParams params = StabilizerCore::get_preset_gaming();

    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "fast"
    );

    auto [before_shake, after_shake] = calculate_shake_reduction(frames, params);

    // Gaming preset should still reduce shake even with fast motion
    double reduction = (before_shake - after_shake) / std::max(before_shake, 0.001);
    EXPECT_GT(reduction, 0.30)
        << "Gaming preset should reduce shake by >30%, got: " << (reduction * 100.0) << "%";
}

/**
 * Test: Streaming scenario shake reduction
 * Tests stabilization quality with streaming preset on HD content
 */
TEST_F(VisualStabilizationTest, StreamingScenarioShakeReduction) {
    StabilizerCore::StabilizerParams params = StabilizerCore::get_preset_streaming();

    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
    );

    auto [before_shake, after_shake] = calculate_shake_reduction(frames, params);

    // Streaming preset should provide good shake reduction at HD resolution
    double reduction = (before_shake - after_shake) / std::max(before_shake, 0.001);
    EXPECT_GT(reduction, 0.40)
        << "Streaming preset should reduce shake by >40% at HD, got: "
        << (reduction * 100.0) << "%";
}
