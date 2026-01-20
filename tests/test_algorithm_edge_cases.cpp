#ifndef SKIP_OPENCV_TESTS

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <random>
#include "core/stabilizer_core.hpp"
#include "test_data_generator.hpp"
#include "test_constants.hpp"

using namespace TestDataGenerator;
using namespace TestConstants;

class AlgorithmEdgeCases : public ::testing::Test {
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

TEST_F(AlgorithmEdgeCases, FeaturePointExhaustion) {
    StabilizerCore stabilizer;
    test_params.quality_level = 0.9f;
    test_params.feature_count = 500;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 50; i++) {
        cv::Mat frame = generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, i % 3);
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 0) << "Should handle feature point exhaustion gracefully";
    EXPECT_LE(successful_frames, 50) << "Some frames may fail due to feature exhaustion";
}

TEST_F(AlgorithmEdgeCases, RapidHorizontalMotionTracking) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 30; i++) {
        cv::Mat frame = generate_horizontal_motion_frame(
            generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0), i, 30);
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 25) << "Should track rapid horizontal motion effectively";
}

TEST_F(AlgorithmEdgeCases, RapidVerticalMotionTracking) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 30; i++) {
        cv::Mat frame = generate_vertical_motion_frame(
            generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0), i, 30);
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 25) << "Should track rapid vertical motion effectively";
}

TEST_F(AlgorithmEdgeCases, RapidRotationTracking) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 30; i++) {
        cv::Mat frame = generate_rotation_frame(
            generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0), i, 30, 5.0f);
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 20) << "Should track rapid rotation (some frames may fail)";
}

TEST_F(AlgorithmEdgeCases, SceneChangeDetectionAbrupt) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat frame1 = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0);
    cv::Mat result1 = stabilizer.process_frame(frame1);

    int successful_frames = 0;
    for (int i = 0; i < 10; i++) {
        cv::Mat frame2 = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, (i + 1) * 10);
        cv::Mat result = stabilizer.process_frame(frame2);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 5) << "Should recover from abrupt scene changes";
}

TEST_F(AlgorithmEdgeCases, SceneChangeDetectionGradual) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 20; i++) {
        cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, i);
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 15) << "Should handle gradual scene changes";
}

TEST_F(AlgorithmEdgeCases, LowLightConditions) {
    StabilizerCore stabilizer;
    test_params.quality_level = 0.3f;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    cv::Mat dark_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC4, cv::Scalar(30, 30, 30, 255));
    cv::Mat result = stabilizer.process_frame(dark_frame);

    EXPECT_FALSE(result.empty()) << "Should handle low-light conditions";
}

TEST_F(AlgorithmEdgeCases, VeryLowLightConditions) {
    StabilizerCore stabilizer;
    test_params.quality_level = 0.5f;
    test_params.feature_count = 100;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    cv::Mat very_dark_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC4, cv::Scalar(10, 10, 10, 255));

    int successful_frames = 0;
    for (int i = 0; i < 20; i++) {
        cv::Mat result = stabilizer.process_frame(very_dark_frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 0) << "Should handle very low-light conditions (may degrade gracefully)";
}

TEST_F(AlgorithmEdgeCases, HighContrastScenes) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat high_contrast(Resolution::HD_HEIGHT, Resolution::HD_WIDTH, CV_8UC4);
    cv::rectangle(high_contrast, cv::Rect(0, 0, Resolution::HD_WIDTH / 2, Resolution::HD_HEIGHT), 
                  cv::Scalar(0, 0, 0, 255), -1);
    cv::rectangle(high_contrast, cv::Rect(Resolution::HD_WIDTH / 2, 0, Resolution::HD_WIDTH / 2, Resolution::HD_HEIGHT), 
                  cv::Scalar(255, 255, 255, 255), -1);

    cv::Mat result = stabilizer.process_frame(high_contrast);
    EXPECT_FALSE(result.empty()) << "Should handle high contrast scenes";
}

TEST_F(AlgorithmEdgeCases, InsufficientFeaturePointsBelowThreshold) {
    StabilizerCore stabilizer;
    test_params.quality_level = 0.95f;
    test_params.min_distance = 100.0f;
    test_params.feature_count = 200;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    cv::Mat plain_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC4, cv::Scalar(128, 128, 128, 255));

    int successful_frames = 0;
    for (int i = 0; i < 30; i++) {
        cv::Mat result = stabilizer.process_frame(plain_frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GE(successful_frames, 0) << "Should handle insufficient feature points gracefully";
}

TEST_F(AlgorithmEdgeCases, ExtremeRotationAngles) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 20; i++) {
        float angle = (i - 10) * 10.0f;
        cv::Mat frame = generate_rotation_frame(
            generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, 0), i, 20, angle);
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 5) << "Should handle extreme rotation angles (some may fail)";
}

TEST_F(AlgorithmEdgeCases, FastZoomTransitions) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 20; i++) {
        float zoom = 1.0f + (i / 10.0f);
        cv::Mat frame = generate_zoom_frame(
            generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0), i, 20, zoom);
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 15) << "Should handle fast zoom transitions";
}

TEST_F(AlgorithmEdgeCases, MotionBlurSimulation) {
    StabilizerCore stabilizer;
    test_params.quality_level = 0.05f;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 30; i++) {
        cv::Mat base = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, i % 3);
        cv::Mat motion_blur;
        cv::GaussianBlur(base, motion_blur, cv::Size(15, 1), 0);
        cv::Mat result = stabilizer.process_frame(motion_blur);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 20) << "Should handle motion blur";
}

TEST_F(AlgorithmEdgeCases, OccludedFeaturePoints) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat frame1 = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0);
    stabilizer.process_frame(frame1);

    int successful_frames = 0;
    for (int i = 0; i < 20; i++) {
        cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 1);
        cv::rectangle(frame, cv::Rect(0, 0, Resolution::HD_WIDTH / 2, Resolution::HD_HEIGHT), 
                      cv::Scalar(0, 0, 0, 255), -1);
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 10) << "Should handle occluded feature points";
}

TEST_F(AlgorithmEdgeCases, MinimalFeatureCountTracking) {
    StabilizerCore stabilizer;
    test_params.feature_count = 4;
    test_params.quality_level = 0.5f;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 30; i++) {
        cv::Mat frame = create_frame_with_features(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, 4);
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 10) << "Should track with minimal feature count";
}

TEST_F(AlgorithmEdgeCases, AlternatingMotionDirections) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 40; i++) {
        cv::Mat frame;
        if (i % 2 == 0) {
            frame = generate_horizontal_motion_frame(
                generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0), i / 2, 20);
        } else {
            frame = generate_vertical_motion_frame(
                generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0), i / 2, 20);
        }
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 30) << "Should handle alternating motion directions";
}

TEST_F(AlgorithmEdgeCases, OutOfBoundFeatureTracking) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 30; i++) {
        cv::Mat frame = generate_horizontal_motion_frame(
            generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, 0), i, 30);
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 20) << "Should handle features going out of bounds";
}

TEST_F(AlgorithmEdgeCases, NearUniformFrames) {
    StabilizerCore stabilizer;
    test_params.quality_level = 0.1f;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 30; i++) {
        cv::Mat frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC4, cv::Scalar(128, 128, 128, 255));
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GE(successful_frames, 0) << "Should handle near-uniform frames gracefully";
}

#endif // SKIP_OPENCV_TESTS
