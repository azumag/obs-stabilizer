/**
 * Preset Manager Implementation for OBS Stabilizer Plugin
 *
 * Handles persistence of custom presets to JSON files.
 *
 * REFACTORED: To eliminate code duplication between OBS and standalone modes,
 * this implementation extracts common parameter serialization logic into
 * helper functions that work with nlohmann::json. The OBS mode then
 * converts obs_data_t to nlohmann::json and uses the common serializer.
 *
 * This follows the DRY principle while maintaining API compatibility with both
 * OBS's obs_data API and nlohmann::json for standalone testing.
 */

#include "preset_manager.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

#include "core/logging.hpp"

#ifdef HAVE_OBS_HEADERS
#include "obs_minimal.h"
#endif

// Include nlohmann/json at top level to avoid namespace conflicts
// This must be outside any namespace to prevent std namespace pollution
#include <nlohmann/json.hpp>

namespace STABILIZER_PRESETS {

namespace {

    // ========================================================================
    // Preset Field Names - Single source of truth for JSON field names
    // ========================================================================

    // Metadata fields
    constexpr const char* FIELD_NAME = "name";
    constexpr const char* FIELD_DESCRIPTION = "description";

    // Basic parameters
    constexpr const char* FIELD_ENABLED = "enabled";
    constexpr const char* FIELD_SMOOTHING_RADIUS = "smoothing_radius";
    constexpr const char* FIELD_MAX_CORRECTION = "max_correction";
    constexpr const char* FIELD_FEATURE_COUNT = "feature_count";
    constexpr const char* FIELD_QUALITY_LEVEL = "quality_level";
    constexpr const char* FIELD_MIN_DISTANCE = "min_distance";
    constexpr const char* FIELD_BLOCK_SIZE = "block_size";
    constexpr const char* FIELD_USE_HARRIS = "use_harris";
    constexpr const char* FIELD_K = "k";
    constexpr const char* FIELD_DEBUG_MODE = "debug_mode";

    // Edge handling
    constexpr const char* FIELD_EDGE_HANDLING = "edge_handling";
    constexpr const char* EDGE_MODE_PADDING = "padding";
    constexpr const char* EDGE_MODE_CROP = "crop";
    constexpr const char* EDGE_MODE_SCALE = "scale";

    // Motion thresholds
    constexpr const char* FIELD_FRAME_MOTION_THRESHOLD = "frame_motion_threshold";
    constexpr const char* FIELD_MAX_DISPLACEMENT = "max_displacement";
    constexpr const char* FIELD_TRACKING_ERROR_THRESHOLD = "tracking_error_threshold";

    // RANSAC parameters
    constexpr const char* FIELD_RANSAC_THRESHOLD_MIN = "ransac_threshold_min";
    constexpr const char* FIELD_RANSAC_THRESHOLD_MAX = "ransac_threshold_max";

    // Point validation parameters
    constexpr const char* FIELD_MIN_POINT_SPREAD = "min_point_spread";
    constexpr const char* FIELD_MAX_COORDINATE = "max_coordinate";

    // Default values for parameters (must match VALIDATION namespace)
    constexpr int DEFAULT_SMOOTHING_RADIUS = 30;
    constexpr double DEFAULT_MAX_CORRECTION = 30.0;
    constexpr int DEFAULT_FEATURE_COUNT = 500;
    constexpr double DEFAULT_QUALITY_LEVEL = 0.01;
    constexpr double DEFAULT_MIN_DISTANCE = 30.0;
    constexpr int DEFAULT_BLOCK_SIZE = 3;
    constexpr bool DEFAULT_USE_HARRIS = false;
    constexpr double DEFAULT_K = 0.04;
    constexpr bool DEFAULT_DEBUG_MODE = false;

    // Default motion thresholds
    constexpr double DEFAULT_FRAME_MOTION_THRESHOLD = 0.25;
    constexpr double DEFAULT_MAX_DISPLACEMENT = 1000.0;
    constexpr double DEFAULT_TRACKING_ERROR_THRESHOLD = 50.0;

    // Default RANSAC parameters
    constexpr double DEFAULT_RANSAC_THRESHOLD_MIN = 1.0;
    constexpr double DEFAULT_RANSAC_THRESHOLD_MAX = 10.0;

    // Default point validation parameters
    constexpr double DEFAULT_MIN_POINT_SPREAD = 10.0;
    constexpr double DEFAULT_MAX_COORDINATE = 100000.0;

    // ========================================================================
    // Common Parameter Serialization Functions
    // ========================================================================

    /**
     * Convert edge mode enum to string
     * RATIONALE: Extracted to eliminate duplication between OBS and standalone modes
     */
    inline const char* edge_mode_to_string(StabilizerCore::EdgeMode mode) {
        switch (mode) {
            case StabilizerCore::EdgeMode::Crop:
                return EDGE_MODE_CROP;
            case StabilizerCore::EdgeMode::Scale:
                return EDGE_MODE_SCALE;
            case StabilizerCore::EdgeMode::Padding:
            default:
                return EDGE_MODE_PADDING;
        }
    }

    /**
     * Convert edge mode string to enum
     * RATIONALE: Extracted to eliminate duplication between OBS and standalone modes
     */
    inline StabilizerCore::EdgeMode string_to_edge_mode(const std::string& mode_str) {
        if (mode_str == EDGE_MODE_CROP) {
            return StabilizerCore::EdgeMode::Crop;
        } else if (mode_str == EDGE_MODE_SCALE) {
            return StabilizerCore::EdgeMode::Scale;
        } else {
            return StabilizerCore::EdgeMode::Padding;
        }
    }

    /**
     * Serialize StabilizerParams to nlohmann::json
     * RATIONALE: Single implementation used by both OBS and standalone modes
     * This eliminates ~200 lines of duplicated code
     */
    nlohmann::json params_to_json(const StabilizerCore::StabilizerParams& params) {
        nlohmann::json j;

        // Basic parameters
        j[FIELD_ENABLED] = params.enabled;
        j[FIELD_SMOOTHING_RADIUS] = params.smoothing_radius;
        j[FIELD_MAX_CORRECTION] = params.max_correction;
        j[FIELD_FEATURE_COUNT] = params.feature_count;
        j[FIELD_QUALITY_LEVEL] = params.quality_level;
        j[FIELD_MIN_DISTANCE] = params.min_distance;
        j[FIELD_BLOCK_SIZE] = params.block_size;
        j[FIELD_USE_HARRIS] = params.use_harris;
        j[FIELD_K] = params.k;
        j[FIELD_DEBUG_MODE] = params.debug_mode;

        // Edge handling
        j[FIELD_EDGE_HANDLING] = edge_mode_to_string(params.edge_mode);

        // Motion thresholds
        j[FIELD_FRAME_MOTION_THRESHOLD] = params.frame_motion_threshold;
        j[FIELD_MAX_DISPLACEMENT] = params.max_displacement;
        j[FIELD_TRACKING_ERROR_THRESHOLD] = params.tracking_error_threshold;

        // RANSAC parameters
        j[FIELD_RANSAC_THRESHOLD_MIN] = params.ransac_threshold_min;
        j[FIELD_RANSAC_THRESHOLD_MAX] = params.ransac_threshold_max;

        // Point validation parameters
        j[FIELD_MIN_POINT_SPREAD] = params.min_point_spread;
        j[FIELD_MAX_COORDINATE] = params.max_coordinate;

        return j;
    }

    /**
     * Deserialize StabilizerParams from nlohmann::json
     * RATIONALE: Single implementation used by both OBS and standalone modes
     * This eliminates ~200 lines of duplicated code
     */
    void json_to_params(const nlohmann::json& j, StabilizerCore::StabilizerParams& params) {
        // Load basic parameters with defaults
        params.enabled = j.value(FIELD_ENABLED, true);
        params.smoothing_radius = j.value(FIELD_SMOOTHING_RADIUS, DEFAULT_SMOOTHING_RADIUS);
        params.max_correction = static_cast<float>(j.value(FIELD_MAX_CORRECTION, DEFAULT_MAX_CORRECTION));
        params.feature_count = j.value(FIELD_FEATURE_COUNT, DEFAULT_FEATURE_COUNT);
        params.quality_level = static_cast<float>(j.value(FIELD_QUALITY_LEVEL, DEFAULT_QUALITY_LEVEL));
        params.min_distance = static_cast<float>(j.value(FIELD_MIN_DISTANCE, DEFAULT_MIN_DISTANCE));
        params.block_size = j.value(FIELD_BLOCK_SIZE, DEFAULT_BLOCK_SIZE);
        params.use_harris = j.value(FIELD_USE_HARRIS, DEFAULT_USE_HARRIS);
        params.k = static_cast<float>(j.value(FIELD_K, DEFAULT_K));
        params.debug_mode = j.value(FIELD_DEBUG_MODE, DEFAULT_DEBUG_MODE);

        // Load edge handling
        std::string edge_str = j.value(FIELD_EDGE_HANDLING, EDGE_MODE_PADDING);
        params.edge_mode = string_to_edge_mode(edge_str);

        // Load motion thresholds
        params.frame_motion_threshold = static_cast<float>(
            j.value(FIELD_FRAME_MOTION_THRESHOLD, DEFAULT_FRAME_MOTION_THRESHOLD));
        params.max_displacement = static_cast<float>(
            j.value(FIELD_MAX_DISPLACEMENT, DEFAULT_MAX_DISPLACEMENT));
        params.tracking_error_threshold = j.value(FIELD_TRACKING_ERROR_THRESHOLD, DEFAULT_TRACKING_ERROR_THRESHOLD);

        // Load RANSAC parameters
        params.ransac_threshold_min = static_cast<float>(
            j.value(FIELD_RANSAC_THRESHOLD_MIN, DEFAULT_RANSAC_THRESHOLD_MIN));
        params.ransac_threshold_max = static_cast<float>(
            j.value(FIELD_RANSAC_THRESHOLD_MAX, DEFAULT_RANSAC_THRESHOLD_MAX));

        // Load point validation parameters
        params.min_point_spread = static_cast<float>(
            j.value(FIELD_MIN_POINT_SPREAD, DEFAULT_MIN_POINT_SPREAD));
        params.max_coordinate = static_cast<float>(
            j.value(FIELD_MAX_COORDINATE, DEFAULT_MAX_COORDINATE));
    }

} // anonymous namespace

// ============================================================================
// OBS Mode Implementation (uses obs_data API)
// ============================================================================
// OBS Mode Implementation (uses obs_data API)
// ============================================================================

#if defined(HAVE_OBS_HEADERS) && !defined(STANDALONE_TEST)

/**
 * Convert obs_data_t to nlohmann::json for use with common serializers
 * RATIONALE: Bridges OBS API and common serialization logic
 */
static nlohmann::json obs_data_to_json(obs_data_t* data) {
    nlohmann::json j;

    // Extract all fields from obs_data
    j[FIELD_NAME] = obs_data_get_string(data, FIELD_NAME);
    j[FIELD_DESCRIPTION] = obs_data_get_string(data, FIELD_DESCRIPTION);
    j[FIELD_ENABLED] = obs_data_get_bool(data, FIELD_ENABLED);
    j[FIELD_SMOOTHING_RADIUS] = obs_data_get_int(data, FIELD_SMOOTHING_RADIUS);
    j[FIELD_MAX_CORRECTION] = obs_data_get_double(data, FIELD_MAX_CORRECTION);
    j[FIELD_FEATURE_COUNT] = obs_data_get_int(data, FIELD_FEATURE_COUNT);
    j[FIELD_QUALITY_LEVEL] = obs_data_get_double(data, FIELD_QUALITY_LEVEL);
    j[FIELD_MIN_DISTANCE] = obs_data_get_double(data, FIELD_MIN_DISTANCE);
    j[FIELD_BLOCK_SIZE] = obs_data_get_int(data, FIELD_BLOCK_SIZE);
    j[FIELD_USE_HARRIS] = obs_data_get_bool(data, FIELD_USE_HARRIS);
    j[FIELD_K] = obs_data_get_double(data, FIELD_K);
    j[FIELD_DEBUG_MODE] = obs_data_get_bool(data, FIELD_DEBUG_MODE);
    j[FIELD_EDGE_HANDLING] = obs_data_get_string(data, FIELD_EDGE_HANDLING);
    j[FIELD_FRAME_MOTION_THRESHOLD] = obs_data_get_double(data, FIELD_FRAME_MOTION_THRESHOLD);
    j[FIELD_MAX_DISPLACEMENT] = obs_data_get_double(data, FIELD_MAX_DISPLACEMENT);
    j[FIELD_TRACKING_ERROR_THRESHOLD] = obs_data_get_double(data, FIELD_TRACKING_ERROR_THRESHOLD);
    j[FIELD_RANSAC_THRESHOLD_MIN] = obs_data_get_double(data, FIELD_RANSAC_THRESHOLD_MIN);
    j[FIELD_RANSAC_THRESHOLD_MAX] = obs_data_get_double(data, FIELD_RANSAC_THRESHOLD_MAX);
    j[FIELD_MIN_POINT_SPREAD] = obs_data_get_double(data, FIELD_MIN_POINT_SPREAD);
    j[FIELD_MAX_COORDINATE] = obs_data_get_double(data, FIELD_MAX_COORDINATE);

    return j;
}

/**
 * Convert nlohmann::json to obs_data_t for use with OBS API
 * RATIONALE: Bridges common serialization logic and OBS API
 */
static obs_data_t* json_to_obs_data(const nlohmann::json& j) {
    obs_data_t* data = obs_data_create();
    if (!data) {
        return nullptr;
    }

    // Set all fields in obs_data
    obs_data_set_string(data, FIELD_NAME, j.value(FIELD_NAME, "").c_str());
    obs_data_set_string(data, FIELD_DESCRIPTION, j.value(FIELD_DESCRIPTION, "").c_str());
    obs_data_set_bool(data, FIELD_ENABLED, j.value(FIELD_ENABLED, true));
    obs_data_set_int(data, FIELD_SMOOTHING_RADIUS, j.value(FIELD_SMOOTHING_RADIUS, DEFAULT_SMOOTHING_RADIUS));
    obs_data_set_double(data, FIELD_MAX_CORRECTION, j.value(FIELD_MAX_CORRECTION, DEFAULT_MAX_CORRECTION));
    obs_data_set_int(data, FIELD_FEATURE_COUNT, j.value(FIELD_FEATURE_COUNT, DEFAULT_FEATURE_COUNT));
    obs_data_set_double(data, FIELD_QUALITY_LEVEL, j.value(FIELD_QUALITY_LEVEL, DEFAULT_QUALITY_LEVEL));
    obs_data_set_double(data, FIELD_MIN_DISTANCE, j.value(FIELD_MIN_DISTANCE, DEFAULT_MIN_DISTANCE));
    obs_data_set_int(data, FIELD_BLOCK_SIZE, j.value(FIELD_BLOCK_SIZE, DEFAULT_BLOCK_SIZE));
    obs_data_set_bool(data, FIELD_USE_HARRIS, j.value(FIELD_USE_HARRIS, DEFAULT_USE_HARRIS));
    obs_data_set_double(data, FIELD_K, j.value(FIELD_K, DEFAULT_K));
    obs_data_set_bool(data, FIELD_DEBUG_MODE, j.value(FIELD_DEBUG_MODE, DEFAULT_DEBUG_MODE));
    obs_data_set_string(data, FIELD_EDGE_HANDLING, j.value(FIELD_EDGE_HANDLING, EDGE_MODE_PADDING).c_str());
    obs_data_set_double(data, FIELD_FRAME_MOTION_THRESHOLD,
                        j.value(FIELD_FRAME_MOTION_THRESHOLD, DEFAULT_FRAME_MOTION_THRESHOLD));
    obs_data_set_double(data, FIELD_MAX_DISPLACEMENT,
                        j.value(FIELD_MAX_DISPLACEMENT, DEFAULT_MAX_DISPLACEMENT));
    obs_data_set_double(data, FIELD_TRACKING_ERROR_THRESHOLD,
                        j.value(FIELD_TRACKING_ERROR_THRESHOLD, DEFAULT_TRACKING_ERROR_THRESHOLD));
    obs_data_set_double(data, FIELD_RANSAC_THRESHOLD_MIN,
                        j.value(FIELD_RANSAC_THRESHOLD_MIN, DEFAULT_RANSAC_THRESHOLD_MIN));
    obs_data_set_double(data, FIELD_RANSAC_THRESHOLD_MAX,
                        j.value(FIELD_RANSAC_THRESHOLD_MAX, DEFAULT_RANSAC_THRESHOLD_MAX));
    obs_data_set_double(data, FIELD_MIN_POINT_SPREAD,
                        j.value(FIELD_MIN_POINT_SPREAD, DEFAULT_MIN_POINT_SPREAD));
    obs_data_set_double(data, FIELD_MAX_COORDINATE,
                        j.value(FIELD_MAX_COORDINATE, DEFAULT_MAX_COORDINATE));

    return data;
}

std::string PresetManager::get_preset_directory() {
    // Get OBS config directory
    const char* config_path = obs_get_config_path("obs-stabilizer/presets");
    // Check for nullptr or empty string
    // RATIONALE: obs_get_config_path() returns nullptr or empty string in test environments
    // because OBS is not fully initialized. Using /tmp as fallback ensures tests work
    // without requiring full OBS initialization. In production, this serves as a safety net
    // for unexpected initialization failures.
    if (!config_path || config_path[0] == '\0') {
        std::string preset_dir = "/tmp/obs-stabilizer-presets";
        try {
            std::filesystem::create_directories(preset_dir);
            // Log warning - this should only happen in test environments
            CORE_LOG_WARNING("OBS config path unavailable, using fallback: %s", preset_dir.c_str());
        } catch (const std::exception& e) {
            CORE_LOG_ERROR("Failed to create preset directory: %s", e.what());
            return "";
        }
        return preset_dir;
    }

    std::string preset_dir(config_path);

    // Create directory if it doesn't exist
    try {
        std::filesystem::create_directories(preset_dir);
    } catch (const std::exception& e) {
        CORE_LOG_ERROR("Failed to create preset directory: %s", e.what());
        return "";
    }

    return preset_dir;
}

std::string PresetManager::get_preset_file_path(const std::string& preset_name) {
    std::string preset_dir = get_preset_directory();
    if (preset_dir.empty()) {
        return "";
    }

    return preset_dir + "/" + preset_name + ".json";
}

bool PresetManager::save_preset(const std::string& preset_name,
                                const StabilizerCore::StabilizerParams& params,
                                const std::string& description) {
    try {
        std::string file_path = get_preset_file_path(preset_name);
        if (file_path.empty()) {
            CORE_LOG_ERROR("Failed to get preset file path for: %s", preset_name.c_str());
            return false;
        }

        // Use common JSON serialization
        nlohmann::json j;
        j[FIELD_NAME] = preset_name;
        j[FIELD_DESCRIPTION] = description;
        j.update(params_to_json(params));

        // Convert to obs_data_t and save
        obs_data_t* data = json_to_obs_data(j);
        if (!data) {
            CORE_LOG_ERROR("Failed to convert preset to obs_data");
            return false;
        }

        // Save to JSON file
        bool success = obs_data_save_json_safe(data, file_path.c_str(), "tmp", "bak");
        obs_data_release(data);

        if (success) {
            CORE_LOG_INFO("Saved preset: %s", preset_name.c_str());
        } else {
            CORE_LOG_ERROR("Failed to save preset: %s", preset_name.c_str());
        }

        return success;

    } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception saving preset: %s", e.what());
        return false;
    }
}

bool PresetManager::load_preset(const std::string& preset_name,
                                StabilizerCore::StabilizerParams& params) {
    try {
        std::string file_path = get_preset_file_path(preset_name);
        if (file_path.empty()) {
            CORE_LOG_ERROR("Failed to get preset file path for: %s", preset_name.c_str());
            return false;
        }

        // Check if file exists
        if (!std::filesystem::exists(file_path)) {
            CORE_LOG_WARNING("Preset file does not exist: %s", preset_name.c_str());
            return false;
        }

        // Load from JSON file
        obs_data_t* data = obs_data_create_from_json_file(file_path.c_str());
        if (!data) {
            CORE_LOG_ERROR("Failed to load preset file: %s", file_path.c_str());
            return false;
        }

        // Convert to JSON and use common deserializer
        nlohmann::json j = obs_data_to_json(data);
        obs_data_release(data);

        json_to_params(j, params);

        CORE_LOG_INFO("Loaded preset: %s", preset_name.c_str());
        return true;

    } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception loading preset: %s", e.what());
        return false;
    }
}

bool PresetManager::delete_preset(const std::string& preset_name) {
    try {
        std::string file_path = get_preset_file_path(preset_name);
        if (file_path.empty()) {
            return false;
        }

        if (!std::filesystem::exists(file_path)) {
            return false;
        }

        bool success = std::filesystem::remove(file_path);
        if (success) {
            CORE_LOG_INFO("Deleted preset: %s", preset_name.c_str());
        } else {
            CORE_LOG_ERROR("Failed to delete preset: %s", preset_name.c_str());
        }

        return success;

    } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception deleting preset: %s", e.what());
        return false;
    }
}

std::vector<std::string> PresetManager::list_presets() {
    std::vector<std::string> presets;

    try {
        std::string preset_dir = get_preset_directory();
        if (preset_dir.empty()) {
            return presets;
        }

        // Iterate through files in preset directory
        for (const auto& entry : std::filesystem::directory_iterator(preset_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                // Extract preset name (without .json extension)
                std::string filename = entry.path().stem().string();
                presets.push_back(filename);
            }
        }

        // Sort alphabetically
        std::sort(presets.begin(), presets.end());

    } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception listing presets: %s", e.what());
    }

    return presets;
}

bool PresetManager::preset_exists(const std::string& preset_name) {
    std::string file_path = get_preset_file_path(preset_name);
    if (file_path.empty()) {
        return false;
    }

    return std::filesystem::exists(file_path);
}

obs_data_t* PresetManager::preset_info_to_obs_data(const PresetInfo& info) {
    // Use common JSON serialization then convert to obs_data
    nlohmann::json j;
    j[FIELD_NAME] = info.name;
    j[FIELD_DESCRIPTION] = info.description;
    j.update(params_to_json(info.params));

    return json_to_obs_data(j);
}

PresetInfo PresetManager::obs_data_to_preset_info(obs_data_t* data) {
    PresetInfo info;

    // Convert obs_data to JSON then use common deserializer
    nlohmann::json j = obs_data_to_json(data);

    info.name = j.value(FIELD_NAME, "");
    info.description = j.value(FIELD_DESCRIPTION, "");
    json_to_params(j, info.params);

    return info;
}

#endif // HAVE_OBS_HEADERS

// ============================================================================
// Standalone Mode Implementation (uses nlohmann::json directly)
// ============================================================================

#if defined(STANDALONE_TEST) || !defined(HAVE_OBS_HEADERS)

std::string PresetManager::get_preset_directory() {
    // Use /tmp/obs-stabilizer-presets for standalone mode
    std::string preset_dir = "/tmp/obs-stabilizer-presets";

    // Create directory if it doesn't exist
    try {
        std::filesystem::create_directories(preset_dir);
    } catch (const std::exception& e) {
        CORE_LOG_ERROR("Failed to create preset directory: %s", e.what());
        return "";
    }

    return preset_dir;
}

std::string PresetManager::get_preset_file_path(const std::string& preset_name) {
    std::string preset_dir = get_preset_directory();
    if (preset_dir.empty()) {
        return "";
    }

    return preset_dir + "/" + preset_name + ".json";
}

bool PresetManager::save_preset(const std::string& preset_name,
                                const StabilizerCore::StabilizerParams& params,
                                const std::string& description) {
    if (preset_name.empty()) {
        CORE_LOG_ERROR("Preset name cannot be empty");
        return false;
    }

    std::string file_path = get_preset_file_path(preset_name);
    if (file_path.empty()) {
        CORE_LOG_ERROR("Failed to get preset file path");
        return false;
    }

    try {
        // Use common JSON serialization
        nlohmann::json j;
        j[FIELD_NAME] = preset_name;
        j[FIELD_DESCRIPTION] = description;
        j.update(params_to_json(params));

        // Write to file with atomic write
        std::string temp_path = file_path + ".tmp";
        std::ofstream file(temp_path);
        if (!file.is_open()) {
            CORE_LOG_ERROR("Failed to open file for writing: %s", temp_path.c_str());
            return false;
        }

        file << j.dump(4);
        file.close();

        // Atomic rename
        std::filesystem::rename(temp_path, file_path);

        CORE_LOG_INFO("Saved preset: %s", preset_name.c_str());
        return true;

    } catch (const std::exception& e) {
        CORE_LOG_ERROR("Failed to save preset: %s", e.what());
        return false;
    }
}

bool PresetManager::load_preset(const std::string& preset_name,
                                StabilizerCore::StabilizerParams& params) {
    if (preset_name.empty()) {
        CORE_LOG_ERROR("Preset name cannot be empty");
        return false;
    }

    std::string file_path = get_preset_file_path(preset_name);
    if (file_path.empty() || !std::filesystem::exists(file_path)) {
        CORE_LOG_ERROR("Preset file does not exist: %s", file_path.c_str());
        return false;
    }

    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            CORE_LOG_ERROR("Failed to open file for reading: %s", file_path.c_str());
            return false;
        }

        nlohmann::json j;
        file >> j;
        file.close();

        // Use common JSON deserialization
        json_to_params(j, params);

        CORE_LOG_INFO("Loaded preset: %s", preset_name.c_str());
        return true;

    } catch (const std::exception& e) {
        CORE_LOG_ERROR("Failed to load preset: %s", e.what());
        return false;
    }
}

bool PresetManager::delete_preset(const std::string& preset_name) {
    if (preset_name.empty()) {
        CORE_LOG_ERROR("Preset name cannot be empty");
        return false;
    }

    std::string file_path = get_preset_file_path(preset_name);
    if (file_path.empty() || !std::filesystem::exists(file_path)) {
        return false;
    }

    try {
        std::filesystem::remove(file_path);
        CORE_LOG_INFO("Deleted preset: %s", preset_name.c_str());
        return true;
    } catch (const std::exception& e) {
        CORE_LOG_ERROR("Failed to delete preset: %s", e.what());
        return false;
    }
}

std::vector<std::string> PresetManager::list_presets() {
    std::vector<std::string> presets;
    std::string preset_dir = get_preset_directory();

    if (preset_dir.empty()) {
        return presets;
    }

    try {
        for (const auto& entry : std::filesystem::directory_iterator(preset_dir)) {
            if (entry.path().extension() == ".json") {
                std::string name = entry.path().stem().string();
                presets.push_back(name);
            }
        }
    } catch (const std::exception& e) {
        CORE_LOG_ERROR("Failed to list presets: %s", e.what());
    }

    return presets;
}

bool PresetManager::preset_exists(const std::string& preset_name) {
    if (preset_name.empty()) {
        return false;
    }

    std::string file_path = get_preset_file_path(preset_name);
    if (file_path.empty()) {
        return false;
    }

    return std::filesystem::exists(file_path);
}

#endif // STANDALONE_TEST || !HAVE_OBS_HEADERS

} // namespace STABILIZER_PRESETS
