#include "test_data_generator.hpp"
#include <opencv2/opencv.hpp>

namespace TestDataGenerator {

cv::Mat generate_test_frame(int width, int height, int frame_type) {
    cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC4);
    
    if (frame_type == 0) {
        cv::rectangle(frame, cv::Rect(width/4, height/4, width/2, height/2), 
                     cv::Scalar(255, 0, 0, 255), -1);
    } else if (frame_type == 1) {
        cv::circle(frame, cv::Point(width/2, height/2), height/4, 
                  cv::Scalar(0, 255, 0, 255), -1);
    } else {
        cv::line(frame, cv::Point(0, 0), cv::Point(width, height), 
                cv::Scalar(0, 0, 255, 255), 3);
        cv::line(frame, cv::Point(width, 0), cv::Point(0, height), 
                cv::Scalar(255, 255, 0, 255), 3);
    }
    
    return frame;
}

std::vector<cv::Mat> generate_test_sequence(int num_frames, int width, int height,
                                           const std::string& motion_pattern) {
    std::vector<cv::Mat> frames;
    cv::Mat base_frame = generate_test_frame(width, height, 0);
    
    for (int i = 0; i < num_frames; i++) {
        if (motion_pattern == "static") {
            frames.push_back(base_frame.clone());
        } else if (motion_pattern == "horizontal") {
            frames.push_back(generate_horizontal_motion_frame(base_frame, i, num_frames));
        } else if (motion_pattern == "vertical") {
            frames.push_back(generate_vertical_motion_frame(base_frame, i, num_frames));
        } else if (motion_pattern == "rotation") {
            frames.push_back(generate_rotation_frame(base_frame, i, num_frames, 2.0f));
        } else if (motion_pattern == "zoom") {
            frames.push_back(generate_zoom_frame(base_frame, i, num_frames, 1.01f));
        } else {
            frames.push_back(base_frame.clone());
        }
    }
    
    return frames;
}

cv::Mat generate_frame_in_format(int width, int height, int format) {
    if (format == CV_8UC4) {
        return cv::Mat::zeros(height, width, CV_8UC4);
    } else if (format == CV_8UC3) {
        return cv::Mat::zeros(height, width, CV_8UC3);
    } else if (format == CV_8UC1) {
        return cv::Mat::zeros(height, width, CV_8UC1);
    }
    return cv::Mat::zeros(height, width, CV_8UC4);
}

cv::Mat create_motion_frame(const cv::Mat& base_frame, float dx, float dy, float rotation, float zoom) {
    cv::Mat result = base_frame.clone();
    
    if (dx != 0.0f || dy != 0.0f) {
        cv::Mat translation = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, dy);
        cv::warpAffine(result, result, translation, result.size());
    }
    
    if (rotation != 0.0f) {
        cv::Point2f center(result.cols/2.0f, result.rows/2.0f);
        cv::Mat rot_mat = cv::getRotationMatrix2D(center, rotation, 1.0);
        cv::warpAffine(result, result, rot_mat, result.size());
    }
    
    if (zoom != 1.0f) {
        cv::Mat zoom_mat = (cv::Mat_<double>(2, 3) << zoom, 0, 0, 0, zoom, 0);
        cv::warpAffine(result, result, zoom_mat, result.size());
    }
    
    return result;
}

cv::Mat create_frame_with_features(int width, int height, int num_features) {
    cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC4);
    
    for (int i = 0; i < num_features; i++) {
        int x = (i * 7) % width;
        int y = (i * 11) % height;
        cv::circle(frame, cv::Point(x, y), 3, cv::Scalar(255, 255, 255, 255), -1);
    }
    
    return frame;
}

TestDataGenerator::TestVideoData generate_comprehensive_test_data(int num_frames, int width, int height) {
    TestVideoData data;
    data.width = width;
    data.height = height;
    data.format_name = "BGRA";
    
    cv::Mat base_frame = generate_test_frame(width, height, 0);
    
    for (int i = 0; i < num_frames; i++) {
        if (i < num_frames / 5) {
            data.frames.push_back(base_frame.clone());
        } else if (i < 2 * num_frames / 5) {
            data.frames.push_back(generate_horizontal_motion_frame(base_frame, i, num_frames));
        } else if (i < 3 * num_frames / 5) {
            data.frames.push_back(generate_vertical_motion_frame(base_frame, i, num_frames));
        } else if (i < 4 * num_frames / 5) {
            data.frames.push_back(generate_rotation_frame(base_frame, i, num_frames, 1.5f));
        } else {
            data.frames.push_back(generate_zoom_frame(base_frame, i, num_frames, 1.005f));
        }
    }
    
    return data;
}

cv::Mat generate_horizontal_motion_frame(const cv::Mat& base_frame, int frame_index, int total_frames) {
    float max_dx = 20.0f;
    float dx = max_dx * sin(2.0 * M_PI * frame_index / total_frames);
    return create_motion_frame(base_frame, dx, 0.0f, 0.0f, 1.0f);
}

cv::Mat generate_vertical_motion_frame(const cv::Mat& base_frame, int frame_index, int total_frames) {
    float max_dy = 20.0f;
    float dy = max_dy * sin(2.0 * M_PI * frame_index / total_frames);
    return create_motion_frame(base_frame, 0.0f, dy, 0.0f, 1.0f);
}

cv::Mat generate_rotation_frame(const cv::Mat& base_frame, int frame_index, int total_frames, float rotation_speed) {
    float angle = rotation_speed * sin(2.0 * M_PI * frame_index / total_frames);
    return create_motion_frame(base_frame, 0.0f, 0.0f, angle, 1.0f);
}

cv::Mat generate_zoom_frame(const cv::Mat& base_frame, int frame_index, int total_frames, float zoom_speed) {
    float zoom = 1.0f + 0.05 * sin(2.0 * M_PI * frame_index / total_frames);
    return create_motion_frame(base_frame, 0.0f, 0.0f, 0.0f, zoom);
}

} // namespace TestDataGenerator
