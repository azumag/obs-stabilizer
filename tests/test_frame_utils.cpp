/*
 * Frame Utils Tests
 *
 * RATIONALE: These tests verify the frame conversion utilities in FRAME_UTILS namespace.
 * This is critical for the OBS integration path where frames need to be converted
 * between OBS format (obs_source_frame) and OpenCV format (cv::Mat).
 *
 * Test coverage focus:
 * - Frame validation (validate_obs_frame, validate_cv_mat)
 * - Color conversion (convert_to_grayscale)
 * - Frame format validation and error handling
 * - Edge cases (null frames, empty mats, invalid dimensions)
 */

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <cstring>

#include "test_constants.hpp"
#include "test_data_generator.hpp"
#include "core/frame_utils.hpp"

using namespace TestConstants;

class FrameUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Validation Tests
// ============================================================================

/**
 * Test: Validate valid OpenCV Mat
 * EXPECTED: Returns true for valid Mat
 */
TEST_F(FrameUtilsTest, ValidateValidCVMat) {
    cv::Mat frame = TestDataGenerator::generate_test_frame(
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT
    );

    EXPECT_TRUE(FRAME_UTILS::Validation::validate_cv_mat(frame));
}

/**
 * Test: Validate empty OpenCV Mat
 * EXPECTED: Returns false for empty Mat
 */
TEST_F(FrameUtilsTest, ValidateEmptyCVMat) {
    cv::Mat empty_mat;
    EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(empty_mat));
}

/**
 * Test: Validate Mat with invalid dimensions
 * EXPECTED: Returns false for Mat with negative or zero dimensions
 */
TEST_F(FrameUtilsTest, ValidateInvalidDimensionsCVMat) {
    cv::Mat invalid_width(0, 640, CV_8UC3);
    cv::Mat invalid_height(640, 0, CV_8UC3);

    EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(invalid_width));
    EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(invalid_height));

    // Note: OpenCV doesn't allow negative dimensions (throws cv::Exception)
    // This is handled by the OpenCV library itself, so we don't need to test
    // negative dimensions separately in our validation function
}

/**
 * Test: Validate Mat with invalid depth
 * EXPECTED: Returns false for non-8-bit depth
 */
TEST_F(FrameUtilsTest, ValidateInvalidDepthCVMat) {
    cv::Mat mat_16bit(640, 480, CV_16UC3);
    cv::Mat mat_float(640, 480, CV_32FC3);

    EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(mat_16bit));
    EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(mat_float));
}

/**
 * Test: Validate Mat with invalid channel count
 * EXPECTED: Returns false for 2-channel Mat
 */
TEST_F(FrameUtilsTest, ValidateInvalidChannelsCVMat) {
    cv::Mat mat_2channel(640, 480, CV_8UC2);

    EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(mat_2channel));
}

/**
 * Test: Validate Mat with valid channel counts (1, 3, 4)
 * EXPECTED: Returns true for valid channel counts
 */
TEST_F(FrameUtilsTest, ValidateValidChannelsCVMat) {
    cv::Mat mat_1channel(640, 480, CV_8UC1);
    cv::Mat mat_3channel(640, 480, CV_8UC3);
    cv::Mat mat_4channel(640, 480, CV_8UC4);

    EXPECT_TRUE(FRAME_UTILS::Validation::validate_cv_mat(mat_1channel));
    EXPECT_TRUE(FRAME_UTILS::Validation::validate_cv_mat(mat_3channel));
    EXPECT_TRUE(FRAME_UTILS::Validation::validate_cv_mat(mat_4channel));
}

// ============================================================================
// Color Conversion Tests
// ============================================================================

/**
 * Test: Convert BGRA to grayscale
 * EXPECTED: Returns valid grayscale Mat with 1 channel
 */
TEST_F(FrameUtilsTest, ConvertBGRAToGrayscale) {
    cv::Mat bgra_frame(640, 480, CV_8UC4);
    cv::randu(bgra_frame, 0, 255);

    cv::Mat gray = FRAME_UTILS::ColorConversion::convert_to_grayscale(bgra_frame);

    EXPECT_FALSE(gray.empty());
    EXPECT_EQ(gray.channels(), 1);
    EXPECT_EQ(gray.size(), bgra_frame.size());
}

/**
 * Test: Convert BGR to grayscale
 * EXPECTED: Returns valid grayscale Mat with 1 channel
 */
TEST_F(FrameUtilsTest, ConvertBGRToGrayscale) {
    cv::Mat bgr_frame(640, 480, CV_8UC3);
    cv::randu(bgr_frame, 0, 255);

    cv::Mat gray = FRAME_UTILS::ColorConversion::convert_to_grayscale(bgr_frame);

    EXPECT_FALSE(gray.empty());
    EXPECT_EQ(gray.channels(), 1);
    EXPECT_EQ(gray.size(), bgr_frame.size());
}

/**
 * Test: Convert grayscale to grayscale (no-op)
 * EXPECTED: Returns same Mat (no conversion needed)
 */
TEST_F(FrameUtilsTest, ConvertGrayscaleToGrayscale) {
    cv::Mat gray_frame(640, 480, CV_8UC1);
    cv::randu(gray_frame, 0, 255);

    cv::Mat result = FRAME_UTILS::ColorConversion::convert_to_grayscale(gray_frame);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.channels(), 1);
    EXPECT_EQ(result.size(), gray_frame.size());
}

/**
 * Test: Convert empty frame to grayscale
 * EXPECTED: Returns empty Mat
 */
TEST_F(FrameUtilsTest, ConvertEmptyToGrayscale) {
    cv::Mat empty_frame;
    cv::Mat result = FRAME_UTILS::ColorConversion::convert_to_grayscale(empty_frame);

    EXPECT_TRUE(result.empty());
}

/**
 * Test: Convert invalid channel count to grayscale
 * EXPECTED: Returns empty Mat
 */
TEST_F(FrameUtilsTest, ConvertInvalidChannelsToGrayscale) {
    cv::Mat mat_2channel(640, 480, CV_8UC2);
    cv::Mat result = FRAME_UTILS::ColorConversion::convert_to_grayscale(mat_2channel);

    EXPECT_TRUE(result.empty());
}

// ============================================================================
// Frame Format Tests (OBS-specific)
// ============================================================================

/**
 * Test: Check if format is supported
 * EXPECTED: Returns true for supported formats (BGRA, BGRX, BGR3, NV12, I420)
 */
#ifdef HAVE_OBS_HEADERS
TEST_F(FrameUtilsTest, IsSupportedFormat) {
    EXPECT_TRUE(FRAME_UTILS::Conversion::is_supported_format(VIDEO_FORMAT_BGRA));
    EXPECT_TRUE(FRAME_UTILS::Conversion::is_supported_format(VIDEO_FORMAT_BGRX));
    EXPECT_TRUE(FRAME_UTILS::Conversion::is_supported_format(VIDEO_FORMAT_BGR3));
    EXPECT_TRUE(FRAME_UTILS::Conversion::is_supported_format(VIDEO_FORMAT_NV12));
    EXPECT_TRUE(FRAME_UTILS::Conversion::is_supported_format(VIDEO_FORMAT_I420));
    EXPECT_FALSE(FRAME_UTILS::Conversion::is_supported_format(VIDEO_FORMAT_I444));
}

/**
 * Test: Get format name
 * EXPECTED: Returns correct format name string
 */
TEST_F(FrameUtilsTest, GetFormatName) {
    EXPECT_EQ(FRAME_UTILS::Conversion::get_format_name(VIDEO_FORMAT_BGRA), "BGRA");
    EXPECT_EQ(FRAME_UTILS::Conversion::get_format_name(VIDEO_FORMAT_BGRX), "BGRX");
    EXPECT_EQ(FRAME_UTILS::Conversion::get_format_name(VIDEO_FORMAT_BGR3), "BGR3");
    EXPECT_EQ(FRAME_UTILS::Conversion::get_format_name(VIDEO_FORMAT_NV12), "NV12");
    EXPECT_EQ(FRAME_UTILS::Conversion::get_format_name(VIDEO_FORMAT_I420), "I420");
    EXPECT_EQ(FRAME_UTILS::Conversion::get_format_name(999), "UNKNOWN");
}

/**
 * Test: Convert null OBS frame to OpenCV Mat
 * EXPECTED: Returns empty Mat and tracks conversion failure
 */
TEST_F(FrameUtilsTest, ConvertNullOBSFrameToCV) {
    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(nullptr);

    EXPECT_TRUE(result.empty());

    // Verify conversion failure was tracked
    auto stats = FRAME_UTILS::Performance::get_stats();
    EXPECT_GT(stats.failed_conversions, 0);
}

/**
 * Test: Convert OBS frame with null data to OpenCV Mat
 * EXPECTED: Returns empty Mat
 */
TEST_F(FrameUtilsTest, ConvertOBSFrameWithNullDataToCV) {
    obs_source_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.width = 640;
    frame.height = 480;
    frame.format = VIDEO_FORMAT_BGRA;

    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);

    EXPECT_TRUE(result.empty());
}

/**
 * Test: Convert OBS frame with invalid dimensions to OpenCV Mat
 * EXPECTED: Returns empty Mat
 */
TEST_F(FrameUtilsTest, ConvertOBSFrameWithInvalidDimensionsToCV) {
    obs_source_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.width = 0;
    frame.height = 0;
    frame.format = VIDEO_FORMAT_BGRA;

    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);

    EXPECT_TRUE(result.empty());
}

/**
 * Test: Convert OBS frame with unsupported format to OpenCV Mat
 * EXPECTED: Returns empty Mat
 */
TEST_F(FrameUtilsTest, ConvertOBSFrameWithUnsupportedFormatToCV) {
    // Create a minimal OBS frame with an unsupported format
    cv::Mat data(640, 480, CV_8UC4);
    obs_source_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.width = 640;
    frame.height = 480;
    frame.format = VIDEO_FORMAT_RGBA;  // Unsupported format
    frame.data[0] = data.data;
    frame.linesize[0] = static_cast<uint32_t>(data.step);

    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);

    // Should return empty for unsupported format
    EXPECT_TRUE(result.empty());
}

/**
 * Test: Validate OBS frame with null pointer
 * EXPECTED: Returns false
 */
TEST_F(FrameUtilsTest, ValidateNullOBSFrame) {
    EXPECT_FALSE(FRAME_UTILS::Validation::validate_obs_frame(nullptr));
}

/**
 * Test: Validate OBS frame with null data
 * EXPECTED: Returns false
 */
TEST_F(FrameUtilsTest, ValidateOBSFrameWithNullData) {
    obs_source_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.width = 640;
    frame.height = 480;
    frame.format = VIDEO_FORMAT_BGRA;

    EXPECT_FALSE(FRAME_UTILS::Validation::validate_obs_frame(&frame));
}

/**
 * Test: Validate OBS frame with zero dimensions
 * EXPECTED: Returns false
 */
TEST_F(FrameUtilsTest, ValidateOBSFrameWithZeroDimensions) {
    obs_source_frame frame;
    memset(&frame, 0, sizeof(frame));

    EXPECT_FALSE(FRAME_UTILS::Validation::validate_obs_frame(&frame));
}

/**
 * Test: Validate OBS frame with unsupported format
 * EXPECTED: Returns false
 */
TEST_F(FrameUtilsTest, ValidateOBSFrameWithUnsupportedFormat) {
    obs_source_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.width = 640;
    frame.height = 480;
    frame.format = VIDEO_FORMAT_RGBA;  // Unsupported

    EXPECT_FALSE(FRAME_UTILS::Validation::validate_obs_frame(&frame));
}

/**
 * Test: Get error message for invalid OBS frame
 * EXPECTED: Returns descriptive error message
 */
TEST_F(FrameUtilsTest, GetOBSFrameErrorMessage) {
    // Null frame
    std::string null_msg = FRAME_UTILS::Validation::get_frame_error_message(nullptr);
    EXPECT_FALSE(null_msg.empty());

    // Frame with null data
    obs_source_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.width = 640;
    frame.height = 480;
    frame.format = VIDEO_FORMAT_BGRA;
    std::string null_data_msg = FRAME_UTILS::Validation::get_frame_error_message(&frame);
    EXPECT_FALSE(null_data_msg.empty());
    EXPECT_NE(null_data_msg, null_msg);
}

/**
 * Test: Create frame buffer with empty Mat
 * EXPECTED: Returns nullptr
 */
TEST_F(FrameUtilsTest, CreateFrameBufferWithEmptyMat) {
    cv::Mat empty_mat;
    obs_source_frame ref_frame;
    memset(&ref_frame, 0, sizeof(ref_frame));
    ref_frame.width = 640;
    ref_frame.height = 480;
    ref_frame.format = VIDEO_FORMAT_BGRA;

    obs_source_frame* result = FRAME_UTILS::FrameBuffer::create(empty_mat, &ref_frame);

    EXPECT_EQ(result, nullptr);
}

/**
 * Test: Create frame buffer with null reference frame
 * EXPECTED: Returns nullptr
 */
TEST_F(FrameUtilsTest, CreateFrameBufferWithNullReference) {
    cv::Mat mat(640, 480, CV_8UC4);

    obs_source_frame* result = FRAME_UTILS::FrameBuffer::create(mat, nullptr);

    EXPECT_EQ(result, nullptr);
}

/**
 * Test: Create frame buffer with invalid reference dimensions
 * EXPECTED: Returns nullptr
 */
TEST_F(FrameUtilsTest, CreateFrameBufferWithInvalidReferenceDimensions) {
    cv::Mat mat(640, 480, CV_8UC4);
    obs_source_frame ref_frame;
    memset(&ref_frame, 0, sizeof(ref_frame));
    ref_frame.width = 0;
    ref_frame.height = 0;
    ref_frame.format = VIDEO_FORMAT_BGRA;

    obs_source_frame* result = FRAME_UTILS::FrameBuffer::create(mat, &ref_frame);

    EXPECT_EQ(result, nullptr);
}

/**
 * Test: Release frame buffer with null pointer
 * EXPECTED: Does not crash
 */
TEST_F(FrameUtilsTest, ReleaseNullFrameBuffer) {
    FRAME_UTILS::FrameBuffer::release(nullptr);
    // Should not crash
}

#endif // HAVE_OBS_HEADERS

// ============================================================================
// Performance Tracking Tests
// ============================================================================

/**
 * Test: Track conversion failure
 * EXPECTED: Failure counter increments
 */
TEST_F(FrameUtilsTest, TrackConversionFailure) {
    // Get initial stats
    auto initial_stats = FRAME_UTILS::Performance::get_stats();

    // Track a failure
    FRAME_UTILS::Performance::track_conversion_failure();

    // Verify counter incremented
    auto final_stats = FRAME_UTILS::Performance::get_stats();
    EXPECT_EQ(final_stats.failed_conversions, initial_stats.failed_conversions + 1);
}

/**
 * Test: Track multiple conversion failures
 * EXPECTED: Failure counter increments correctly
 */
TEST_F(FrameUtilsTest, TrackMultipleConversionFailures) {
    size_t num_failures = 5;

    // Get initial stats
    auto initial_stats = FRAME_UTILS::Performance::get_stats();

    // Track multiple failures
    for (size_t i = 0; i < num_failures; ++i) {
        FRAME_UTILS::Performance::track_conversion_failure();
    }

    // Verify counter incremented correctly
    auto final_stats = FRAME_UTILS::Performance::get_stats();
    EXPECT_EQ(final_stats.failed_conversions, initial_stats.failed_conversions + num_failures);
}

// ============================================================================
// Success Path Conversion Tests (OBS-specific)
// ============================================================================

#ifdef HAVE_OBS_HEADERS

/**
 * Test: Convert valid BGRA OBS frame to OpenCV Mat
 * EXPECTED: Returns valid BGRA Mat with correct dimensions and data
 *
 * RATIONALE: This test verifies the success path for BGRA format conversion,
 * which is the most common format in OBS. This is critical for integration
 * testing and ensures the frame conversion pipeline works correctly.
 */
TEST_F(FrameUtilsTest, ConvertValidBGRAOBSFrameToCV) {
    // Create a test BGRA frame with continuous memory
    cv::Mat test_data(640, 480, CV_8UC4);
    cv::randu(test_data, 0, 255);
    test_data = test_data.clone();  // Ensure continuous memory

    obs_source_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.width = 640;
    frame.height = 480;
    frame.format = VIDEO_FORMAT_BGRA;
    frame.data[0] = test_data.data;
    // Calculate correct linesize: width * channels (4 for BGRA)
    frame.linesize[0] = 640 * 4;

    // Convert OBS frame to OpenCV Mat
    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);

    // Verify successful conversion
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.size().width, 640);
    EXPECT_EQ(result.size().height, 480);
    EXPECT_EQ(result.channels(), 4);
    EXPECT_EQ(result.type(), CV_8UC4);

    // Verify data was copied (not a shallow copy)
    // Modify original data and verify result is unaffected
    test_data.setTo(cv::Scalar(0, 0, 0, 0));
    cv::Scalar mean_value = cv::mean(result);
    EXPECT_GT(mean_value[0], 0);  // Should not be zero if data was copied
}

/**
 * Test: Convert valid BGRX OBS frame to OpenCV Mat
 * EXPECTED: Returns valid BGRX Mat with correct dimensions and data
 *
 * RATIONALE: BGRX is another common OBS format where the alpha channel is unused.
 * This test verifies BGRX format is handled correctly.
 */
TEST_F(FrameUtilsTest, ConvertValidBGRXOBSFrameToCV) {
    // Create a test BGRX frame with continuous memory
    cv::Mat test_data(640, 480, CV_8UC4);
    cv::randu(test_data, 0, 255);
    test_data = test_data.clone();  // Ensure continuous memory

    obs_source_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.width = 640;
    frame.height = 480;
    frame.format = VIDEO_FORMAT_BGRX;
    frame.data[0] = test_data.data;
    // Calculate correct linesize: width * channels (4 for BGRX)
    frame.linesize[0] = 640 * 4;

    // Convert OBS frame to OpenCV Mat
    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);

    // Verify successful conversion
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.size().width, 640);
    EXPECT_EQ(result.size().height, 480);
    EXPECT_EQ(result.channels(), 4);
    EXPECT_EQ(result.type(), CV_8UC4);
}

/**
 * Test: Convert valid BGR3 OBS frame to OpenCV Mat
 * EXPECTED: Returns valid BGR3 Mat with correct dimensions and data
 *
 * RATIONALE: BGR3 is a 3-channel format without alpha channel. This test verifies
 * proper handling of 3-channel formats.
 */
TEST_F(FrameUtilsTest, ConvertValidBGR3OBSFrameToCV) {
    // Create a test BGR3 frame with continuous memory
    cv::Mat test_data(640, 480, CV_8UC3);
    cv::randu(test_data, 0, 255);
    test_data = test_data.clone();  // Ensure continuous memory

    obs_source_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.width = 640;
    frame.height = 480;
    frame.format = VIDEO_FORMAT_BGR3;
    frame.data[0] = test_data.data;
    // Calculate correct linesize: width * channels (3 for BGR3)
    frame.linesize[0] = 640 * 3;

    // Convert OBS frame to OpenCV Mat
    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);

    // Verify successful conversion
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.size().width, 640);
    EXPECT_EQ(result.size().height, 480);
    EXPECT_EQ(result.channels(), 3);
    EXPECT_EQ(result.type(), CV_8UC3);
}

/**
 * Test: Convert valid NV12 OBS frame to OpenCV Mat
 * EXPECTED: Returns valid BGRA Mat with correct dimensions and data
 *
 * RATIONALE: NV12 is a common YUV format used in video encoding. This test verifies
 * YUV to RGB conversion works correctly for this format.
 *
 * CRITICAL: This addresses QA Review Issue #1 - NV12 conversion success case was untested.
 */
TEST_F(FrameUtilsTest, ConvertValidNV12OBSFrameToCV) {
    // Create test NV12 frame data
    // NV12 format: Y plane followed by interleaved UV plane
    // Y plane: width * height bytes
    // UV plane: (width * height) / 2 bytes
    const int width = 640;
    const int height = 480;
    const int y_size = width * height;
    const int uv_size = (width * height) / 2;
    const int total_size = y_size + uv_size;

    std::vector<uint8_t> nv12_data(total_size);

    // Fill Y plane with gradient (0-255)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            nv12_data[y * width + x] = static_cast<uint8_t>((x + y) % 256);
        }
    }

    // Fill UV plane with gray (128, 128) for neutral colors
    for (int i = y_size; i < total_size; i += 2) {
        nv12_data[i] = 128;      // U
        nv12_data[i + 1] = 128;  // V
    }

    obs_source_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.width = width;
    frame.height = height;
    frame.format = VIDEO_FORMAT_NV12;
    frame.data[0] = nv12_data.data();
    frame.data[1] = nv12_data.data() + y_size;
    frame.linesize[0] = width;
    frame.linesize[1] = width;

    // Convert OBS frame to OpenCV Mat
    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);

    // Verify successful conversion
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.size().width, width);
    EXPECT_EQ(result.size().height, height);
    EXPECT_EQ(result.channels(), 4);  // NV12 converts to BGRA
    EXPECT_EQ(result.type(), CV_8UC4);

    // Verify the converted image is not uniform (gradient should be visible)
    cv::Scalar mean_val = cv::mean(result);
    cv::Scalar stddev_val;
    cv::meanStdDev(result, mean_val, stddev_val);
    EXPECT_GT(stddev_val[0], 10);  // Should have some variation from gradient
}

/**
 * Test: Convert NV12 with separate, padded Y and UV planes
 * EXPECTED: Plane strides are honored and padding bytes never enter the image
 *
 * RATIONALE: OBS exposes NV12 as separate planes in real media-source frames.
 * Treating data[0] as one contiguous allocation reads beyond the Y plane and
 * can corrupt or crash live filter processing.
 */
TEST_F(FrameUtilsTest, ConvertPaddedSeparateNV12PlanesToCV) {
    constexpr int width = 16;
    constexpr int height = 8;
    constexpr int y_stride = width + 8;
    constexpr int uv_stride = width + 8;

    cv::Mat bgr(height, width, CV_8UC3);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            bgr.at<cv::Vec3b>(y, x) = cv::Vec3b(
                static_cast<uint8_t>(x * 11),
                static_cast<uint8_t>(y * 23),
                static_cast<uint8_t>((x + y) * 7));
        }
    }

    cv::Mat i420;
    cv::cvtColor(bgr, i420, cv::COLOR_BGR2YUV_I420);
    const int y_size = width * height;
    const int chroma_size = y_size / 4;
    std::vector<uint8_t> packed_nv12(y_size + chroma_size * 2);
    memcpy(packed_nv12.data(), i420.data, y_size);
    for (int i = 0; i < chroma_size; ++i) {
        packed_nv12[y_size + i * 2] = i420.data[y_size + i];
        packed_nv12[y_size + i * 2 + 1] = i420.data[y_size + chroma_size + i];
    }

    std::vector<uint8_t> y_plane(y_stride * height, 0xEE);
    std::vector<uint8_t> uv_plane(uv_stride * (height / 2), 0xDD);
    for (int row = 0; row < height; ++row) {
        memcpy(y_plane.data() + row * y_stride,
               packed_nv12.data() + row * width, width);
    }
    for (int row = 0; row < height / 2; ++row) {
        memcpy(uv_plane.data() + row * uv_stride,
               packed_nv12.data() + y_size + row * width, width);
    }

    obs_source_frame frame{};
    frame.width = width;
    frame.height = height;
    frame.format = VIDEO_FORMAT_NV12;
    frame.data[0] = y_plane.data();
    frame.data[1] = uv_plane.data();
    frame.linesize[0] = y_stride;
    frame.linesize[1] = uv_stride;

    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);
    cv::Mat expected_yuv(height + height / 2, width, CV_8UC1, packed_nv12.data());
    cv::Mat expected;
    cv::cvtColor(expected_yuv, expected, cv::COLOR_YUV2BGRA_NV12);

    ASSERT_FALSE(result.empty());
    EXPECT_EQ(cv::norm(result, expected, cv::NORM_INF), 0.0);
}

/**
 * Test: Convert valid I420 OBS frame to OpenCV Mat
 * EXPECTED: Returns valid BGRA Mat with correct dimensions and data
 *
 * RATIONALE: I420 is another common YUV format used in video encoding. This test verifies
 * YUV to RGB conversion works correctly for this format with separate Y, U, V planes.
 *
 * CRITICAL: This addresses QA Review Issue #1 - I420 conversion success case was untested.
 */
TEST_F(FrameUtilsTest, ConvertValidI420OBSFrameToCV) {
    // Create test I420 frame data
    // I420 format: separate Y, U, V planes
    // Y plane: width * height bytes
    // U plane: (width / 2) * (height / 2) bytes
    // V plane: (width / 2) * (height / 2) bytes
    const int width = 640;
    const int height = 480;
    const int y_size = width * height;
    const int u_size = (width / 2) * (height / 2);
    const int v_size = (width / 2) * (height / 2);

    std::vector<uint8_t> y_data(y_size);
    std::vector<uint8_t> u_data(u_size);
    std::vector<uint8_t> v_data(v_size);

    // Fill Y plane with gradient (0-255)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            y_data[y * width + x] = static_cast<uint8_t>((x + y) % 256);
        }
    }

    // Fill U and V planes with gray (128) for neutral colors
    std::fill(u_data.begin(), u_data.end(), 128);
    std::fill(v_data.begin(), v_data.end(), 128);

    obs_source_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.width = width;
    frame.height = height;
    frame.format = VIDEO_FORMAT_I420;
    frame.data[0] = y_data.data();
    frame.data[1] = u_data.data();
    frame.data[2] = v_data.data();
    frame.linesize[0] = width;
    frame.linesize[1] = width / 2;
    frame.linesize[2] = width / 2;

    // Convert OBS frame to OpenCV Mat
    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);

    // Verify successful conversion
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.size().width, width);
    EXPECT_EQ(result.size().height, height);
    EXPECT_EQ(result.channels(), 4);  // I420 converts to BGRA
    EXPECT_EQ(result.type(), CV_8UC4);

    // Verify the converted image is not uniform (gradient should be visible)
    cv::Scalar mean_val = cv::mean(result);
    cv::Scalar stddev_val;
    cv::meanStdDev(result, mean_val, stddev_val);
    EXPECT_GT(stddev_val[0], 10);  // Should have some variation from gradient
}

/**
 * Test: Convert I420 with independently padded planes
 * EXPECTED: Y, U, and V row strides are respected
 */
TEST_F(FrameUtilsTest, ConvertPaddedI420PlanesToCV) {
    constexpr int width = 16;
    constexpr int height = 8;
    constexpr int y_stride = width + 8;
    constexpr int uv_stride = width / 2 + 4;

    cv::Mat bgr(height, width, CV_8UC3);
    cv::randu(bgr, 0, 255);
    cv::Mat packed_i420;
    cv::cvtColor(bgr, packed_i420, cv::COLOR_BGR2YUV_I420);

    const int y_size = width * height;
    const int chroma_width = width / 2;
    const int chroma_height = height / 2;
    const int chroma_size = chroma_width * chroma_height;
    std::vector<uint8_t> y_plane(y_stride * height, 0xEE);
    std::vector<uint8_t> u_plane(uv_stride * chroma_height, 0xDD);
    std::vector<uint8_t> v_plane(uv_stride * chroma_height, 0xCC);

    for (int row = 0; row < height; ++row) {
        memcpy(y_plane.data() + row * y_stride,
               packed_i420.data + row * width, width);
    }
    for (int row = 0; row < chroma_height; ++row) {
        memcpy(u_plane.data() + row * uv_stride,
               packed_i420.data + y_size + row * chroma_width, chroma_width);
        memcpy(v_plane.data() + row * uv_stride,
               packed_i420.data + y_size + chroma_size + row * chroma_width,
               chroma_width);
    }

    obs_source_frame frame{};
    frame.width = width;
    frame.height = height;
    frame.format = VIDEO_FORMAT_I420;
    frame.data[0] = y_plane.data();
    frame.data[1] = u_plane.data();
    frame.data[2] = v_plane.data();
    frame.linesize[0] = y_stride;
    frame.linesize[1] = uv_stride;
    frame.linesize[2] = uv_stride;

    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);
    cv::Mat expected;
    cv::cvtColor(packed_i420, expected, cv::COLOR_YUV2BGRA_I420);

    ASSERT_FALSE(result.empty());
    EXPECT_EQ(cv::norm(result, expected, cv::NORM_INF), 0.0);
}

/**
 * Test: Convert valid OpenCV Mat to OBS frame (BGRA format)
 * EXPECTED: Returns valid OBS frame with correct format and data
 *
 * RATIONALE: This test verifies cv_to_obs() conversion for the reverse direction,
 * converting processed frames back to OBS format for output.
 *
 * CRITICAL: This addresses QA Review Issue #1 - cv_to_obs() conversion was untested.
 */
TEST_F(FrameUtilsTest, ConvertValidCVMatToOBSBGRA) {
    // Create test OpenCV Mat (BGRA) with continuous memory
    // Note: Mat constructor takes (rows, cols) = (height, width)
    const int width = 640;
    const int height = 480;
    cv::Mat test_mat(height, width, CV_8UC4);
    cv::randu(test_mat, 0, 255);
    test_mat = test_mat.clone();  // Ensure continuous memory

    // Create reference OBS frame (for metadata)
    obs_source_frame ref_frame;
    memset(&ref_frame, 0, sizeof(ref_frame));
    ref_frame.width = width;
    ref_frame.height = height;
    ref_frame.format = VIDEO_FORMAT_BGRA;
    ref_frame.timestamp = 12345;
    ref_frame.full_range = 1;
    ref_frame.flip = 0;
    ref_frame.flags = 0;

    // Convert OpenCV Mat to OBS frame
    obs_source_frame* result = FRAME_UTILS::Conversion::cv_to_obs(test_mat, &ref_frame);

    // Verify successful conversion
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->width, 640);
    EXPECT_EQ(result->height, 480);
    EXPECT_EQ(result->format, VIDEO_FORMAT_BGRA);
    EXPECT_EQ(result->timestamp, 12345);
    EXPECT_EQ(result->full_range, 1);
    EXPECT_EQ(result->flip, 0);
    EXPECT_EQ(result->flags, 0);
    EXPECT_NE(result->data[0], nullptr);

    // Verify data was copied correctly
    // Use memcpy to create continuous Mat and avoid step issues
    cv::Mat converted(result->height, result->width, CV_8UC4);
    memcpy(converted.data, result->data[0],
           result->height * result->linesize[0]);
    double diff = cv::norm(test_mat, converted, cv::NORM_L2);
    EXPECT_NEAR(diff, 0.0, 1.0);  // Should be nearly identical

    // Clean up
    FRAME_UTILS::FrameBuffer::release(result);
}

/**
 * Test: Convert a BGRA Mat back to OBS BGRX
 * EXPECTED: BGRX is supported consistently in both conversion directions
 */
TEST_F(FrameUtilsTest, ConvertValidCVMatToOBSBGRX) {
    constexpr int width = 64;
    constexpr int height = 48;
    cv::Mat test_mat(height, width, CV_8UC4);
    cv::randu(test_mat, 0, 255);

    obs_source_frame ref_frame{};
    ref_frame.width = width;
    ref_frame.height = height;
    ref_frame.format = VIDEO_FORMAT_BGRX;

    obs_source_frame *result = FRAME_UTILS::Conversion::cv_to_obs(test_mat, &ref_frame);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->format, VIDEO_FORMAT_BGRX);
    EXPECT_NE(result->data[0], nullptr);
    EXPECT_EQ(result->linesize[0], static_cast<uint32_t>(width * 4));

    FRAME_UTILS::FrameBuffer::release(result);
}

/**
 * Test: Create an OBS NV12 frame with explicit Y and UV planes
 * EXPECTED: The output layout matches OBS's two-plane contract
 */
TEST_F(FrameUtilsTest, CreateNV12FrameWithTwoPlanes) {
    constexpr int width = 64;
    constexpr int height = 48;
    cv::Mat test_mat(height, width, CV_8UC4);
    cv::randu(test_mat, 0, 255);

    obs_source_frame ref_frame{};
    ref_frame.width = width;
    ref_frame.height = height;
    ref_frame.format = VIDEO_FORMAT_NV12;

    obs_source_frame *result = FRAME_UTILS::Conversion::cv_to_obs(test_mat, &ref_frame);

    ASSERT_NE(result, nullptr);
    ASSERT_NE(result->data[0], nullptr);
    ASSERT_NE(result->data[1], nullptr);
    EXPECT_EQ(result->data[1], result->data[0] + width * height);
    EXPECT_EQ(result->linesize[0], static_cast<uint32_t>(width));
    EXPECT_EQ(result->linesize[1], static_cast<uint32_t>(width));
    EXPECT_EQ(result->data[2], nullptr);

    FRAME_UTILS::FrameBuffer::release(result);
}

/**
 * Test: Write converted NV12 pixels into an OBS-owned padded frame
 * EXPECTED: The frame object and plane padding remain owned by the caller
 */
TEST_F(FrameUtilsTest, WriteOddCroppedCVMatIntoExistingPaddedNV12Frame) {
    constexpr int width = 64;
    constexpr int height = 48;
    constexpr int stride = width + 8;
    cv::Mat test_mat(height - 3, width - 3, CV_8UC4);
    cv::randu(test_mat, 0, 255);

    std::vector<uint8_t> y_plane(stride * height, 0xEE);
    std::vector<uint8_t> uv_plane(stride * (height / 2), 0xDD);
    obs_source_frame destination{};
    destination.width = width;
    destination.height = height;
    destination.format = VIDEO_FORMAT_NV12;
    destination.data[0] = y_plane.data();
    destination.data[1] = uv_plane.data();
    destination.linesize[0] = stride;
    destination.linesize[1] = stride;

    ASSERT_TRUE(FRAME_UTILS::Conversion::cv_to_obs_in_place(test_mat, &destination));

    for (int row = 0; row < height; ++row) {
        EXPECT_EQ(y_plane[row * stride + width], 0xEE);
    }
    for (int row = 0; row < height / 2; ++row) {
        EXPECT_EQ(uv_plane[row * stride + width], 0xDD);
    }

    cv::Mat round_trip = FRAME_UTILS::Conversion::obs_to_cv(&destination);
    ASSERT_FALSE(round_trip.empty());
    EXPECT_EQ(round_trip.rows, height);
    EXPECT_EQ(round_trip.cols, width);
}

/**
 * Test: Convert valid OpenCV Mat to OBS frame (BGR3 format)
 * EXPECTED: Returns valid OBS frame with correct format and data
 *
 * RATIONALE: This test verifies cv_to_obs() conversion for BGR3 format,
 * ensuring 3-channel formats are handled correctly.
 */
TEST_F(FrameUtilsTest, ConvertValidCVMatToOBSBGR3) {
    // Create test OpenCV Mat (BGR) with continuous memory
    // Note: Mat constructor takes (rows, cols) = (height, width)
    const int width = 640;
    const int height = 480;
    cv::Mat test_mat(height, width, CV_8UC3);
    cv::randu(test_mat, 0, 255);
    test_mat = test_mat.clone();  // Ensure continuous memory

    // Create reference OBS frame (for metadata)
    obs_source_frame ref_frame;
    memset(&ref_frame, 0, sizeof(ref_frame));
    ref_frame.width = width;
    ref_frame.height = height;
    ref_frame.format = VIDEO_FORMAT_BGR3;
    ref_frame.timestamp = 54321;
    ref_frame.full_range = 0;
    ref_frame.flip = 1;
    ref_frame.flags = 0x100;

    // Convert OpenCV Mat to OBS frame
    obs_source_frame* result = FRAME_UTILS::Conversion::cv_to_obs(test_mat, &ref_frame);

    // Verify successful conversion
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->width, 640);
    EXPECT_EQ(result->height, 480);
    EXPECT_EQ(result->format, VIDEO_FORMAT_BGR3);
    EXPECT_EQ(result->timestamp, 54321);
    EXPECT_EQ(result->full_range, 0);
    EXPECT_EQ(result->flip, 1);
    EXPECT_EQ(result->flags, 0x100);
    EXPECT_NE(result->data[0], nullptr);

    // Verify data was copied correctly
    // Use memcpy to create continuous Mat and avoid step issues
    cv::Mat converted(result->height, result->width, CV_8UC3);
    memcpy(converted.data, result->data[0],
           result->height * result->linesize[0]);
    double diff = cv::norm(test_mat, converted, cv::NORM_L2);
    EXPECT_NEAR(diff, 0.0, 1.0);  // Should be nearly identical

    // Clean up
    FRAME_UTILS::FrameBuffer::release(result);
}

/**
 * Test: Convert BGR OpenCV Mat to BGRA OBS frame
 * EXPECTED: Returns valid OBS frame with BGRA format (channels converted)
 *
 * RATIONALE: This test verifies that channel conversion works correctly when
 * the Mat format differs from the target OBS format.
 */
TEST_F(FrameUtilsTest, ConvertBGRMatToBGRAOBSFrame) {
    // Create test OpenCV Mat (BGR - 3 channels) with continuous memory
    // Note: Mat constructor takes (rows, cols) = (height, width)
    const int width = 320;
    const int height = 240;
    cv::Mat test_mat(height, width, CV_8UC3);
    cv::randu(test_mat, 0, 255);
    test_mat = test_mat.clone();  // Ensure continuous memory

    // Create reference OBS frame (BGRA - 4 channels)
    obs_source_frame ref_frame;
    memset(&ref_frame, 0, sizeof(ref_frame));
    ref_frame.width = width;
    ref_frame.height = height;
    ref_frame.format = VIDEO_FORMAT_BGRA;
    ref_frame.timestamp = 99999;

    // Convert OpenCV Mat to OBS frame
    obs_source_frame* result = FRAME_UTILS::Conversion::cv_to_obs(test_mat, &ref_frame);

    // Verify successful conversion with channel conversion
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->width, 320);
    EXPECT_EQ(result->height, 240);
    EXPECT_EQ(result->format, VIDEO_FORMAT_BGRA);
    EXPECT_EQ(result->timestamp, 99999);
    EXPECT_NE(result->data[0], nullptr);

    // Verify data is valid (BGR->BGRA conversion adds alpha channel)
    // Use memcpy to create continuous Mat and avoid step issues
    cv::Mat result_mat(result->height, result->width, CV_8UC4);
    memcpy(result_mat.data, result->data[0],
           result->height * result->linesize[0]);
    EXPECT_FALSE(result_mat.empty());

    // Clean up
    FRAME_UTILS::FrameBuffer::release(result);
}

/**
 * Test: Full round-trip conversion (OBS -> CV -> OBS)
 * EXPECTED: Original and final frames have similar data
 *
 * RATIONALE: This test verifies the complete conversion pipeline works correctly,
 * which is critical for the plugin's main functionality.
 */
TEST_F(FrameUtilsTest, FullRoundTripConversion) {
    // Create original OBS frame (BGRA) with continuous memory
    // Note: Mat constructor takes (rows, cols) = (height, width)
    const int width = 640;
    const int height = 480;
    cv::Mat original_data(height, width, CV_8UC4);
    cv::randu(original_data, 0, 255);
    original_data = original_data.clone();  // Ensure continuous memory

    obs_source_frame original_frame;
    memset(&original_frame, 0, sizeof(original_frame));
    original_frame.width = width;
    original_frame.height = height;
    original_frame.format = VIDEO_FORMAT_BGRA;
    original_frame.timestamp = 11111;
    original_frame.data[0] = original_data.data;
    // Calculate correct linesize: width * channels (4 for BGRA)
    original_frame.linesize[0] = 640 * 4;

    // Convert OBS -> CV
    cv::Mat cv_frame = FRAME_UTILS::Conversion::obs_to_cv(&original_frame);
    ASSERT_FALSE(cv_frame.empty());

    // Simulate some processing (e.g., stabilization would modify the frame)
    // For this test, we'll just make a copy
    cv::Mat processed_frame = cv_frame.clone();

    // Convert CV -> OBS
    obs_source_frame* final_frame = FRAME_UTILS::Conversion::cv_to_obs(processed_frame, &original_frame);
    ASSERT_NE(final_frame, nullptr);

    // Verify metadata is preserved
    EXPECT_EQ(final_frame->width, 640);
    EXPECT_EQ(final_frame->height, 480);
    EXPECT_EQ(final_frame->format, VIDEO_FORMAT_BGRA);
    EXPECT_EQ(final_frame->timestamp, 11111);

    // Verify data is similar (not exact due to clone, but should be close)
    // Use memcpy to create continuous Mat and avoid step issues
    cv::Mat final_mat(final_frame->height, final_frame->width, CV_8UC4);
    memcpy(final_mat.data, final_frame->data[0],
           final_frame->height * final_frame->linesize[0]);
    double diff = cv::norm(original_data, final_mat, cv::NORM_L2);
    EXPECT_NEAR(diff, 0.0, 1.0);  // Should be nearly identical

    // Clean up
    FRAME_UTILS::FrameBuffer::release(final_frame);
}

/**
 * Test: Convert NV12 frame with full round-trip
 * EXPECTED: Round-trip conversion works for NV12 format
 *
 * RATIONALE: This test verifies NV12 format works through the full conversion pipeline.
 */
TEST_F(FrameUtilsTest, FullRoundTripConversionNV12) {
    // Create original OBS frame (NV12)
    const int width = 640;
    const int height = 480;
    const int y_size = width * height;
    const int uv_size = (width * height) / 2;
    const int total_size = y_size + uv_size;

    std::vector<uint8_t> nv12_data(total_size);
    cv::randu(nv12_data, 0, 255);

    obs_source_frame original_frame;
    memset(&original_frame, 0, sizeof(original_frame));
    original_frame.width = width;
    original_frame.height = height;
    original_frame.format = VIDEO_FORMAT_NV12;
    original_frame.timestamp = 22222;
    original_frame.data[0] = nv12_data.data();
    original_frame.data[1] = nv12_data.data() + y_size;
    original_frame.linesize[0] = width;
    original_frame.linesize[1] = width;

    // Convert OBS (NV12) -> CV (BGRA)
    cv::Mat cv_frame = FRAME_UTILS::Conversion::obs_to_cv(&original_frame);
    ASSERT_FALSE(cv_frame.empty());
    EXPECT_EQ(cv_frame.channels(), 4);  // NV12 converts to BGRA

    // Convert CV -> OBS (BGRA)
    obs_source_frame* final_frame = FRAME_UTILS::Conversion::cv_to_obs(cv_frame, &original_frame);
    ASSERT_NE(final_frame, nullptr);

    // Verify metadata is preserved (format is copied from reference)
    EXPECT_EQ(final_frame->width, width);
    EXPECT_EQ(final_frame->height, height);
    EXPECT_EQ(final_frame->format, VIDEO_FORMAT_NV12);  // Format matches reference
    EXPECT_EQ(final_frame->timestamp, 22222);

    // Clean up
    FRAME_UTILS::FrameBuffer::release(final_frame);
}

/**
 * Test: Convert I420 frame with full round-trip
 * EXPECTED: Round-trip conversion works for I420 format
 *
 * RATIONALE: This test verifies I420 format works through the full conversion pipeline.
 */
TEST_F(FrameUtilsTest, FullRoundTripConversionI420) {
    // Create original OBS frame (I420)
    const int width = 640;
    const int height = 480;
    const int y_size = width * height;
    const int u_size = (width / 2) * (height / 2);
    const int v_size = (width / 2) * (height / 2);

    std::vector<uint8_t> y_data(y_size);
    std::vector<uint8_t> u_data(u_size);
    std::vector<uint8_t> v_data(v_size);
    cv::randu(y_data, 0, 255);
    cv::randu(u_data, 0, 255);
    cv::randu(v_data, 0, 255);

    obs_source_frame original_frame;
    memset(&original_frame, 0, sizeof(original_frame));
    original_frame.width = width;
    original_frame.height = height;
    original_frame.format = VIDEO_FORMAT_I420;
    original_frame.timestamp = 33333;
    original_frame.data[0] = y_data.data();
    original_frame.data[1] = u_data.data();
    original_frame.data[2] = v_data.data();
    original_frame.linesize[0] = width;
    original_frame.linesize[1] = width / 2;
    original_frame.linesize[2] = width / 2;

    // Convert OBS (I420) -> CV (BGRA)
    cv::Mat cv_frame = FRAME_UTILS::Conversion::obs_to_cv(&original_frame);
    ASSERT_FALSE(cv_frame.empty());
    EXPECT_EQ(cv_frame.channels(), 4);  // I420 converts to BGRA

    // Convert CV -> OBS (BGRA)
    obs_source_frame* final_frame = FRAME_UTILS::Conversion::cv_to_obs(cv_frame, &original_frame);
    ASSERT_NE(final_frame, nullptr);

    // Verify metadata is preserved (format is copied from reference)
    EXPECT_EQ(final_frame->width, width);
    EXPECT_EQ(final_frame->height, height);
    EXPECT_EQ(final_frame->format, VIDEO_FORMAT_I420);  // Format matches reference
    EXPECT_EQ(final_frame->timestamp, 33333);

    // Clean up
    FRAME_UTILS::FrameBuffer::release(final_frame);
}

/**
 * Test: Frame buffer creation with different sizes
 * EXPECTED: Correctly handles various frame sizes
 *
 * RATIONALE: This test verifies frame buffer creation works for different
 * resolutions, ensuring flexibility for various video sources.
 */
TEST_F(FrameUtilsTest, FrameBufferCreationDifferentSizes) {
    struct FrameSize {
        int width;
        int height;
    };

    std::vector<FrameSize> sizes = {
        {320, 240},    // QVGA
        {640, 480},    // VGA
        {1280, 720},   // HD 720p
        {1920, 1080}   // Full HD 1080p
    };

    for (const auto& size : sizes) {
        // Create test Mat
        cv::Mat test_mat(size.width, size.height, CV_8UC4);
        cv::randu(test_mat, 0, 255);

        // Create reference OBS frame
        obs_source_frame ref_frame;
        memset(&ref_frame, 0, sizeof(ref_frame));
        ref_frame.width = size.width;
        ref_frame.height = size.height;
        ref_frame.format = VIDEO_FORMAT_BGRA;

        // Convert OpenCV Mat to OBS frame
        obs_source_frame* result = FRAME_UTILS::Conversion::cv_to_obs(test_mat, &ref_frame);

        // Verify successful conversion
        ASSERT_NE(result, nullptr) << "Failed for size " << size.width << "x" << size.height;
        EXPECT_EQ(result->width, static_cast<uint32_t>(size.width));
        EXPECT_EQ(result->height, static_cast<uint32_t>(size.height));
        EXPECT_EQ(result->format, VIDEO_FORMAT_BGRA);
        EXPECT_NE(result->data[0], nullptr);

        // Clean up
        FRAME_UTILS::FrameBuffer::release(result);
    }
}

/**
 * Test: Frame buffer with custom timestamp
 * EXPECTED: Timestamp is correctly preserved in output frame
 *
 * RATIONALE: Timestamps are critical for OBS frame synchronization.
 * This test verifies they are properly copied from reference frame.
 */
TEST_F(FrameUtilsTest, FrameBufferPreservesTimestamp) {
    cv::Mat test_mat(640, 480, CV_8UC4);
    cv::randu(test_mat, 0, 255);

    // Test with various timestamps
    std::vector<uint64_t> timestamps = {
        0,
        1000,
        1234567890,
        std::numeric_limits<uint64_t>::max() / 2
    };

    for (auto timestamp : timestamps) {
        obs_source_frame ref_frame;
        memset(&ref_frame, 0, sizeof(ref_frame));
        ref_frame.width = 640;
        ref_frame.height = 480;
        ref_frame.format = VIDEO_FORMAT_BGRA;
        ref_frame.timestamp = timestamp;

        obs_source_frame* result = FRAME_UTILS::Conversion::cv_to_obs(test_mat, &ref_frame);

        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->timestamp, timestamp) << "Failed for timestamp " << timestamp;

        FRAME_UTILS::FrameBuffer::release(result);
    }
}

/**
 * Test: Frame buffer with full_range flag
 * EXPECTED: full_range flag is correctly preserved in output frame
 *
 * RATIONALE: Color range information is important for accurate color reproduction.
 * This test verifies it's properly copied from reference frame.
 */
TEST_F(FrameUtilsTest, FrameBufferPreservesColorRange) {
    cv::Mat test_mat(640, 480, CV_8UC4);
    cv::randu(test_mat, 0, 255);

    // Test with different full_range values
    for (int full_range = 0; full_range <= 1; ++full_range) {
        obs_source_frame ref_frame;
        memset(&ref_frame, 0, sizeof(ref_frame));
        ref_frame.width = 640;
        ref_frame.height = 480;
        ref_frame.format = VIDEO_FORMAT_BGRA;
        ref_frame.full_range = full_range;

        obs_source_frame* result = FRAME_UTILS::Conversion::cv_to_obs(test_mat, &ref_frame);

        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->full_range, full_range);

        FRAME_UTILS::FrameBuffer::release(result);
    }
}

/**
 * Test: Frame buffer with flip flag
 * EXPECTED: flip flag is correctly preserved in output frame
 *
 * RATIONALE: Flip flag is used for vertical flip handling.
 * This test verifies it's properly copied from reference frame.
 */
TEST_F(FrameUtilsTest, FrameBufferPreservesFlipFlag) {
    cv::Mat test_mat(640, 480, CV_8UC4);
    cv::randu(test_mat, 0, 255);

    // Test with different flip values
    std::vector<uint8_t> flip_values = {0, 1, 255};

    for (auto flip : flip_values) {
        obs_source_frame ref_frame;
        memset(&ref_frame, 0, sizeof(ref_frame));
        ref_frame.width = 640;
        ref_frame.height = 480;
        ref_frame.format = VIDEO_FORMAT_BGRA;
        ref_frame.flip = flip;

        obs_source_frame* result = FRAME_UTILS::Conversion::cv_to_obs(test_mat, &ref_frame);

        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->flip, flip);

        FRAME_UTILS::FrameBuffer::release(result);
    }
}

/**
 * Test: Frame buffer with custom flags
 * EXPECTED: flags field is correctly preserved in output frame
 *
 * RATIONALE: Flags field contains OBS-specific frame properties (keyframe, preroll, etc.).
 * This test verifies they're properly copied from reference frame.
 */
TEST_F(FrameUtilsTest, FrameBufferPreservesFlags) {
    cv::Mat test_mat(640, 480, CV_8UC4);
    cv::randu(test_mat, 0, 255);

    // Test with different flag values
    std::vector<uint32_t> flag_values = {0, 0x100, 0x200, 0xFFFFFFFF};

    for (auto flags : flag_values) {
        obs_source_frame ref_frame;
        memset(&ref_frame, 0, sizeof(ref_frame));
        ref_frame.width = 640;
        ref_frame.height = 480;
        ref_frame.format = VIDEO_FORMAT_BGRA;
        ref_frame.flags = flags;

        obs_source_frame* result = FRAME_UTILS::Conversion::cv_to_obs(test_mat, &ref_frame);

        ASSERT_NE(result, nullptr);
        EXPECT_EQ(result->flags, flags);

        FRAME_UTILS::FrameBuffer::release(result);
    }
}

/**
 * Test: Conversion tracking does not increment on success
 * EXPECTED: failed_conversions counter remains unchanged on successful conversions
 *
 * RATIONALE: This verifies that the failure tracking only counts actual failures,
 * not successful conversions.
 */
TEST_F(FrameUtilsTest, ConversionTrackingNoIncrementOnSuccess) {
    // Get initial stats
    auto initial_stats = FRAME_UTILS::Performance::get_stats();
    size_t initial_failures = initial_stats.failed_conversions;

    // Perform successful conversions
    cv::Mat test_mat(640, 480, CV_8UC4);
    cv::randu(test_mat, 0, 255);

    obs_source_frame ref_frame;
    memset(&ref_frame, 0, sizeof(ref_frame));
    ref_frame.width = 640;
    ref_frame.height = 480;
    ref_frame.format = VIDEO_FORMAT_BGRA;

    obs_source_frame* result = FRAME_UTILS::Conversion::cv_to_obs(test_mat, &ref_frame);
    ASSERT_NE(result, nullptr);

    // Verify no failures were tracked
    auto final_stats = FRAME_UTILS::Performance::get_stats();
    EXPECT_EQ(final_stats.failed_conversions, initial_failures);

    // Clean up
    FRAME_UTILS::FrameBuffer::release(result);
}

#endif // HAVE_OBS_HEADERS
