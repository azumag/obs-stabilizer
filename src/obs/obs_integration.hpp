/*
 * OBS Integration Module
 * Handles OBS Studio API integration and plugin lifecycle
 * Separates OBS-specific code from core stabilization algorithms
 */

#pragma once

#include <obs-module.h>
#include "core/stabilizer_core.hpp"
#include <memory>
#include <string>

/**
 * OBS integration layer that wraps the StabilizerCore
 * Handles OBS data structures, properties, and plugin lifecycle
 */
class OBSIntegration {
public:
    struct OBSFilterData {
        obs_source_t* source = nullptr;
        std::unique_ptr<StabilizerCore> stabilizer;
        bool initialized = false;
    };

    /**
     * Create OBS filter data structure
     * @param settings OBS settings data
     * @param source OBS source
     * @return Filter data pointer or nullptr on error
     */
    static std::unique_ptr<OBSFilterData> create_filter_data(obs_data_t* settings, obs_source_t* source);

    /**
     * Destroy OBS filter data structure
     * @param data Filter data pointer
     */
    static void destroy_filter_data(OBSFilterData* data);

    /**
     * Update filter parameters from OBS settings
     * @param data Filter data pointer
     * @param settings OBS settings data
     */
    static bool update_parameters(OBSFilterData* data, obs_data_t* settings);

    /**
     * Process video frame using the stabilizer core
     * @param data Filter data pointer
     * @param frame Input OBS frame
     * @return Processed OBS frame or nullptr on error
     */
    static obs_source_frame* process_video_frame(OBSFilterData* data, obs_source_frame* frame);

    /**
     * Get default OBS settings for the filter
     * @param settings OBS settings data to populate
     */
    static void get_defaults(obs_data_t* settings);

    /**
     * Get OBS properties for the filter UI
     * @param settings OBS settings data
     * @return OBS properties pointer
     */
    static obs_properties_t* get_properties(void* data);

    /**
     * Get filter name
     * @return Filter name string
     */
    static const char* get_name(void* unused);

    /**
     * Get filter type
     * @return OBS source type
     */
    static enum obs_source_type get_type(void* unused);

    /**
     * Apply preset configuration
     * @param settings OBS settings data
     * @param preset_name Preset name
     */
    static void apply_preset(obs_data_t* settings, const char* preset_name);

    /**
     * Convert OBS settings to StabilizerCore parameters
     * @param settings OBS settings data
     * @return StabilizerCore parameters
     */
    static StabilizerCore::StabilizerParams settings_to_params(const obs_data_t* settings);

    /**
     * Convert StabilizerCore parameters to OBS settings
     * @param params StabilizerCore parameters
     * @param settings OBS settings data to populate
     */
    static void params_to_settings(const StabilizerCore::StabilizerParams& params, obs_data_t* settings);

private:
    // Frame conversion utilities
    static cv::Mat obs_frame_to_cv_mat(const obs_source_frame* frame);
    static obs_source_frame* cv_mat_to_obs_frame(const cv::Mat& mat, const obs_source_frame* reference_frame);

    // Property creation helpers
    static obs_properties_t* create_basic_properties();
    static obs_properties_t* create_advanced_properties();
    static obs_properties_t* create_performance_properties();

    // Validation helpers
    static bool validate_frame_format(const obs_source_frame* frame);
    static bool validate_settings(const obs_data_t* settings);

    // Error handling
    static void log_error(const std::string& message);
    static void log_info(const std::string& message);

    // Constants
    static constexpr const char* FILTER_NAME = "Video Stabilizer";
    static constexpr const char* FILTER_ID = "stabilizer_filter";
    static constexpr int MIN_SMOOTHING_RADIUS = 5;
    static constexpr int MAX_SMOOTHING_RADIUS = 200;
    static constexpr float MIN_MAX_CORRECTION = 1.0f;
    static constexpr float MAX_MAX_CORRECTION = 100.0f;
    static constexpr int MIN_FEATURE_COUNT = 50;
    static constexpr int MAX_FEATURE_COUNT = 2000;
    static constexpr float MIN_QUALITY_LEVEL = 0.001f;
    static constexpr float MAX_QUALITY_LEVEL = 0.1f;
    static constexpr float MIN_MIN_DISTANCE = 1.0f;
    static constexpr float MAX_MIN_DISTANCE = 200.0f;
    static constexpr int MIN_BLOCK_SIZE = 3;
    static constexpr int MAX_BLOCK_SIZE = 31;
    static constexpr float MIN_HARRIS_K = 0.01f;
    static constexpr float MAX_HARRIS_K = 0.1f;
};

/**
 * Property callback handler for preset changes
 */
class PresetHandler {
public:
    /**
     * Callback function for preset property changes
     * @param priv Private data (unused)
     * @param props OBS properties
     * @param property Changed property
     * @param settings OBS settings data
     * @return True if property was handled
     */
    static bool preset_changed_callback(void* priv, obs_properties_t* props, 
                                      obs_property_t* property, obs_data_t* settings);

private:
    static const char* get_preset_list_value(int index);
    static bool is_valid_preset(const char* preset_name);
};

/**
 * Performance monitoring for OBS integration
 */
class OBSPerformanceMonitor {
public:
    struct PerformanceStats {
        double avg_processing_time = 0.0;
        uint64_t total_frames = 0;
        uint64_t dropped_frames = 0;
        double current_fps = 0.0;
        bool performance_warning = false;
    };

    /**
     * Update performance statistics
     * @param stats Performance stats structure
     * @param processing_time Processing time for current frame in milliseconds
     */
    static void update_stats(PerformanceStats& stats, double processing_time);

    /**
     * Check if performance is acceptable
     * @param stats Performance stats structure
     * @param target_fps Target frame rate
     * @return True if performance is acceptable
     */
    static bool is_performance_acceptable(const PerformanceStats& stats, double target_fps = 30.0);

    /**
     * Get performance warning message
     * @param stats Performance stats structure
     * @return Warning message or empty string
     */
    static std::string get_performance_warning(const PerformanceStats& stats);

private:
    static constexpr double WARNING_THRESHOLD_MS = 33.33; // ~30 FPS
    static constexpr double CRITICAL_THRESHOLD_MS = 50.0; // ~20 FPS
};

/**
 * OBS data conversion utilities
 */
class OBSDataConverter {
public:
    /**
     * Safely get integer value from OBS data
     * @param data OBS data
     * @param name Parameter name
     * @param default_value Default value if not found
     * @return Integer value
     */
    static int get_int_safe(const obs_data_t* data, const char* name, int default_value);

    /**
     * Safely get double value from OBS data
     * @param data OBS data
     * @param name Parameter name
     * @param default_value Default value if not found
     * @return Double value
     */
    static double get_double_safe(const obs_data_t* data, const char* name, double default_value);

    /**
     * Safely get boolean value from OBS data
     * @param data OBS data
     * @param name Parameter name
     * @param default_value Default value if not found
     * @return Boolean value
     */
    static bool get_bool_safe(const obs_data_t* data, const char* name, bool default_value);

    /**
     * Safely get string value from OBS data
     * @param data OBS data
     * @param name Parameter name
     * @param default_value Default value if not found
     * @return String value
     */
    static std::string get_string_safe(const obs_data_t* data, const char* name, const std::string& default_value);

    /**
     * Set integer value to OBS data with validation
     * @param data OBS data
     * @param name Parameter name
     * @param value Integer value
     * @param min_value Minimum allowed value
     * @param max_value Maximum allowed value
     */
    static void set_int_validated(obs_data_t* data, const char* name, int value, int min_value, int max_value);

    /**
     * Set double value to OBS data with validation
     * @param data OBS data
     * @param name Parameter name
     * @param value Double value
     * @param min_value Minimum allowed value
     * @param max_value Maximum allowed value
     */
    static void set_double_validated(obs_data_t* data, const char* name, double value, double min_value, double max_value);
};