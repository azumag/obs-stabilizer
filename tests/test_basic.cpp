#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <cstdint>
#include <stdexcept>
#include <typeinfo>
#include <vector>
#include "test_constants.hpp"
#include "test_data_generator.hpp"
#include "core/logging.hpp"

#ifdef HAVE_OBS_HEADERS
#include "obs_minimal.h"
#include "core/frame_utils.hpp"
#endif

using namespace TestConstants;

class BasicTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(BasicTest, TestOpenCVInitialization) {
    cv::Mat test_frame = TestDataGenerator::generate_test_frame(
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT
    );
    EXPECT_FALSE(test_frame.empty());
    EXPECT_EQ(test_frame.cols, Resolution::VGA_WIDTH);
    EXPECT_EQ(test_frame.rows, Resolution::VGA_HEIGHT);
}

TEST_F(BasicTest, TestFrameGeneration) {
    cv::Mat frame = TestDataGenerator::generate_test_frame(640, 480);
    EXPECT_EQ(frame.type(), CV_8UC4);
    EXPECT_EQ(frame.cols, 640);
    EXPECT_EQ(frame.rows, 480);
}

TEST_F(BasicTest, TestMotionFrameGeneration) {
    cv::Mat base_frame = TestDataGenerator::generate_test_frame(640, 480);
    cv::Mat motion_frame = TestDataGenerator::create_motion_frame(base_frame, 10.0f, 20.0f, 5.0f);
    EXPECT_FALSE(motion_frame.empty());
    EXPECT_EQ(motion_frame.size(), base_frame.size());
}

TEST_F(BasicTest, TestSequenceGeneration) {
    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "static"
    );
    EXPECT_EQ(frames.size(), FrameCount::STANDARD_SEQUENCE);

    for (const auto& frame : frames) {
        EXPECT_FALSE(frame.empty());
        EXPECT_EQ(frame.cols, Resolution::VGA_WIDTH);
        EXPECT_EQ(frame.rows, Resolution::VGA_HEIGHT);
    }
}

TEST_F(BasicTest, TestDifferentVideoFormats) {
    cv::Mat frame_bgra = TestDataGenerator::generate_frame_in_format(640, 480, CV_8UC4);
    EXPECT_EQ(frame_bgra.type(), CV_8UC4);
}

TEST_F(BasicTest, TestFrameWithFeatures) {
    cv::Mat frame = TestDataGenerator::create_frame_with_features(640, 480, Features::DEFAULT_COUNT);
    EXPECT_FALSE(frame.empty());
    EXPECT_EQ(frame.cols, 640);
    EXPECT_EQ(frame.rows, 480);
}

TEST_F(BasicTest, TestComprehensiveTestData) {
    auto data = TestDataGenerator::generate_comprehensive_test_data(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT
    );
    EXPECT_EQ(data.frames.size(), FrameCount::STANDARD_SEQUENCE);
    EXPECT_EQ(data.width, Resolution::VGA_WIDTH);
    EXPECT_EQ(data.height, Resolution::VGA_HEIGHT);
}

TEST_F(BasicTest, TestHorizontalMotionFrame) {
    cv::Mat base_frame = TestDataGenerator::generate_test_frame(640, 480);
    cv::Mat h_motion = TestDataGenerator::generate_horizontal_motion_frame(
        base_frame,
        5,
        FrameCount::STANDARD_SEQUENCE
    );
    EXPECT_FALSE(h_motion.empty());
}

TEST_F(BasicTest, TestVerticalMotionFrame) {
    cv::Mat base_frame = TestDataGenerator::generate_test_frame(640, 480);
    cv::Mat v_motion = TestDataGenerator::generate_vertical_motion_frame(
        base_frame,
        5,
        FrameCount::STANDARD_SEQUENCE
    );
    EXPECT_FALSE(v_motion.empty());
}

TEST_F(BasicTest, TestRotationFrame) {
    cv::Mat base_frame = TestDataGenerator::generate_test_frame(640, 480);
    cv::Mat rot_frame = TestDataGenerator::generate_rotation_frame(
        base_frame,
        5,
        FrameCount::STANDARD_SEQUENCE,
        Motion::MEDIUM_ROTATION
    );
    EXPECT_FALSE(rot_frame.empty());
}

TEST_F(BasicTest, TestZoomFrame) {
    cv::Mat base_frame = TestDataGenerator::generate_test_frame(640, 480);
    cv::Mat zoom_frame = TestDataGenerator::generate_zoom_frame(
        base_frame,
        5,
        FrameCount::STANDARD_SEQUENCE,
        1.02f
    );
    EXPECT_FALSE(zoom_frame.empty());
}

TEST_F(BasicTest, TestResolutionConstants) {
    EXPECT_EQ(Resolution::VGA_WIDTH, 640);
    EXPECT_EQ(Resolution::VGA_HEIGHT, 480);
    EXPECT_EQ(Resolution::HD_WIDTH, 1920);
    EXPECT_EQ(Resolution::HD_HEIGHT, 1080);
}

TEST_F(BasicTest, TestFrameCountConstants) {
    EXPECT_GT(FrameCount::SHORT_SEQUENCE, 0);
    EXPECT_GT(FrameCount::STANDARD_SEQUENCE, FrameCount::SHORT_SEQUENCE);
    EXPECT_GT(FrameCount::LONG_SEQUENCE, FrameCount::STANDARD_SEQUENCE);
    EXPECT_GT(FrameCount::VERY_LONG_SEQUENCE, FrameCount::LONG_SEQUENCE);
}

TEST_F(BasicTest, TestMotionConstants) {
    EXPECT_GT(Motion::SMALL_MOTION, 0);
    EXPECT_GT(Motion::MEDIUM_MOTION, Motion::SMALL_MOTION);
    EXPECT_GT(Motion::LARGE_MOTION, Motion::MEDIUM_MOTION);
    EXPECT_GT(Motion::SMALL_ROTATION, 0.0f);
    EXPECT_GT(Motion::MEDIUM_ROTATION, Motion::SMALL_ROTATION);
    EXPECT_GT(Motion::LARGE_ROTATION, Motion::MEDIUM_ROTATION);
    EXPECT_GT(Motion::DEFAULT_ZOOM, 1.0f);
}

TEST_F(BasicTest, TestFeatureConstants) {
    EXPECT_GE(Features::MIN_COUNT, 10);
    EXPECT_GT(Features::LOW_COUNT, Features::MIN_COUNT);
    EXPECT_GT(Features::DEFAULT_COUNT, Features::LOW_COUNT);
    EXPECT_GT(Features::HIGH_COUNT, Features::DEFAULT_COUNT);
    EXPECT_LE(Features::MAX_COUNT, 1000);
}

TEST_F(BasicTest, TestProcessingConstants) {
    EXPECT_GT(Processing::SMALL_SMOOTHING_WINDOW, 0);
    EXPECT_GT(Processing::MEDIUM_SMOOTHING_WINDOW, Processing::SMALL_SMOOTHING_WINDOW);
    EXPECT_GT(Processing::LARGE_SMOOTHING_WINDOW, Processing::MEDIUM_SMOOTHING_WINDOW);
    EXPECT_GT(Processing::DEFAULT_QUALITY_LEVEL, 0.0f);
    EXPECT_GT(Processing::DEFAULT_MIN_DISTANCE, 0.0f);
}

TEST_F(BasicTest, TestExceptionTelemetryTracksCategoriesAndTypes) {
    using namespace StabilizerLogging;

    reset_exception_telemetry();

    std::runtime_error standard_error("standard failure");
    log_exception("basic_test", standard_error);

    cv::Exception opencv_error(
        cv::Error::StsError,
        "opencv failure",
        "basic_test",
        __FILE__,
        __LINE__
    );
    log_opencv_exception("basic_test", opencv_error);
    log_unknown_exception("basic_test");

    const ExceptionTelemetrySnapshot snapshot = get_exception_telemetry();
    EXPECT_EQ(snapshot.total, 3u);
    EXPECT_EQ(snapshot.standard, 1u);
    EXPECT_EQ(snapshot.opencv, 1u);
    EXPECT_EQ(snapshot.unknown, 1u);
    EXPECT_EQ(snapshot.by_type.at(typeid(standard_error).name()), 1u);
    EXPECT_EQ(snapshot.by_type.at("cv::Exception"), 1u);
    EXPECT_EQ(snapshot.by_type.at("unknown"), 1u);
}

TEST_F(BasicTest, TestSafeCallRecordsExceptionTelemetry) {
    using namespace StabilizerLogging;

    reset_exception_telemetry();

    const int result = safe_call(
        []() -> int {
            throw std::logic_error("safe_call failure");
        },
        "safe_call_test",
        -1
    );

    EXPECT_EQ(result, -1);
    const ExceptionTelemetrySnapshot snapshot = get_exception_telemetry();
    EXPECT_EQ(snapshot.total, 1u);
    EXPECT_EQ(snapshot.standard, 1u);
    EXPECT_EQ(snapshot.opencv, 0u);
    EXPECT_EQ(snapshot.unknown, 0u);
}

#ifdef HAVE_OBS_HEADERS
TEST_F(BasicTest, TestFrameDimensionValidation) {
    EXPECT_GT(FRAME_UTILS::MAX_FRAME_WIDTH, 0u);
    EXPECT_GT(FRAME_UTILS::MAX_FRAME_HEIGHT, 0u);

    EXPECT_EQ(FRAME_UTILS::MAX_FRAME_WIDTH, 16384u);
    EXPECT_EQ(FRAME_UTILS::MAX_FRAME_HEIGHT, 16384u);
}

TEST_F(BasicTest, TestOverflowProtectionLargeFrame) {
    obs_source_frame frame{};
    frame.width = 20000;
    frame.height = 20000;
    frame.format = VIDEO_FORMAT_BGRA;
    frame.data[0] = nullptr;

    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);
    EXPECT_TRUE(result.empty());
}

TEST_F(BasicTest, TestOverflowProtectionZeroDimension) {
    obs_source_frame frame{};
    frame.width = 0;
    frame.height = 480;
    frame.format = VIDEO_FORMAT_BGRA;
    frame.data[0] = nullptr;

    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);
    EXPECT_TRUE(result.empty());

    frame.width = 640;
    frame.height = 0;
    result = FRAME_UTILS::Conversion::obs_to_cv(&frame);
    EXPECT_TRUE(result.empty());
}

TEST_F(BasicTest, TestUnsupportedObsFormatReturnsEmpty) {
    constexpr uint32_t width = 32;
    constexpr uint32_t height = 32;
    std::vector<uint8_t> pixels(width * height * 4, 0);

    obs_source_frame frame{};
    frame.width = width;
    frame.height = height;
    frame.linesize[0] = width * 4;
    frame.data[0] = pixels.data();
    frame.format = static_cast<decltype(frame.format)>(UINT32_MAX);

    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);
    EXPECT_TRUE(result.empty());
}

TEST_F(BasicTest, TestI420MissingChromaPlaneReturnsEmpty) {
    constexpr uint32_t width = 32;
    constexpr uint32_t height = 32;
    std::vector<uint8_t> luma(width * height, 0);

    obs_source_frame frame{};
    frame.width = width;
    frame.height = height;
    frame.linesize[0] = width;
    frame.data[0] = luma.data();
    frame.data[1] = nullptr;
    frame.data[2] = nullptr;
    frame.format = VIDEO_FORMAT_I420;

    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);
    EXPECT_TRUE(result.empty());
}
#endif

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
