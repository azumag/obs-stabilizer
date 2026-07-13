#include <gtest/gtest.h>

#include "core/kalman_transform_filter.hpp"

TEST(KalmanTransformFilterTest, FirstMeasurementInitializesState) {
    KalmanTransformFilter filter;
    const cv::Vec4f measurement(10.0f, -4.0f, 0.2f, 1.1f);

    const cv::Vec4f result = filter.update(measurement);

    EXPECT_TRUE(filter.is_initialized());
    for (int i = 0; i < 4; ++i) {
        EXPECT_FLOAT_EQ(result[i], measurement[i]);
    }
}

TEST(KalmanTransformFilterTest, SmoothsNoisyConstantMotion) {
    KalmanTransformFilter filter;
    filter.update({0.0f, 0.0f, 0.0f, 1.0f});

    cv::Vec4f result;
    for (int i = 1; i <= 20; ++i) {
        const float noise = (i % 2 == 0) ? 1.5f : -1.5f;
        result = filter.update({static_cast<float>(i) + noise, 0.0f, 0.0f, 1.0f});
    }

    EXPECT_NEAR(result[0], 20.0f, 2.0f);
    EXPECT_NEAR(result[1], 0.0f, 0.1f);
    EXPECT_NEAR(result[2], 0.0f, 0.1f);
    EXPECT_NEAR(result[3], 1.0f, 0.1f);
}

TEST(KalmanTransformFilterTest, ResetClearsInitialization) {
    KalmanTransformFilter filter;
    filter.update({1.0f, 2.0f, 0.1f, 1.0f});

    filter.reset();

    EXPECT_FALSE(filter.is_initialized());
    const cv::Vec4f prediction = filter.predict();
    EXPECT_FLOAT_EQ(prediction[0], 0.0f);
    EXPECT_FLOAT_EQ(prediction[1], 0.0f);
    EXPECT_FLOAT_EQ(prediction[2], 0.0f);
    EXPECT_FLOAT_EQ(prediction[3], 1.0f);
}

TEST(KalmanTransformFilterTest, RejectsNonPositiveNoise) {
    KalmanTransformFilter::NoiseConfig config;
    config.process_noise = 0.0f;
    EXPECT_THROW(KalmanTransformFilter filter(config), std::invalid_argument);
}
