#ifndef TEST_DATA_GENERATOR_HPP
#define TEST_DATA_GENERATOR_HPP

#include <opencv2/opencv.hpp>
#include <vector>

namespace TestDataGenerator {

// Generate a simple test frame with known features
cv::Mat generate_test_frame(int width, int height, int frame_type = 0);

// Generate a sequence of frames for testing motion patterns
std::vector<cv::Mat> generate_test_sequence(int num_frames, int width, int height,
                                           const std::string& motion_pattern = "static");

// Generate frame in different video formats
cv::Mat generate_frame_in_format(int width, int height, int format = CV_8UC4);

// Create frame with specific motion pattern
cv::Mat create_motion_frame(const cv::Mat& base_frame, float dx, float dy, float rotation, float zoom = 1.0f);

// Create frame with known feature locations
cv::Mat create_frame_with_features(int width, int height, int num_features = 100);

// Generate synthetic video data for testing
struct TestVideoData {
    std::vector<cv::Mat> frames;
    int width;
    int height;
    std::string format_name;
};

// Generate comprehensive test video data
TestVideoData generate_comprehensive_test_data(int num_frames = 30, int width = 640, int height = 480);

// Generate frame with horizontal motion
cv::Mat generate_horizontal_motion_frame(const cv::Mat& base_frame, int frame_index, int total_frames);

// Generate frame with vertical motion
cv::Mat generate_vertical_motion_frame(const cv::Mat& base_frame, int frame_index, int total_frames);

// Generate frame with rotation
cv::Mat generate_rotation_frame(const cv::Mat& base_frame, int frame_index, int total_frames, float rotation_speed);

// Generate frame with zoom
cv::Mat generate_zoom_frame(const cv::Mat& base_frame, int frame_index, int total_frames, float zoom_speed);

// Generate frame with programmable black borders for testing edge handling modes
cv::Mat generate_test_frame_with_borders(int width, int height, int border_pixels);

// Generate a realistic scene with multiple objects, textures, and features
cv::Mat generate_realistic_frame(int width, int height, int scene_variant = 0);

// Generate frames with strong corner features for feature detection testing
cv::Mat generate_frame_with_corners(int width, int height, int complexity = 1);

} // namespace TestDataGenerator

#endif // TEST_DATA_GENERATOR_HPP
