#ifndef STABILIZER_WRAPPER_HPP
#define STABILIZER_WRAPPER_HPP

#include "core/stabilizer_core.hpp"
#include <memory>
#include <mutex>

/**
 * @brief RAII wrapper for StabilizerCore with thread-safe memory management
 * 
 * This class addresses the critical memory safety issues by providing:
 * - Automatic cleanup via RAII pattern
 * - Thread-safe access through mutex protection
 * - Exception-safe boundaries for OBS callbacks
 * - Safe initialization and error handling
 */
class StabilizerWrapper {
private:
    std::unique_ptr<StabilizerCore> stabilizer;
    std::mutex mutex;
    
public:
    /**
     * @brief Constructor - automatically initializes resources
     */
    StabilizerWrapper() = default;
    
    /**
     * @brief Destructor - automatically cleans up resources
     */
    ~StabilizerWrapper() = default;
    
    // Disable copying and moving for thread safety
    StabilizerWrapper(const StabilizerWrapper&) = delete;
    StabilizerWrapper& operator=(const StabilizerWrapper&) = delete;
    StabilizerWrapper(StabilizerWrapper&&) = delete;
    StabilizerWrapper& operator=(StabilizerWrapper&&) = delete;
    
    /**
     * @brief Safely initialize the stabilizer
     * @param width Video frame width
     * @param height Video frame height  
     * @param params Stabilizer parameters
     * @return true if initialization successful, false otherwise
     */
    bool initialize(uint32_t width, uint32_t height, const StabilizerCore::StabilizerParams& params) {
        try {
            std::lock_guard<std::mutex> lock(mutex);
            
            // Clean up any existing stabilizer
            stabilizer.reset();
            
            // Create new stabilizer with RAII protection
            stabilizer = std::make_unique<StabilizerCore>();
            if (!stabilizer->initialize(width, height, params)) {
                stabilizer.reset();
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            // Log error and return failure
            // Note: In real implementation, this would use obs_log()
            stabilizer.reset();
            return false;
        }
    }
    
    /**
     * @brief Process video frame with exception safety
     * @param frame Input video frame
     * @return Stabilized frame, or original frame on error
     */
    cv::Mat process_frame(cv::Mat frame) {
        try {
            std::lock_guard<std::mutex> lock(mutex);
            
            if (!stabilizer) {
                // Return original frame if not initialized
                return frame;
            }
            
            return stabilizer->process_frame(frame);
        } catch (const std::exception& e) {
            // Log error and return original frame on exception
            // Note: In real implementation, this would use obs_log()
            return frame.clone();
        }
    }
    
    /**
     * @brief Check if wrapper is initialized
     * @return true if stabilizer is active, false otherwise
     */
    bool is_initialized() {
        std::lock_guard<std::mutex> lock(mutex);
        return stabilizer != nullptr;
    }
    
    /**
     * @brief Clear all state and reset the stabilizer
     */
    void clear_state() {
        try {
            std::lock_guard<std::mutex> lock(mutex);
            if (stabilizer) {
                stabilizer->clear_state();
            }
        } catch (const std::exception& e) {
            // Log error but don't throw
            // Note: In real implementation, this would use obs_log()
        }
    }
    
    /**
     * @brief Get last error message
     * @return Error string, or empty if no error
     */
    std::string get_last_error() {
        std::lock_guard<std::mutex> lock(mutex);
        if (stabilizer) {
            return stabilizer->get_last_error();
        }
        return "Not initialized";
    }
    
    /**
     * @brief Get current performance metrics
     * @return Performance metrics structure
     */
    StabilizerCore::PerformanceMetrics get_performance_metrics() {
        std::lock_guard<std::mutex> lock(mutex);
        if (stabilizer) {
            return stabilizer->get_performance_metrics();
        }
        return {};
    }
    
    /**
     * @brief Update stabilization parameters
     * @param params New parameters
     */
    void update_parameters(const StabilizerCore::StabilizerParams& params) {
        std::lock_guard<std::mutex> lock(mutex);
        if (stabilizer) {
            stabilizer->update_parameters(params);
        }
    }
    
    /**
     * @brief Get current parameters
     * @return The current stabilizer parameters
     */
    StabilizerCore::StabilizerParams get_current_params() {
        std::lock_guard<std::mutex> lock(mutex);
        if (stabilizer) {
            return stabilizer->get_current_params();
        }
        return {};
    }
    
    /**
     * @brief Reset the stabilizer state
     */
    void reset() {
        std::lock_guard<std::mutex> lock(mutex);
        if (stabilizer) {
            stabilizer->reset();
        }
    }
    
    /**
     * @brief Check if wrapper is ready
     * @return true if stabilizer is ready
     */
    bool is_ready() {
        std::lock_guard<std::mutex> lock(mutex);
        if (stabilizer) {
            return stabilizer->is_ready();
        }
        return false;
    }
};

#endif // STABILIZER_WRAPPER_HPP