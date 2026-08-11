/*
 * OBS filter runtime regression tests
 *
 * The plugin implementation is included directly so these tests exercise the
 * actual static OBS callbacks with the isolated test stubs. This catches
 * registration and first-frame control-flow bugs that core-only tests cannot.
 */

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>

#define OBS_DECLARE_MODULE()
#define OBS_MODULE_AUTHOR(author)
#include "../src/stabilizer_opencv.cpp"

TEST(FilterRuntimeTest, RegistersAsAsyncVideoFilter)
{
    EXPECT_EQ(stabilizer_filter_info.type, OBS_SOURCE_TYPE_FILTER);
    EXPECT_NE(stabilizer_filter_info.output_flags & OBS_SOURCE_VIDEO, 0u);
    EXPECT_NE(stabilizer_filter_info.output_flags & OBS_SOURCE_ASYNC, 0u);
    EXPECT_NE(stabilizer_filter_info.filter_video, nullptr);
}

TEST(FilterRuntimeTest, InitializesAndProcessesTheFirstFrame)
{
    stabilizer_filter context{};
    context.params = StabilizerCore::get_preset_streaming();

    constexpr uint32_t width = 96;
    constexpr uint32_t height = 64;
    cv::Mat pixels(height, width, CV_8UC4);
    cv::randu(pixels, 0, 255);

    obs_source_frame frame{};
    frame.width = width;
    frame.height = height;
    frame.format = VIDEO_FORMAT_BGRA;
    frame.data[0] = pixels.data;
    frame.linesize[0] = static_cast<uint32_t>(pixels.step);

    obs_source_frame *result = stabilizer_filter_video(&context, &frame);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(context.stabilizer.is_initialized());
    EXPECT_EQ(context.frame_width, width);
    EXPECT_EQ(context.frame_height, height);
    EXPECT_EQ(context.frame_count, 1u);
    EXPECT_EQ(&frame, result);
}
