#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <random>

#include <opencv2/core.hpp>

#include "core/frame_analyzer.hpp"
#include "core/parameter_validation.hpp"
#include "core/stabilizer_constants.hpp"

namespace {

using Params = StabilizerCore::StabilizerParams;

TEST(FuzzRegression, RandomizedParametersAreClampedToStableContracts)
{
    std::mt19937 rng(0x308u);
    std::uniform_int_distribution<int> integer_dist(-10000, 10000);
    std::uniform_real_distribution<float> float_dist(-10000.0f, 10000.0f);

    for (int iteration = 0; iteration < 512; ++iteration) {
        Params input;
        input.smoothing_radius = integer_dist(rng);
        input.max_correction = float_dist(rng);
        input.feature_count = integer_dist(rng);
        input.quality_level = float_dist(rng);
        input.min_distance = float_dist(rng);
        input.block_size = integer_dist(rng);
        input.k = float_dist(rng);
        input.tracking_error_threshold = static_cast<double>(float_dist(rng));
        input.ransac_threshold_min = float_dist(rng);
        input.ransac_threshold_max = float_dist(rng);
        input.min_point_spread = float_dist(rng);

        const Params validated = VALIDATION::validate_parameters(input);
        const Params validated_twice = VALIDATION::validate_parameters(validated);

        EXPECT_GE(validated.smoothing_radius, StabilizerConstants::Smoothing::MIN_RADIUS);
        EXPECT_LE(validated.smoothing_radius, StabilizerConstants::Smoothing::MAX_RADIUS);
        EXPECT_GE(validated.max_correction, StabilizerConstants::Correction::MIN_MAX);
        EXPECT_LE(validated.max_correction, StabilizerConstants::Correction::MAX_MAX);
        EXPECT_GE(validated.feature_count, StabilizerConstants::Features::MIN_COUNT);
        EXPECT_LE(validated.feature_count, StabilizerConstants::Features::MAX_COUNT);
        EXPECT_GE(validated.quality_level, StabilizerConstants::Quality::MIN_LEVEL);
        EXPECT_LE(validated.quality_level, StabilizerConstants::Quality::MAX_LEVEL);
        EXPECT_GE(validated.min_distance, StabilizerConstants::Distance::MIN);
        EXPECT_LE(validated.min_distance, StabilizerConstants::Distance::MAX);
        EXPECT_GE(validated.block_size, StabilizerConstants::Block::MIN_SIZE);
        EXPECT_LE(validated.block_size, StabilizerConstants::Block::MAX_SIZE);
        EXPECT_EQ(validated.block_size % 2, 1);
        EXPECT_GE(validated.k, StabilizerConstants::Harris::MIN_K);
        EXPECT_LE(validated.k, StabilizerConstants::Harris::MAX_K);
        EXPECT_GE(validated.ransac_threshold_min, 0.1f);
        EXPECT_LE(validated.ransac_threshold_max, 100.0f);
        EXPECT_LE(validated.ransac_threshold_min, validated.ransac_threshold_max);
        EXPECT_TRUE(std::isfinite(validated.tracking_error_threshold));
        EXPECT_TRUE(std::isfinite(validated.min_point_spread));

        // Validation is intentionally idempotent: once clamped, feeding the
        // same configuration back through the boundary must not drift.
        EXPECT_EQ(validated.smoothing_radius, validated_twice.smoothing_radius);
        EXPECT_FLOAT_EQ(validated.max_correction, validated_twice.max_correction);
        EXPECT_EQ(validated.feature_count, validated_twice.feature_count);
        EXPECT_FLOAT_EQ(validated.quality_level, validated_twice.quality_level);
        EXPECT_FLOAT_EQ(validated.min_distance, validated_twice.min_distance);
        EXPECT_EQ(validated.block_size, validated_twice.block_size);
        EXPECT_FLOAT_EQ(validated.k, validated_twice.k);
        EXPECT_DOUBLE_EQ(validated.tracking_error_threshold,
                         validated_twice.tracking_error_threshold);
        EXPECT_FLOAT_EQ(validated.ransac_threshold_min,
                        validated_twice.ransac_threshold_min);
        EXPECT_FLOAT_EQ(validated.ransac_threshold_max,
                        validated_twice.ransac_threshold_max);
        EXPECT_FLOAT_EQ(validated.min_point_spread, validated_twice.min_point_spread);
    }
}

TEST(FuzzRegression, RandomizedSupportedFramesStayWithinAnalyzerBounds)
{
    std::mt19937 rng(0xF00D308u);
    std::uniform_int_distribution<int> size_dist(32, 192);
    std::uniform_int_distribution<int> channels_dist(0, 2);
    const int supported_channels[] = {1, 3, 4};
    cv::RNG pixel_rng(0x308F00Du);

    for (int iteration = 0; iteration < 256; ++iteration) {
        const int width = size_dist(rng);
        const int height = size_dist(rng);
        const int channels = supported_channels[channels_dist(rng)];
        cv::Mat frame(height, width, CV_MAKETYPE(CV_8U, channels));
        pixel_rng.fill(frame, cv::RNG::UNIFORM, 0, 256);

        ASSERT_TRUE(FrameAnalyzer::is_valid_frame(frame));

        cv::Rect bounds;
        ASSERT_NO_THROW(bounds = FrameAnalyzer::detect_content_bounds(frame));
        EXPECT_GE(bounds.x, 0);
        EXPECT_GE(bounds.y, 0);
        EXPECT_GT(bounds.width, 0);
        EXPECT_GT(bounds.height, 0);
        EXPECT_LE(bounds.x + bounds.width, frame.cols);
        EXPECT_LE(bounds.y + bounds.height, frame.rows);
    }
}

TEST(FuzzRegression, FeaturePointValidationRejectsNonFiniteAndOutOfBoundsInputs)
{
    constexpr int width = 1920;
    constexpr int height = 1080;

    EXPECT_TRUE(VALIDATION::is_valid_feature_point({0.0f, 0.0f}, width, height));
    EXPECT_TRUE(VALIDATION::is_valid_feature_point(
        {static_cast<float>(width - 1), static_cast<float>(height - 1)}, width, height));
    EXPECT_FALSE(VALIDATION::is_valid_feature_point({-1.0f, 10.0f}, width, height));
    EXPECT_FALSE(VALIDATION::is_valid_feature_point(
        {static_cast<float>(width), 10.0f}, width, height));
    EXPECT_FALSE(VALIDATION::is_valid_feature_point(
        {std::numeric_limits<float>::quiet_NaN(), 10.0f}, width, height));
    EXPECT_FALSE(VALIDATION::is_valid_feature_point(
        {10.0f, std::numeric_limits<float>::infinity()}, width, height));
}

} // namespace
