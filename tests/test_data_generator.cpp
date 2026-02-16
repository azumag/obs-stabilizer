#define _USE_MATH_DEFINES
#include <cmath>
#include "test_data_generator.hpp"
#include <opencv2/opencv.hpp>

namespace TestDataGenerator {

cv::Mat generate_test_frame(int width, int height, int frame_type) {
    cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC4);

    if (frame_type == 0) {
        // Type 0: Rectangle with grid pattern for better corner detection
        cv::rectangle(frame, cv::Rect(width/4, height/4, width/2, height/2),
                     cv::Scalar(128, 128, 128, 255), -1);

        // Add grid lines for more detectable corners
        int grid_size = width / 20;
        for (int x = width/4; x <= 3*width/4; x += grid_size) {
            cv::line(frame, cv::Point(x, height/4), cv::Point(x, 3*height/4),
                    cv::Scalar(200, 200, 200, 255), 1);
        }
        for (int y = height/4; y <= 3*height/4; y += grid_size) {
            cv::line(frame, cv::Point(width/4, y), cv::Point(3*width/4, y),
                    cv::Scalar(200, 200, 200, 255), 1);
        }
    } else if (frame_type == 1) {
        // Type 1: Circle with radial pattern
        cv::circle(frame, cv::Point(width/2, height/2), height/4,
                  cv::Scalar(128, 128, 128, 255), -1);

        // Add radial lines for more corners
        for (int angle = 0; angle < 360; angle += 45) {
            double rad = angle * M_PI / 180.0;
            int end_x = width/2 + (height/4 - 10) * cos(rad);
            int end_y = height/2 + (height/4 - 10) * sin(rad);
            cv::line(frame, cv::Point(width/2, height/2), cv::Point(end_x, end_y),
                    cv::Scalar(200, 200, 200, 255), 2);
        }
    } else {
        // Type 2: Checkerboard pattern - excellent for corner detection
        int block_size = width / 16;
        for (int y = 0; y < height / block_size; y++) {
            for (int x = 0; x < width / block_size; x++) {
                cv::Scalar color = ((x + y) % 2 == 0) ?
                    cv::Scalar(255, 255, 255, 255) : cv::Scalar(0, 0, 0, 255);
                cv::rectangle(frame,
                    cv::Rect(x * block_size, y * block_size, block_size, block_size),
                    color, -1);
            }
        }
    }

    return frame;
}

std::vector<cv::Mat> generate_test_sequence(int num_frames, int width, int height,
                                           const std::string& motion_pattern) {
    // Use fixed seed for deterministic test data generation
    // This ensures tests are repeatable and not flaky due to random data
    srand(42);

    std::vector<cv::Mat> frames;
    cv::Mat base_frame = generate_test_frame(width, height, 0);

    for (int i = 0; i < num_frames; i++) {
        if (motion_pattern == "static") {
            frames.push_back(base_frame.clone());
        } else if (motion_pattern == "horizontal" || motion_pattern == "shake") {
            // Shake uses horizontal motion pattern for testing
            frames.push_back(generate_horizontal_motion_frame(base_frame, i, num_frames));
        } else if (motion_pattern == "vertical") {
            frames.push_back(generate_vertical_motion_frame(base_frame, i, num_frames));
        } else if (motion_pattern == "rotation") {
            frames.push_back(generate_rotation_frame(base_frame, i, num_frames, 2.0f));
        } else if (motion_pattern == "zoom" || motion_pattern == "zoom_in") {
            frames.push_back(generate_zoom_frame(base_frame, i, num_frames, 1.01f));
        } else if (motion_pattern == "pan_right") {
            // Pan uses horizontal motion with different speed
            frames.push_back(generate_horizontal_motion_frame(base_frame, i, num_frames));
        } else if (motion_pattern == "fast") {
            // Fast motion - larger displacement
            float dx = (rand() % 40 - 20) / 2.0f;
            float dy = (rand() % 40 - 20) / 2.0f;
            frames.push_back(create_motion_frame(base_frame, dx, dy, 0.0f, 1.0f));
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

cv::Mat generate_test_frame_with_borders(int width, int height, int border_pixels) {
    cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC4);

    // Add content in center, leaving borders black
    int content_x = border_pixels;
    int content_y = border_pixels;
    int content_w = width - 2 * border_pixels;
    int content_h = height - 2 * border_pixels;

    // Ensure content area is non-negative
    if (content_w > 0 && content_h > 0) {
        cv::rectangle(frame, cv::Rect(content_x, content_y, content_w, content_h),
                      cv::Scalar(128, 128, 128, 255), -1);
    }

    return frame;
}

/**
 * Generate a realistic scene with multiple objects, textures, and features
 * Creates an indoor scene with furniture, windows, and textural patterns
 * Provides rich corners and edges for feature tracking
 */
cv::Mat generate_realistic_frame(int width, int height, int scene_variant) {
    cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC4);

    // Base wall color
    cv::rectangle(frame, cv::Rect(0, 0, width, height), cv::Scalar(220, 220, 230, 255), -1);

    // Add floor
    int floor_y = height * 3 / 4;
    cv::rectangle(frame, cv::Rect(0, floor_y, width, height - floor_y),
                 cv::Scalar(180, 160, 140, 255), -1);

    // Add floor planks (lines) for more corners
    int plank_height = 20;
    for (int y = floor_y; y < height; y += plank_height) {
        cv::line(frame, cv::Point(0, y), cv::Point(width, y),
                cv::Scalar(160, 140, 120, 255), 1);
    }

    // Add window
    int window_width = width / 4;
    int window_height = height / 4;
    int window_x = width / 3;
    int window_y = height / 4;
    cv::rectangle(frame, cv::Rect(window_x, window_y, window_width, window_height),
                 cv::Scalar(135, 206, 250, 255), -1);
    // Window frame (cross)
    cv::line(frame, cv::Point(window_x + window_width/2, window_y),
            cv::Point(window_x + window_width/2, window_y + window_height),
            cv::Scalar(255, 255, 255, 255), 3);
    cv::line(frame, cv::Point(window_x, window_y + window_height/2),
            cv::Point(window_x + window_width, window_y + window_height/2),
            cv::Scalar(255, 255, 255, 255), 3);

    // Add furniture based on variant
    if (scene_variant == 0) {
        // Table with chairs
        int table_x = width / 2;
        int table_y = floor_y - 50;
        cv::rectangle(frame, cv::Rect(table_x - 100, table_y, 200, 80),
                     cv::Scalar(139, 69, 19, 255), -1);
        // Table legs and surface details
        cv::rectangle(frame, cv::Rect(table_x - 90, table_y + 10, 180, 60),
                     cv::Scalar(160, 82, 45, 255), -1);
    } else if (scene_variant == 1) {
        // Bookshelf
        int shelf_x = width * 2 / 3;
        int shelf_y = floor_y - 250;
        cv::rectangle(frame, cv::Rect(shelf_x, shelf_y, 150, 250),
                     cv::Scalar(101, 67, 33, 255), -1);
        // Shelves
        for (int i = 1; i < 5; i++) {
            int y = shelf_y + i * 50;
            cv::line(frame, cv::Point(shelf_x, y), cv::Point(shelf_x + 150, y),
                    cv::Scalar(80, 50, 20, 255), 3);
        }
        // Books
        for (int row = 0; row < 4; row++) {
            int book_y = shelf_y + row * 50 + 5;
            int book_x = shelf_x + 5;
            int book_width;
            while (book_x < shelf_x + 140) {
                book_width = 10 + (rand() % 20);
                cv::Scalar book_color(rand() % 100 + 50, rand() % 100 + 50, rand() % 100 + 100, 255);
                cv::rectangle(frame, cv::Rect(book_x, book_y, book_width, 40),
                             book_color, -1);
                book_x += book_width + 2;
            }
        }
    } else {
        // Geometric art on wall
        int art_x = width / 6;
        int art_y = height / 4;
        int art_size = 150;
        cv::rectangle(frame, cv::Rect(art_x, art_y, art_size, art_size),
                     cv::Scalar(240, 240, 240, 255), -1);
        // Add multiple shapes for corners
        cv::circle(frame, cv::Point(art_x + art_size/2, art_y + art_size/2),
                  art_size/4, cv::Scalar(255, 100, 100, 255), -1);
        cv::rectangle(frame, cv::Rect(art_x + 20, art_y + 20, art_size/3, art_size/3),
                     cv::Scalar(100, 255, 100, 255), -1);
    }

    // Add picture frame on wall
    int frame_x = width / 6;
    int frame_y = height / 4;
    if (scene_variant != 2) {
        cv::rectangle(frame, cv::Rect(frame_x, frame_y, 120, 90),
                     cv::Scalar(255, 250, 240, 255), -1);
        cv::rectangle(frame, cv::Rect(frame_x - 5, frame_y - 5, 130, 100),
                     cv::Scalar(139, 69, 19, 255), 5);
    }

    // Add some random small objects for more features
    for (int i = 0; i < 10; i++) {
        int obj_x = rand() % width;
        int obj_y = rand() % (height - floor_y) + floor_y;
        int size = 5 + rand() % 10;
        cv::Scalar obj_color(rand() % 256, rand() % 256, rand() % 256, 255);
        cv::circle(frame, cv::Point(obj_x, obj_y), size, obj_color, -1);
    }

    // Add noise for realism
    cv::Mat noise(frame.size(), frame.type());
    cv::randu(noise, cv::Scalar(0, 0, 0, 0), cv::Scalar(10, 10, 10, 0));
    cv::add(frame, noise, frame);

    return frame;
}

/**
 * Generate frames with strong corner features
 * Creates a checkerboard pattern with additional geometric shapes
 * Excellent for testing feature detection algorithms
 */
cv::Mat generate_frame_with_corners(int width, int height, int complexity) {
    cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC4);

    // Checkerboard pattern
    int base_block_size = width / 20;
    for (int y = 0; y < height / base_block_size; y++) {
        for (int x = 0; x < width / base_block_size; x++) {
            cv::Scalar color = ((x + y) % 2 == 0) ?
                cv::Scalar(200, 200, 200, 255) : cv::Scalar(50, 50, 50, 255);
            cv::rectangle(frame,
                cv::Rect(x * base_block_size, y * base_block_size,
                        base_block_size, base_block_size),
                color, -1);
        }
    }

    // Add additional corner-generating shapes based on complexity
    if (complexity > 0) {
        // Add rectangles at various positions and sizes
        for (int i = 0; i < 5 + complexity * 2; i++) {
            int x = rand() % (width - 100);
            int y = rand() % (height - 100);
            int w = 20 + rand() % 60;
            int h = 20 + rand() % 60;
            cv::Scalar rect_color(rand() % 256, rand() % 256, rand() % 256, 255);
            cv::rectangle(frame, cv::Rect(x, y, w, h), rect_color, 2);
        }

        // Add crosses (excellent corner sources)
        for (int i = 0; i < 3 + complexity; i++) {
            int cx = rand() % width;
            int cy = rand() % height;
            int size = 10 + rand() % 20;
            cv::Scalar cross_color(255, 255, 0, 255);
            cv::line(frame, cv::Point(cx - size, cy), cv::Point(cx + size, cy),
                    cross_color, 3);
            cv::line(frame, cv::Point(cx, cy - size), cv::Point(cx, cy + size),
                    cross_color, 3);
        }
    }

    if (complexity > 1) {
        // Add triangles (corners)
        for (int i = 0; i < 5; i++) {
            cv::Point pts[3];
            pts[0] = cv::Point(rand() % width, rand() % height);
            pts[1] = cv::Point(rand() % width, rand() % height);
            pts[2] = cv::Point(rand() % width, rand() % height);
            cv::Scalar tri_color(rand() % 256, rand() % 256, rand() % 256, 255);
            cv::fillConvexPoly(frame, pts, 3, tri_color);
        }

        // Add grid lines
        int grid_spacing = 40;
        for (int x = 0; x < width; x += grid_spacing) {
            cv::line(frame, cv::Point(x, 0), cv::Point(x, height),
                    cv::Scalar(100, 100, 100, 255), 1);
        }
        for (int y = 0; y < height; y += grid_spacing) {
            cv::line(frame, cv::Point(0, y), cv::Point(width, y),
                    cv::Scalar(100, 100, 100, 255), 1);
        }
    }

    return frame;
}

} // namespace TestDataGenerator
