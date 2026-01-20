#include "test_data_generator.hpp"
#include <opencv2/opencv.hpp>
#include <cmath>
#include <random>
#include <sstream>

namespace TestDataGenerator {

cv::Mat generate_test_frame(int width, int height, int frame_type) {
    cv::Mat frame(height, width, CV_8UC4);
    cv::Scalar color;

    switch(frame_type) {
        case 0:
            // Solid color frame
            color = cv::Scalar(128, 128, 128, 255);
            break;
        case 1:
            // Gradient frame
            for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                    frame.at<cv::Vec4b>(y, x) = cv::Vec4b(
                        static_cast<uchar>(x * 255 / width),
                        static_cast<uchar>(y * 255 / height),
                        128,
                        255
                    );
                }
            }
            return frame;
        case 2:
            // Checkerboard pattern
            for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                    int checker = ((x / 50) + (y / 50)) % 2;
                    frame.at<cv::Vec4b>(y, x) = cv::Vec4b(
                        checker ? 255 : 0,
                        checker ? 0 : 255,
                        128,
                        255
                    );
                }
            }
            return frame;
        default:
            color = cv::Scalar(128, 128, 128, 255);
    }

    frame = color;
    return frame;
}

std::vector<cv::Mat> generate_test_sequence(int num_frames, int width, int height,
                                           const std::string& motion_pattern) {
    std::vector<cv::Mat> sequence;

    for(int i = 0; i < num_frames; i++) {
        cv::Mat frame = generate_test_frame(width, height, i % 3);

        if(motion_pattern == "horizontal") {
            float dx = i * 2.0f;
            frame = create_motion_frame(frame, dx, 0, 0);
        } else if(motion_pattern == "vertical") {
            float dy = i * 2.0f;
            frame = create_motion_frame(frame, 0, dy, 0);
        } else if(motion_pattern == "rotation") {
            float rotation = i * 2.0f;
            frame = create_motion_frame(frame, 0, 0, rotation);
        } else if(motion_pattern == "zoom") {
            float zoom = 1.0f + i * 0.05f;
            frame = create_motion_frame(frame, 0, 0, 0, zoom);
        }

        sequence.push_back(frame.clone());
    }

    return sequence;
}

cv::Mat generate_frame_in_format(int width, int height, int format) {
    cv::Mat mat(height, width, format);

    switch(format) {
        case CV_8UC1:
            mat = cv::Scalar(128);
            break;
        case CV_8UC3:
            mat = cv::Scalar(128, 128, 128);
            break;
        case CV_8UC4:
            mat = cv::Scalar(128, 128, 128, 255);
            break;
        case CV_16UC1:
            mat = cv::Scalar(128);
            break;
        case CV_32FC1:
            mat = cv::Scalar(128.0f);
            break;
        default:
            mat = cv::Scalar(128);
    }

    return mat;
}

cv::Mat create_motion_frame(const cv::Mat& base_frame, float dx, float dy, float rotation, float zoom) {
    if(base_frame.empty()) {
        return cv::Mat();
    }

    cv::Mat frame = base_frame.clone();

    if(dx != 0 || dy != 0) {
        cv::Mat M = cv::Mat::eye(2, 3, CV_32F);
        M.at<float>(0, 2) = dx;
        M.at<float>(1, 2) = dy;
        cv::warpAffine(frame, frame, M, frame.size());
    }

    if(rotation != 0) {
        cv::Point2f center(frame.cols / 2.0f, frame.rows / 2.0f);
        cv::Mat M = cv::getRotationMatrix2D(center, rotation, zoom);
        cv::warpAffine(frame, frame, M, frame.size());
    }

    return frame;
}

cv::Mat create_frame_with_features(int width, int height, int num_features) {
    cv::Mat frame(height, width, CV_8UC4, cv::Scalar(128, 128, 128, 255));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(10, width - 10);

    for(int i = 0; i < num_features; i++) {
        int x = dist(gen);
        int y = dist(gen);
        int size = 20;

        cv::rectangle(frame, cv::Point(x - size, y - size), cv::Point(x + size, y + size),
                     cv::Scalar(255, 0, 0, 255), -1);
        cv::circle(frame, cv::Point(x, y), size / 2, cv::Scalar(0, 0, 255, 255), -1);
    }

    return frame;
}

TestVideoData generate_comprehensive_test_data(int num_frames, int width, int height) {
    TestVideoData data;
    data.frames = generate_test_sequence(num_frames, width, height, "horizontal");
    data.width = width;
    data.height = height;
    data.format_name = "BGRA";

    return data;
}

cv::Mat generate_horizontal_motion_frame(const cv::Mat& base_frame, int frame_index, int total_frames) {
    float dx = (frame_index / static_cast<float>(total_frames)) * 100.0f;
    return create_motion_frame(base_frame, dx, 0, 0);
}

cv::Mat generate_vertical_motion_frame(const cv::Mat& base_frame, int frame_index, int total_frames) {
    float dy = (frame_index / static_cast<float>(total_frames)) * 100.0f;
    return create_motion_frame(base_frame, 0, dy, 0, 1.0f);
}

cv::Mat generate_rotation_frame(const cv::Mat& base_frame, int frame_index, int total_frames, float rotation_speed) {
    float rotation = (frame_index / static_cast<float>(total_frames)) * rotation_speed;
    return create_motion_frame(base_frame, 0, 0, rotation, 1.0f);
}

cv::Mat generate_zoom_frame(const cv::Mat& base_frame, int frame_index, int total_frames, float zoom_speed) {
    float zoom = 1.0f + (frame_index / static_cast<float>(total_frames)) * zoom_speed;
    return create_motion_frame(base_frame, 0, 0, 0, zoom);
}

} // namespace TestDataGenerator
