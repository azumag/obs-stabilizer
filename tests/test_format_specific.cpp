#ifndef SKIP_OPENCV_TESTS

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include "core/stabilizer_core.hpp"
#include "test_data_generator.hpp"
#include "test_constants.hpp"

using namespace TestDataGenerator;
using namespace TestConstants;

class FormatSpecificTests : public ::testing::Test {
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

TEST_F(FormatSpecificTests, NV12OddWidth) {
    StabilizerCore stabilizer;
    stabilizer.initialize(641, 480, test_params);

    cv::Mat bgra_frame = generate_test_frame(641, 480, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle NV12 with odd width";
}

TEST_F(FormatSpecificTests, NV12OddHeight) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 481, test_params);

    cv::Mat bgra_frame = generate_test_frame(640, 481, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle NV12 with odd height";
}

TEST_F(FormatSpecificTests, NV12BothOddDimensions) {
    StabilizerCore stabilizer;
    stabilizer.initialize(643, 483, test_params);

    cv::Mat bgra_frame = generate_test_frame(643, 483, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle NV12 with both odd dimensions";
}

TEST_F(FormatSpecificTests, NV12AlignmentBoundary16) {
    StabilizerCore stabilizer;
    stabilizer.initialize(1920, 1080, test_params);

    cv::Mat bgra_frame = generate_test_frame(1920, 1080, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle NV12 with 16-pixel alignment";
}

TEST_F(FormatSpecificTests, NV12AlignmentBoundary32) {
    StabilizerCore stabilizer;
    stabilizer.initialize(1280, 720, test_params);

    cv::Mat bgra_frame = generate_test_frame(1280, 720, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle NV12 with 32-pixel alignment";
}

TEST_F(FormatSpecificTests, NV12MisalignedDimensions) {
    StabilizerCore stabilizer;
    stabilizer.initialize(638, 478, test_params);

    cv::Mat bgra_frame = generate_test_frame(638, 478, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle NV12 with misaligned dimensions";
}

TEST_F(FormatSpecificTests, NV12SmallDimensions) {
    StabilizerCore stabilizer;
    stabilizer.initialize(64, 64, test_params);

    cv::Mat bgra_frame = generate_test_frame(64, 64, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle NV12 with small dimensions";
}

TEST_F(FormatSpecificTests, NV12VerySmallDimensions) {
    StabilizerCore stabilizer;
    stabilizer.initialize(32, 32, test_params);

    cv::Mat bgra_frame = generate_test_frame(32, 32, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle NV12 with very small dimensions";
}

TEST_F(FormatSpecificTests, I420OddWidth) {
    StabilizerCore stabilizer;
    stabilizer.initialize(641, 480, test_params);

    cv::Mat bgra_frame = generate_test_frame(641, 480, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle I420 with odd width";
}

TEST_F(FormatSpecificTests, I420OddHeight) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 481, test_params);

    cv::Mat bgra_frame = generate_test_frame(640, 481, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle I420 with odd height";
}

TEST_F(FormatSpecificTests, I420BothOddDimensions) {
    StabilizerCore stabilizer;
    stabilizer.initialize(643, 483, test_params);

    cv::Mat bgra_frame = generate_test_frame(643, 483, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle I420 with both odd dimensions";
}

TEST_F(FormatSpecificTests, I420AlignmentBoundary16) {
    StabilizerCore stabilizer;
    stabilizer.initialize(1920, 1080, test_params);

    cv::Mat bgra_frame = generate_test_frame(1920, 1080, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle I420 with 16-pixel alignment";
}

TEST_F(FormatSpecificTests, I420BoundaryYUVPlaneSeparation) {
    StabilizerCore stabilizer;
    stabilizer.initialize(640, 480, test_params);

    cv::Mat bgra_frame = generate_test_frame(640, 480, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle I420 Y/UV plane separation at boundaries";
}

TEST_F(FormatSpecificTests, I420SmallDimensions) {
    StabilizerCore stabilizer;
    stabilizer.initialize(64, 64, test_params);

    cv::Mat bgra_frame = generate_test_frame(64, 64, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle I420 with small dimensions";
}

TEST_F(FormatSpecificTests, BGRAFormat) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat bgra_frame(Resolution::HD_HEIGHT, Resolution::HD_WIDTH, CV_8UC4);
    cv::rectangle(bgra_frame, cv::Rect(100, 100, 500, 500), cv::Scalar(255, 0, 0, 255), -1);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle BGRA format";
    EXPECT_EQ(result.channels(), 4) << "Should maintain BGRA channels";
}

TEST_F(FormatSpecificTests, BGRFormat) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat bgr_frame(Resolution::HD_HEIGHT, Resolution::HD_WIDTH, CV_8UC3);
    cv::rectangle(bgr_frame, cv::Rect(100, 100, 500, 500), cv::Scalar(255, 0, 0), -1);
    cv::Mat result = stabilizer.process_frame(bgr_frame);

    EXPECT_FALSE(result.empty()) << "Should handle BGR format";
    EXPECT_EQ(result.channels(), 3) << "Should maintain BGR channels";
}

TEST_F(FormatSpecificTests, GrayscaleFormat) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat gray_frame(Resolution::HD_HEIGHT, Resolution::HD_WIDTH, CV_8UC1, cv::Scalar(128));
    cv::Mat result = stabilizer.process_frame(gray_frame);

    EXPECT_FALSE(result.empty()) << "Should handle grayscale format";
    EXPECT_EQ(result.channels(), 1) << "Should maintain grayscale channel";
}

TEST_F(FormatSpecificTests, CrossFormatConversionSequence) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    cv::Mat bgra_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC4, cv::Scalar(100, 100, 100, 255));
    cv::Mat result1 = stabilizer.process_frame(bgra_frame);
    EXPECT_FALSE(result1.empty());

    cv::Mat bgr_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC3, cv::Scalar(100, 100, 100));
    cv::Mat result2 = stabilizer.process_frame(bgr_frame);
    EXPECT_FALSE(result2.empty());

    cv::Mat gray_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC1, cv::Scalar(100));
    cv::Mat result3 = stabilizer.process_frame(gray_frame);
    EXPECT_FALSE(result3.empty());
}

TEST_F(FormatSpecificTests, CrossFormatStressTest) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, test_params);

    int successful_frames = 0;
    for (int i = 0; i < 60; i++) {
        cv::Mat frame;
        if (i % 3 == 0) {
            frame = generate_test_frame(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, i);
        } else if (i % 3 == 1) {
            frame = cv::Mat(Resolution::HD720_HEIGHT, Resolution::HD720_WIDTH, CV_8UC3, 
                           cv::Scalar(100, 100, 100));
        } else {
            frame = cv::Mat(Resolution::HD720_HEIGHT, Resolution::HD720_WIDTH, CV_8UC1, 
                           cv::Scalar(100));
        }
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
        }
    }

    EXPECT_GT(successful_frames, 50) << "Should handle cross-format processing";
}

TEST_F(FormatSpecificTests, FormatPreservationAfterProcessing) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    cv::Mat bgra_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC4);
    cv::rectangle(bgra_frame, cv::Rect(50, 50, 200, 200), cv::Scalar(255, 0, 0, 255), -1);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.type(), bgra_frame.type()) << "Should preserve format type";
    EXPECT_EQ(result.size(), bgra_frame.size()) << "Should preserve dimensions";
}

TEST_F(FormatSpecificTests, UnsupportedFormatHandling) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    cv::Mat unsupported_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC2);
    cv::Mat result = stabilizer.process_frame(unsupported_frame);

    EXPECT_TRUE(result.empty()) << "Should reject unsupported format";
}

TEST_F(FormatSpecificTests, NV12HighDefinition) {
    StabilizerCore stabilizer;
    stabilizer.initialize(3840, 2160, test_params);

    cv::Mat bgra_frame = generate_test_frame(3840, 2160, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle NV12 4K resolution";
}

TEST_F(FormatSpecificTests, I420HighDefinition) {
    StabilizerCore stabilizer;
    stabilizer.initialize(3840, 2160, test_params);

    cv::Mat bgra_frame = generate_test_frame(3840, 2160, 0);
    cv::Mat result = stabilizer.process_frame(bgra_frame);

    EXPECT_FALSE(result.empty()) << "Should handle I420 4K resolution";
}

TEST_F(FormatSpecificTests, MultipleFormatInSingleSession) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    std::vector<int> formats = {CV_8UC4, CV_8UC3, CV_8UC1};
    int successful_frames = 0;

    for (int i = 0; i < 30; i++) {
        int format = formats[i % 3];
        cv::Mat frame(Resolution::HD_HEIGHT, Resolution::HD_WIDTH, format, cv::Scalar(128));
        if (format == CV_8UC4) {
            frame.setTo(cv::Scalar(128, 128, 128, 255));
        }
        cv::Mat result = stabilizer.process_frame(frame);
        if (!result.empty()) {
            successful_frames++;
            EXPECT_EQ(result.channels(), (format == CV_8UC4) ? 4 : 
                                           (format == CV_8UC3) ? 3 : 1);
        }
    }

    EXPECT_GT(successful_frames, 25) << "Should handle multiple formats in single session";
}

TEST_F(FormatSpecificTests, FormatSwitchingDuringSequence) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, test_params);

    cv::Mat frame1 = generate_test_frame(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, 0);
    cv::Mat result1 = stabilizer.process_frame(frame1);
    EXPECT_FALSE(result1.empty());

    cv::Mat frame2(Resolution::HD720_HEIGHT, Resolution::HD720_WIDTH, CV_8UC3, cv::Scalar(100, 100, 100));
    cv::Mat result2 = stabilizer.process_frame(frame2);
    EXPECT_FALSE(result2.empty());

    cv::Mat frame3(Resolution::HD720_HEIGHT, Resolution::HD720_WIDTH, CV_8UC1, cv::Scalar(100));
    cv::Mat result3 = stabilizer.process_frame(frame3);
    EXPECT_FALSE(result3.empty());

    cv::Mat frame4 = generate_test_frame(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, 1);
    cv::Mat result4 = stabilizer.process_frame(frame4);
    EXPECT_FALSE(result4.empty());
}

TEST_F(FormatSpecificTests, AlphaChannelPreservation) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    cv::Mat frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC4);
    for (int y = 0; y < Resolution::VGA_HEIGHT; y++) {
        for (int x = 0; x < Resolution::VGA_WIDTH; x++) {
            frame.at<cv::Vec4b>(y, x) = cv::Vec4b(100, 100, 100, 
                static_cast<uchar>((x * 255) / Resolution::VGA_WIDTH));
        }
    }

    cv::Mat result = stabilizer.process_frame(frame);
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.channels(), 4) << "Should preserve alpha channel";
}

TEST_F(FormatSpecificTests, LargeFrameFormat) {
    StabilizerCore stabilizer;
    stabilizer.initialize(4096, 3072, test_params);

    cv::Mat frame(3072, 4096, CV_8UC4, cv::Scalar(128, 128, 128, 255));
    cv::Mat result = stabilizer.process_frame(frame);

    EXPECT_FALSE(result.empty()) << "Should handle large frame formats";
    EXPECT_EQ(result.size(), frame.size());
}

#endif // SKIP_OPENCV_TESTS
