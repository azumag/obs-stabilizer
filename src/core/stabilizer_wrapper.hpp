#ifndef STABILIZER_WRAPPER_HPP
#define STABILIZER_WRAPPER_HPP

#include "stabilizer_core.hpp"
#include <mutex>
#include <memory>
#include <string>

/**
 * @brief Thread-safe RAII wrapper for StabilizerCore
 *
 * This class provides thread-safe RAII resource management for StabilizerCore.
 * OBS properties can be updated from UI thread while video processing happens on video thread,
 * so mutex is required to prevent data races.
 *
 * RATIONALE: Thread safety is implemented in StabilizerWrapper (not StabilizerCore) because:
 * 1. StabilizerCore is designed for single-threaded use (OBS video thread)
 * 2. StabilizerWrapper provides the interface boundary between OBS UI thread and video thread
 * 3. This separation keeps StabilizerCore simple (KISS principle) while ensuring thread safety
 *
 * Key benefits:
 * - Automatic cleanup via RAII pattern
 * - Exception-safe boundaries for OBS callbacks
 * - Safe initialization and error handling
 * - Thread-safe interface for concurrent UI and video thread access
 *
 * Design decision: Mutex is added here rather than StabilizerCore to:
 * - Avoid locking overhead in the performance-critical processing path
 * - Keep core algorithm simple and focused on video stabilization
 * - Provide clear separation of concerns (wrapper = thread safety, core = processing)
 */
class StabilizerWrapper {
private:
    std::unique_ptr<StabilizerCore> stabilizer;
    mutable std::mutex mutex_;  // Mutable to allow locking in const methods

public:
    StabilizerWrapper();
    ~StabilizerWrapper();

    // Disable copying and moving
    StabilizerWrapper(const StabilizerWrapper&) = delete;
    StabilizerWrapper& operator=(const StabilizerWrapper&) = delete;
    StabilizerWrapper(StabilizerWrapper&&) = delete;
    StabilizerWrapper& operator=(StabilizerWrapper&&) = delete;

    /**
     * @brief Initialize stabilizer
     * @param width Video frame width
     * @param height Video frame height
     * @param params Stabilizer parameters
     * @return true if initialization successful, false otherwise
     */
    bool initialize(uint32_t width, uint32_t height,
                  const StabilizerCore::StabilizerParams& params);

    /**
     * @brief Process video frame with exception safety
     * @param frame Input video frame
     * @return Stabilized frame, or original frame on error
     */
    cv::Mat process_frame(cv::Mat frame);

    /**
     * @brief Check if wrapper is initialized
     * @return true if stabilizer is active, false otherwise
     */
    bool is_initialized();

    /**
     * @brief Get last error message
     * @return Error string, or "Not initialized" if no error
     */
    std::string get_last_error();

    /**
     * @brief Get current performance metrics
     * @return Performance metrics structure
     */
    StabilizerCore::PerformanceMetrics get_performance_metrics();

    /**
     * @brief Update stabilization parameters
     * @param params New parameters
     */
    void update_parameters(const StabilizerCore::StabilizerParams& params);

    /**
     * @brief Get current parameters
     * @return The current stabilizer parameters
     */
    StabilizerCore::StabilizerParams get_current_params();

    /**
     * @brief Reset stabilizer state
     */
    void reset();

    /**
     * @brief Check if wrapper is ready
     * @return true if stabilizer is ready
     */
    bool is_ready();
};

#endif // STABILIZER_WRAPPER_HPP