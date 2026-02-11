#ifndef STABILIZER_WRAPPER_HPP
#define STABILIZER_WRAPPER_HPP

#include "stabilizer_core.hpp"
#include <opencv2/opencv.hpp>
#include <memory>
#include <string>

/**
 * @brief RAII wrapper for StabilizerCore for memory safety
 *
 * This class provides RAII resource management for StabilizerCore.
 * OBS filters are single-threaded by design, so no mutex is needed.
 *
 * Key benefits:
 * - Automatic cleanup via RAII pattern
 * - Exception-safe boundaries for OBS callbacks
 * - Safe initialization and error handling
 * - No mutex overhead (OBS filters are single-threaded)
 */
class StabilizerWrapper {
private:
    std::unique_ptr<StabilizerCore> stabilizer;

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