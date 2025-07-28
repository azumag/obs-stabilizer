/*
Performance Test Prototype for OBS Stabilizer Plugin
Tests the stabilization algorithms against real-time requirements
*/

#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/video/tracking.hpp>
#include <chrono>
#include <vector>
#include <iostream>
#include <iomanip>

class StabilizationProfiler {
private:
    cv::Mat prev_frame;
    cv::Mat working_gray;
    std::vector<cv::Point2f> prev_points;
    cv::Mat accumulated_transform;
    cv::Mat smoothed_transform;
    std::vector<cv::Mat> transform_history;
    bool first_frame;
    int smoothing_radius;
    int max_features;
    
    // Performance metrics
    std::vector<double> processing_times;
    size_t total_frames;
    
public:
    StabilizationProfiler(int smoothing = 30, int features = 200) 
        : first_frame(true), smoothing_radius(smoothing), max_features(features), total_frames(0) {
        accumulated_transform = cv::Mat::eye(2, 3, CV_64F);
        smoothed_transform = cv::Mat::eye(2, 3, CV_64F);
    }
    
    // Process a single frame and return processing time in milliseconds
    double processFrame(const cv::Mat& frame) {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // Convert to grayscale if needed
            if (frame.channels() == 3) {
                cv::cvtColor(frame, working_gray, cv::COLOR_BGR2GRAY);
            } else {
                frame.copyTo(working_gray);
            }
            
            if (first_frame) {
                working_gray.copyTo(prev_frame);
                
                if (working_gray.rows < 50 || working_gray.cols < 50) {
                    return 0.0; // Skip if too small
                }
                
                cv::goodFeaturesToTrack(prev_frame, prev_points, 
                                      std::max(50, std::min(max_features, 1000)), 0.01, 10);
                
                first_frame = false;
                total_frames++;
                
                auto end = std::chrono::high_resolution_clock::now();
                double duration = std::chrono::duration<double, std::milli>(end - start).count();
                processing_times.push_back(duration);
                return duration;
            }
            
            // Track feature points
            std::vector<cv::Point2f> current_points;
            std::vector<uchar> status;
            std::vector<float> errors;
            
            if (!prev_points.empty()) {
                cv::calcOpticalFlowPyrLK(prev_frame, working_gray, 
                                    prev_points, current_points, status, errors);
                
                // Filter good points
                std::vector<cv::Point2f> good_prev, good_current;
                const float max_error = 30.0f;
                
                for (size_t i = 0; i < status.size() && i < errors.size(); i++) {
                    if (status[i] && errors[i] < max_error) {
                        if (current_points[i].x >= 0 && current_points[i].x < working_gray.cols &&
                            current_points[i].y >= 0 && current_points[i].y < working_gray.rows) {
                            good_prev.push_back(prev_points[i]);
                            good_current.push_back(current_points[i]);
                        }
                    }
                }
                
                // Estimate transformation
                const size_t min_points = 6;
                if (good_prev.size() >= min_points) {
                    cv::Mat transform = cv::estimateAffinePartial2D(good_current, good_prev, 
                                                                  cv::noArray(), cv::RANSAC, 3.0);
                    
                    if (!transform.empty()) {
                        accumulated_transform = transform * accumulated_transform;
                        transform_history.push_back(transform.clone());
                        
                        size_t max_history = static_cast<size_t>(smoothing_radius);
                        if (transform_history.size() > max_history) {
                            transform_history.erase(transform_history.begin());
                        }
                        
                        // Calculate smoothed transform
                        cv::Mat smoothed = cv::Mat::zeros(2, 3, CV_64F);
                        for (const auto& hist_transform : transform_history) {
                            smoothed += hist_transform;
                        }
                        smoothed /= static_cast<double>(transform_history.size());
                        smoothed_transform = smoothed;
                        
                        // Simulate frame transformation (just timing, no actual transform)
                        cv::Mat dummy_output;
                        cv::warpAffine(working_gray, dummy_output, smoothed_transform,
                                     cv::Size(working_gray.cols, working_gray.rows),
                                     cv::INTER_LINEAR);
                    }
                }
                
                prev_points = good_current;
            }
            
            // Update previous frame
            working_gray.copyTo(prev_frame);
            
            // Refresh feature points if needed
            const size_t refresh_threshold = std::max(static_cast<size_t>(25), 
                                                    static_cast<size_t>(max_features / 3));
            if (prev_points.size() < refresh_threshold) {
                std::vector<cv::Point2f> new_points;
                cv::goodFeaturesToTrack(working_gray, new_points, 
                                      std::max(50, std::min(max_features, 1000)), 0.01, 10);
                prev_points.insert(prev_points.end(), new_points.begin(), new_points.end());
            }
            
        } catch (const cv::Exception& e) {
            std::cerr << "OpenCV error: " << e.what() << std::endl;
            first_frame = true;
            prev_points.clear();
        }
        
        total_frames++;
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        processing_times.push_back(duration);
        return duration;
    }
    
    void printStatistics() {
        if (processing_times.empty()) {
            std::cout << "No frames processed!" << std::endl;
            return;
        }
        
        double sum = 0.0;
        double min_time = processing_times[0];
        double max_time = processing_times[0];
        
        for (double time : processing_times) {
            sum += time;
            min_time = std::min(min_time, time);
            max_time = std::max(max_time, time);
        }
        
        double avg_time = sum / processing_times.size();
        double avg_fps = 1000.0 / avg_time;
        
        std::cout << "\n=== Performance Statistics ===" << std::endl;
        std::cout << "Total frames processed: " << total_frames << std::endl;
        std::cout << "Average processing time: " << std::fixed << std::setprecision(2) 
                  << avg_time << " ms" << std::endl;
        std::cout << "Min processing time: " << min_time << " ms" << std::endl;
        std::cout << "Max processing time: " << max_time << " ms" << std::endl;
        std::cout << "Average FPS capacity: " << std::setprecision(1) << avg_fps << " fps" << std::endl;
        
        // Performance targets
        std::cout << "\n=== Target Analysis ===" << std::endl;
        std::cout << "30 FPS target (33.3ms): " << (avg_time <= 33.3 ? "✓ PASS" : "✗ FAIL") << std::endl;
        std::cout << "60 FPS target (16.7ms): " << (avg_time <= 16.7 ? "✓ PASS" : "✗ FAIL") << std::endl;
        
        // Show frame time distribution
        std::cout << "\n=== Frame Time Distribution ===" << std::endl;
        int under_16ms = 0, under_33ms = 0, over_33ms = 0;
        for (double time : processing_times) {
            if (time <= 16.7) under_16ms++;
            else if (time <= 33.3) under_33ms++;
            else over_33ms++;
        }
        
        std::cout << "Frames ≤ 16.7ms (60fps): " << under_16ms << " (" 
                  << (100.0 * under_16ms / processing_times.size()) << "%)" << std::endl;
        std::cout << "Frames ≤ 33.3ms (30fps): " << (under_16ms + under_33ms) << " (" 
                  << (100.0 * (under_16ms + under_33ms) / processing_times.size()) << "%)" << std::endl;
        std::cout << "Frames > 33.3ms: " << over_33ms << " (" 
                  << (100.0 * over_33ms / processing_times.size()) << "%)" << std::endl;
    }
};

// Test with different resolutions and configurations
void runPerformanceTests() {
    std::vector<cv::Size> resolutions = {
        cv::Size(640, 480),   // VGA
        cv::Size(1280, 720),  // HD
        cv::Size(1920, 1080), // Full HD
        cv::Size(2560, 1440), // QHD
    };
    
    std::vector<std::pair<int, int>> configs = {
        {30, 100},  // Low settings
        {30, 200},  // Default settings  
        {50, 300},  // High settings
    };
    
    for (const auto& resolution : resolutions) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "Testing Resolution: " << resolution.width << "x" << resolution.height << std::endl;
        
        for (const auto& config : configs) {
            int smoothing = config.first;
            int features = config.second;
            
            std::cout << "\nConfiguration: Smoothing=" << smoothing 
                     << ", Features=" << features << std::endl;
            
            StabilizationProfiler profiler(smoothing, features);
            
            // Generate test frames (synthetic data)
            const int test_frames = 100;
            for (int i = 0; i < test_frames; i++) {
                // Create synthetic noisy frame
                cv::Mat frame = cv::Mat::zeros(resolution, CV_8UC1);
                cv::randu(frame, 0, 255);
                
                // Add some structure for feature detection
                for (int y = 20; y < resolution.height - 20; y += 40) {
                    for (int x = 20; x < resolution.width - 20; x += 40) {
                        cv::circle(frame, cv::Point(x + (i % 10), y + (i % 8)), 3, cv::Scalar(255), -1);
                    }
                }
                
                double time = profiler.processFrame(frame);
                
                // Show progress for long tests
                if (i % 25 == 0) {
                    std::cout << "." << std::flush;
                }
            }
            
            std::cout << std::endl;
            profiler.printStatistics();
        }
    }
}

int main() {
    std::cout << "OBS Stabilizer Performance Test" << std::endl;
    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
    std::cout << "Testing stabilization performance against real-time requirements..." << std::endl;
    
    runPerformanceTests();
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Performance testing completed!" << std::endl;
    std::cout << "Results show processing times for each configuration." << std::endl;
    std::cout << "Target: ≤33.3ms per frame for 30fps real-time processing" << std::endl;
    
    return 0;
}