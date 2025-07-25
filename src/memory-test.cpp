/*
Memory Usage Test for OBS Stabilizer Plugin
Verifies that memory usage remains stable during long-term operation
*/

#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/video/tracking.hpp>
#include <chrono>
#include <vector>
#include <iostream>
#include <iomanip>
#include <thread>

#ifdef __APPLE__
#include <mach/mach.h>
#elif __linux__
#include <fstream>
#include <sstream>
#elif _WIN32
#include <windows.h>
#include <psapi.h>
#endif

class MemoryMonitor {
public:
    static size_t getCurrentMemoryUsage() {
#ifdef __APPLE__
        struct mach_task_basic_info info;
        mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                     (task_info_t)&info, &infoCount) != KERN_SUCCESS) {
            return 0;
        }
        return info.resident_size;
#elif __linux__
        std::ifstream status_file("/proc/self/status");
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::istringstream iss(line);
                std::string dummy;
                size_t memory_kb;
                iss >> dummy >> memory_kb;
                return memory_kb * 1024; // Convert to bytes
            }
        }
        return 0;
#elif _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize;
        }
        return 0;
#else
        return 0; // Unsupported platform
#endif
    }
    
    static void printMemoryUsage(const std::string& label, size_t memory_bytes) {
        double memory_mb = memory_bytes / (1024.0 * 1024.0);
        std::cout << label << ": " << std::fixed << std::setprecision(2) 
                  << memory_mb << " MB" << std::endl;
    }
};

class StabilizationMemoryTest {
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
    
public:
    StabilizationMemoryTest(int smoothing = 30, int features = 200) 
        : first_frame(true), smoothing_radius(smoothing), max_features(features) {
        accumulated_transform = cv::Mat::eye(2, 3, CV_64F);
        smoothed_transform = cv::Mat::eye(2, 3, CV_64F);
    }
    
    void processFrame(const cv::Mat& frame) {
        try {
            // Convert to grayscale if needed
            if (frame.channels() == 3) {
                cv::cvtColor(frame, working_gray, cv::COLOR_BGR2GRAY);
            } else {
                frame.copyTo(working_gray);
            }
            
            if (first_frame) {
                working_gray.copyTo(prev_frame);
                
                if (working_gray.rows >= 50 && working_gray.cols >= 50) {
                    cv::goodFeaturesToTrack(prev_frame, prev_points, 
                                          std::max(50, std::min(max_features, 1000)), 0.01, 10);
                }
                
                first_frame = false;
                return;
            }
            
            // Track feature points
            std::vector<cv::Point2f> current_points;
            std::vector<uchar> status;
            std::vector<float> errors;
            
            if (!prev_points.empty()) {
                cv::calcOpticalFlowLK(prev_frame, working_gray, 
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
                if (good_prev.size() >= 6) {
                    cv::Mat transform = cv::estimateAffinePartial2D(good_current, good_prev, 
                                                                  cv::noArray(), cv::RANSAC, 3.0);
                    
                    if (!transform.empty()) {
                        accumulated_transform = transform * accumulated_transform;
                        transform_history.push_back(transform.clone());
                        
                        // Maintain fixed history size to prevent memory growth
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
                        
                        // Apply transformation (simulate)
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
    }
    
    void printCurrentState() {
        std::cout << "Transform history size: " << transform_history.size() << std::endl;
        std::cout << "Feature points: " << prev_points.size() << std::endl;
        std::cout << "Frame size: " << working_gray.cols << "x" << working_gray.rows << std::endl;
    }
};

void runMemoryTest() {
    const cv::Size test_resolution(1920, 1080); // Full HD test
    const int test_duration_minutes = 2; // Shorter test for practical use
    const int frames_per_second = 30;
    const int total_frames = test_duration_minutes * 60 * frames_per_second;
    const int memory_check_interval = frames_per_second * 5; // Every 5 seconds
    
    std::cout << "=== Memory Stability Test ===" << std::endl;
    std::cout << "Resolution: " << test_resolution.width << "x" << test_resolution.height << std::endl;
    std::cout << "Duration: " << test_duration_minutes << " minutes (" << total_frames << " frames)" << std::endl;
    std::cout << "Checking for memory leaks and growth..." << std::endl;
    
    StabilizationMemoryTest stabilizer;
    
    std::vector<size_t> memory_samples;
    size_t initial_memory = MemoryMonitor::getCurrentMemoryUsage();
    MemoryMonitor::printMemoryUsage("Initial memory", initial_memory);
    memory_samples.push_back(initial_memory);
    
    for (int frame = 0; frame < total_frames; frame++) {
        // Generate test frame
        cv::Mat test_frame = cv::Mat::zeros(test_resolution, CV_8UC1);
        cv::randu(test_frame, 0, 255);
        
        // Add some features for tracking
        for (int y = 20; y < test_resolution.height - 20; y += 60) {
            for (int x = 20; x < test_resolution.width - 20; x += 60) {
                int offset_x = (frame % 20) - 10; // Simulate camera shake
                int offset_y = (frame % 15) - 7;
                cv::circle(test_frame, cv::Point(x + offset_x, y + offset_y), 3, cv::Scalar(255), -1);
            }
        }
        
        stabilizer.processFrame(test_frame);
        
        // Check memory usage periodically
        if (frame % memory_check_interval == 0) {
            size_t current_memory = MemoryMonitor::getCurrentMemoryUsage();
            memory_samples.push_back(current_memory);
            
            double elapsed_seconds = frame / static_cast<double>(frames_per_second);
            std::cout << "Time: " << std::setw(3) << static_cast<int>(elapsed_seconds) << "s - ";
            MemoryMonitor::printMemoryUsage("Memory", current_memory);
            
            // Show progress
            int progress = (frame * 100) / total_frames;
            std::cout << "Progress: " << progress << "%" << std::endl;
        }
        
        // Simulate real-time processing delay
        if (frame % 100 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    size_t final_memory = MemoryMonitor::getCurrentMemoryUsage();
    MemoryMonitor::printMemoryUsage("Final memory", final_memory);
    
    // Analyze memory growth
    std::cout << "\n=== Memory Analysis ===" << std::endl;
    
    if (memory_samples.size() >= 2) {
        size_t memory_growth = final_memory - initial_memory;
        double growth_mb = memory_growth / (1024.0 * 1024.0);
        double growth_rate = (static_cast<double>(final_memory) / initial_memory - 1.0) * 100.0;
        
        std::cout << "Memory growth: " << std::fixed << std::setprecision(2) 
                  << growth_mb << " MB (" << growth_rate << "%)" << std::endl;
        
        // Check for memory leaks
        const double acceptable_growth_mb = 50.0; // 50MB growth considered acceptable
        if (std::abs(growth_mb) <= acceptable_growth_mb) {
            std::cout << "Memory stability: ✓ PASS (growth within acceptable limits)" << std::endl;
        } else {
            std::cout << "Memory stability: ✗ FAIL (excessive memory growth detected)" << std::endl;
        }
        
        // Show memory trend
        std::cout << "\nMemory usage over time:" << std::endl;
        for (size_t i = 0; i < memory_samples.size(); i++) {
            double time_point = i * memory_check_interval / static_cast<double>(frames_per_second);
            double memory_mb = memory_samples[i] / (1024.0 * 1024.0);
            std::cout << "  " << std::setw(3) << static_cast<int>(time_point) << "s: " 
                     << std::setprecision(1) << memory_mb << " MB" << std::endl;
        }
    }
    
    std::cout << "\nFinal state:" << std::endl;
    stabilizer.printCurrentState();
}

int main() {
    std::cout << "OBS Stabilizer Memory Test" << std::endl;
    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
    std::cout << "Testing memory stability during extended operation..." << std::endl;
    
    runMemoryTest();
    
    std::cout << "\n=== Memory Test Complete ===" << std::endl;
    std::cout << "This test verifies that the stabilizer doesn't leak memory" << std::endl;
    std::cout << "during extended operation typical of streaming sessions." << std::endl;
    
    return 0;
}