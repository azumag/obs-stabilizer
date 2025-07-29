/*
Transform smoothing and stabilization tests
Tests the transformation matrix smoothing algorithms and stabilization logic
*/

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

class TransformSmoothingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test data
        identity_transform = cv::Mat::eye(2, 3, CV_64F);

        // Create sample transforms representing camera shake
        transforms.clear();

        // Simulate shaky camera motion
        for (int i = 0; i < 10; i++) {
            cv::Mat transform = cv::Mat::eye(2, 3, CV_64F);
            // Add small random translations (simulating shake)
            double tx = std::sin(i * 0.5) * 3.0 + (rand() % 100 - 50) / 50.0;
            double ty = std::cos(i * 0.3) * 2.0 + (rand() % 100 - 50) / 60.0;

            transform.at<double>(0, 2) = tx;
            transform.at<double>(1, 2) = ty;

            transforms.push_back(transform.clone());
        }
    }

    cv::Mat identity_transform;
    std::vector<cv::Mat> transforms;

    // Helper to calculate moving average of transforms
    cv::Mat calculateMovingAverage(const std::vector<cv::Mat>& transform_history) {
        if (transform_history.empty()) {
            return cv::Mat::eye(2, 3, CV_64F);
        }

        cv::Mat smoothed = cv::Mat::zeros(2, 3, CV_64F);
        for (const auto& transform : transform_history) {
            smoothed += transform;
        }
        smoothed /= static_cast<double>(transform_history.size());
        return smoothed;
    }

    // Helper to calculate variance of translations
    double calculateTranslationVariance(const std::vector<cv::Mat>& transform_list) {
        if (transform_list.size() < 2) return 0.0;

        // Calculate mean translation
        double mean_tx = 0, mean_ty = 0;
        for (const auto& t : transform_list) {
            mean_tx += t.at<double>(0, 2);
            mean_ty += t.at<double>(1, 2);
        }
        mean_tx /= transform_list.size();
        mean_ty /= transform_list.size();

        // Calculate variance
        double variance = 0;
        for (const auto& t : transform_list) {
            double dx = t.at<double>(0, 2) - mean_tx;
            double dy = t.at<double>(1, 2) - mean_ty;
            variance += dx * dx + dy * dy;
        }
        return variance / transform_list.size();
    }
};

TEST_F(TransformSmoothingTest, IdentityTransformTest) {
    // Test identity transform properties
    EXPECT_EQ(identity_transform.rows, 2);
    EXPECT_EQ(identity_transform.cols, 3);

    // Check identity matrix values
    EXPECT_DOUBLE_EQ(identity_transform.at<double>(0, 0), 1.0);
    EXPECT_DOUBLE_EQ(identity_transform.at<double>(0, 1), 0.0);
    EXPECT_DOUBLE_EQ(identity_transform.at<double>(0, 2), 0.0);
    EXPECT_DOUBLE_EQ(identity_transform.at<double>(1, 0), 0.0);
    EXPECT_DOUBLE_EQ(identity_transform.at<double>(1, 1), 1.0);
    EXPECT_DOUBLE_EQ(identity_transform.at<double>(1, 2), 0.0);
}

TEST_F(TransformSmoothingTest, MovingAverageCalculation) {
    // Test with single transform
    std::vector<cv::Mat> single_transform = {transforms[0]};
    cv::Mat result = calculateMovingAverage(single_transform);

    EXPECT_NEAR(result.at<double>(0, 2), transforms[0].at<double>(0, 2), 1e-10);
    EXPECT_NEAR(result.at<double>(1, 2), transforms[0].at<double>(1, 2), 1e-10);

    // Test with multiple transforms
    std::vector<cv::Mat> three_transforms = {transforms[0], transforms[1], transforms[2]};
    cv::Mat average = calculateMovingAverage(three_transforms);

    double expected_tx = (transforms[0].at<double>(0, 2) +
                         transforms[1].at<double>(0, 2) +
                         transforms[2].at<double>(0, 2)) / 3.0;
    double expected_ty = (transforms[0].at<double>(1, 2) +
                         transforms[1].at<double>(1, 2) +
                         transforms[2].at<double>(1, 2)) / 3.0;

    EXPECT_NEAR(average.at<double>(0, 2), expected_tx, 1e-10);
    EXPECT_NEAR(average.at<double>(1, 2), expected_ty, 1e-10);
}

TEST_F(TransformSmoothingTest, SmoothingReducesVariance) {
    // Calculate variance of original transforms
    double original_variance = calculateTranslationVariance(transforms);

    // Apply smoothing with different window sizes
    std::vector<int> window_sizes = {3, 5, 7};

    for (int window_size : window_sizes) {
        std::vector<cv::Mat> smoothed_transforms;

        for (size_t i = 0; i < transforms.size(); i++) {
            // Create window of transforms
            std::vector<cv::Mat> window;
            int start = std::max(0, static_cast<int>(i) - window_size/2);
            int end = std::min(static_cast<int>(transforms.size()),
                              static_cast<int>(i) + window_size/2 + 1);

            for (int j = start; j < end; j++) {
                window.push_back(transforms[j]);
            }

            cv::Mat smoothed = calculateMovingAverage(window);
            smoothed_transforms.push_back(smoothed);
        }

        double smoothed_variance = calculateTranslationVariance(smoothed_transforms);

        EXPECT_LT(smoothed_variance, original_variance)
            << "Smoothing should reduce variance for window size " << window_size;
    }
}

TEST_F(TransformSmoothingTest, HistoryManagement) {
    const int max_history = 5;
    std::vector<cv::Mat> history;

    // Add transforms beyond maximum
    for (int i = 0; i < 10; i++) {
        history.push_back(transforms[i % transforms.size()].clone());

        // Maintain history size limit
        if (history.size() > static_cast<size_t>(max_history)) {
            history.erase(history.begin());
        }

        EXPECT_LE(history.size(), static_cast<size_t>(max_history))
            << "History size should not exceed limit";
    }

    EXPECT_EQ(history.size(), static_cast<size_t>(max_history))
        << "History should be at maximum size";
}

TEST_F(TransformSmoothingTest, EmptyHistoryHandling) {
    std::vector<cv::Mat> empty_history;
    cv::Mat result = calculateMovingAverage(empty_history);

    // Should return identity transform for empty history
    EXPECT_DOUBLE_EQ(result.at<double>(0, 0), 1.0);
    EXPECT_DOUBLE_EQ(result.at<double>(0, 1), 0.0);
    EXPECT_DOUBLE_EQ(result.at<double>(0, 2), 0.0);
    EXPECT_DOUBLE_EQ(result.at<double>(1, 0), 0.0);
    EXPECT_DOUBLE_EQ(result.at<double>(1, 1), 1.0);
    EXPECT_DOUBLE_EQ(result.at<double>(1, 2), 0.0);
}

TEST_F(TransformSmoothingTest, TransformComposition) {
    // Test accumulation of transforms
    cv::Mat accumulated = cv::Mat::eye(2, 3, CV_64F);

    // Apply several transforms
    for (int i = 0; i < 3; i++) {
        accumulated = transforms[i] * accumulated;
    }

    EXPECT_EQ(accumulated.rows, 2);
    EXPECT_EQ(accumulated.cols, 3);

    // Accumulated transform should be different from identity
    bool is_different = false;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            if (std::abs(accumulated.at<double>(i, j) - identity_transform.at<double>(i, j)) > 1e-10) {
                is_different = true;
                break;
            }
        }
    }

    EXPECT_TRUE(is_different) << "Accumulated transform should differ from identity";
}

TEST_F(TransformSmoothingTest, SmoothingParameters) {
    // Test different smoothing radii
    std::vector<int> smoothing_radii = {10, 30, 50};

    for (int radius : smoothing_radii) {
        // Simulate the smoothing process
        std::vector<cv::Mat> transform_history;
        std::vector<cv::Mat> smoothed_results;

        for (size_t i = 0; i < transforms.size(); i++) {
            transform_history.push_back(transforms[i].clone());

            // Limit history size based on radius
            if (transform_history.size() > static_cast<size_t>(radius)) {
                transform_history.erase(transform_history.begin());
            }

            cv::Mat smoothed = calculateMovingAverage(transform_history);
            smoothed_results.push_back(smoothed);
        }

        // Larger radius should provide more smoothing
        double variance = calculateTranslationVariance(smoothed_results);

        // Should be a reasonable value
        EXPECT_GE(variance, 0.0) << "Variance should be non-negative for radius " << radius;
        EXPECT_LT(variance, 100.0) << "Variance should be reasonable for radius " << radius;
    }
}

TEST_F(TransformSmoothingTest, TransformValidation) {
    // Test validation of transform matrices
    for (const auto& transform : transforms) {
        EXPECT_EQ(transform.rows, 2) << "Transform should have 2 rows";
        EXPECT_EQ(transform.cols, 3) << "Transform should have 3 columns";
        EXPECT_EQ(transform.type(), CV_64F) << "Transform should be double precision";

        // Check that it's a proper affine transform matrix structure
        // [a c tx]
        // [b d ty]
        // Should have reasonable values
        double a = transform.at<double>(0, 0);
        double b = transform.at<double>(1, 0);
        double c = transform.at<double>(0, 1);
        double d = transform.at<double>(1, 1);

        // For our test data, these should be close to identity
        EXPECT_NEAR(a, 1.0, 0.1) << "Scale/rotation component should be reasonable";
        EXPECT_NEAR(d, 1.0, 0.1) << "Scale/rotation component should be reasonable";
        EXPECT_NEAR(b, 0.0, 0.1) << "Shear component should be small";
        EXPECT_NEAR(c, 0.0, 0.1) << "Shear component should be small";
    }
}

TEST_F(TransformSmoothingTest, NumericalStability) {
    // Test numerical stability with extreme values
    cv::Mat extreme_transform = cv::Mat::eye(2, 3, CV_64F);
    extreme_transform.at<double>(0, 2) = 1000.0; // Large translation
    extreme_transform.at<double>(1, 2) = -1000.0;

    std::vector<cv::Mat> extreme_history = {extreme_transform, identity_transform};
    cv::Mat result = calculateMovingAverage(extreme_history);

    // Result should be finite and reasonable
    EXPECT_TRUE(std::isfinite(result.at<double>(0, 2))) << "Translation should be finite";
    EXPECT_TRUE(std::isfinite(result.at<double>(1, 2))) << "Translation should be finite";

    // Should be average of extreme and identity
    EXPECT_NEAR(result.at<double>(0, 2), 500.0, 1e-10);
    EXPECT_NEAR(result.at<double>(1, 2), -500.0, 1e-10);
}

TEST_F(TransformSmoothingTest, SmoothingEffectiveness) {
    // Compare smoothed vs unsmoothed motion
    std::vector<cv::Mat> smoothed_transforms;
    std::vector<cv::Mat> history;
    const int smoothing_window = 5;

    for (size_t i = 0; i < transforms.size(); i++) {
        history.push_back(transforms[i].clone());

        if (history.size() > smoothing_window) {
            history.erase(history.begin());
        }

        cv::Mat smoothed = calculateMovingAverage(history);
        smoothed_transforms.push_back(smoothed);
    }

    // Calculate frame-to-frame differences (jerkiness)
    double original_jerkiness = 0.0, smoothed_jerkiness = 0.0;

    for (size_t i = 1; i < transforms.size(); i++) {
        // Original jerkiness
        double dx_orig = transforms[i].at<double>(0, 2) - transforms[i-1].at<double>(0, 2);
        double dy_orig = transforms[i].at<double>(1, 2) - transforms[i-1].at<double>(1, 2);
        original_jerkiness += sqrt(dx_orig * dx_orig + dy_orig * dy_orig);

        // Smoothed jerkiness
        double dx_smooth = smoothed_transforms[i].at<double>(0, 2) - smoothed_transforms[i-1].at<double>(0, 2);
        double dy_smooth = smoothed_transforms[i].at<double>(1, 2) - smoothed_transforms[i-1].at<double>(1, 2);
        smoothed_jerkiness += sqrt(dx_smooth * dx_smooth + dy_smooth * dy_smooth);
    }

    EXPECT_LT(smoothed_jerkiness, original_jerkiness)
        << "Smoothing should reduce frame-to-frame motion variation";
}