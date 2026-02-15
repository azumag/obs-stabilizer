/**
 * Preset Manager Implementation for OBS Stabilizer Plugin
 *
 * Handles persistence of custom presets to JSON files.
 */

#include "preset_manager.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef HAVE_OBS_HEADERS
#include <obs-module.h>
#endif

// Include nlohmann/json at top level to avoid namespace conflicts
// This must be outside any namespace to prevent std namespace pollution
#include <nlohmann/json.hpp>

namespace STABILIZER_PRESETS {

#ifdef HAVE_OBS_HEADERS
std::string PresetManager::get_preset_directory() {
    // Get OBS config directory
    const char* config_path = obs_get_config_path("obs-stabilizer/presets");
    if (!config_path) {
        obs_log(LOG_ERROR, "Failed to get OBS config path");
        return "";
    }

    std::string preset_dir(config_path);

    // Create directory if it doesn't exist
    try {
        std::filesystem::create_directories(preset_dir);
    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Failed to create preset directory: %s", e.what());
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
            obs_log(LOG_ERROR, "Failed to get preset file path for: %s", preset_name.c_str());
            return false;
        }

        // Create PresetInfo
        PresetInfo info;
        info.name = preset_name;
        info.description = description;
        info.params = params;

        // Convert to obs_data_t
        obs_data_t* data = preset_info_to_obs_data(info);
        if (!data) {
            obs_log(LOG_ERROR, "Failed to convert preset info to obs_data");
            return false;
        }

        // Save to JSON file
        bool success = obs_data_save_json_safe(data, file_path.c_str(), "tmp", "bak");
        obs_data_release(data);

        if (success) {
            obs_log(LOG_INFO, "Saved preset: %s", preset_name.c_str());
        } else {
            obs_log(LOG_ERROR, "Failed to save preset: %s", preset_name.c_str());
        }

        return success;

    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception saving preset: %s", e.what());
        return false;
    }
}

bool PresetManager::load_preset(const std::string& preset_name,
                                StabilizerCore::StabilizerParams& params) {
    try {
        std::string file_path = get_preset_file_path(preset_name);
        if (file_path.empty()) {
            obs_log(LOG_ERROR, "Failed to get preset file path for: %s", preset_name.c_str());
            return false;
        }

        // Check if file exists
        if (!std::filesystem::exists(file_path)) {
            obs_log(LOG_WARNING, "Preset file does not exist: %s", preset_name.c_str());
            return false;
        }

        // Load from JSON file
        obs_data_t* data = obs_data_create_from_json_file(file_path.c_str());
        if (!data) {
            obs_log(LOG_ERROR, "Failed to load preset file: %s", file_path.c_str());
            return false;
        }

        // Convert to PresetInfo
        PresetInfo info = obs_data_to_preset_info(data);
        obs_data_release(data);

        params = info.params;

        obs_log(LOG_INFO, "Loaded preset: %s", preset_name.c_str());
        return true;

    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception loading preset: %s", e.what());
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
            obs_log(LOG_INFO, "Deleted preset: %s", preset_name.c_str());
        } else {
            obs_log(LOG_ERROR, "Failed to delete preset: %s", preset_name.c_str());
        }

        return success;

    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception deleting preset: %s", e.what());
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
        obs_log(LOG_ERROR, "Exception listing presets: %s", e.what());
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
    obs_data_t* data = obs_data_create();

    // Save metadata
    obs_data_set_string(data, "name", info.name.c_str());
    obs_data_set_string(data, "description", info.description.c_str());

    // Save parameters
    obs_data_set_bool(data, "enabled", info.params.enabled);
    obs_data_set_int(data, "smoothing_radius", info.params.smoothing_radius);
    obs_data_set_double(data, "max_correction", info.params.max_correction);
    obs_data_set_int(data, "feature_count", info.params.feature_count);
    obs_data_set_double(data, "quality_level", info.params.quality_level);
    obs_data_set_double(data, "min_distance", info.params.min_distance);
    obs_data_set_int(data, "block_size", info.params.block_size);
    obs_data_set_bool(data, "use_harris", info.params.use_harris);
    obs_data_set_double(data, "k", info.params.k);
    obs_data_set_bool(data, "debug_mode", info.params.debug_mode);

    // Save edge mode
    const char* edge_str = "padding";
    switch (info.params.edge_mode) {
        case StabilizerCore::EdgeMode::Crop:
            edge_str = "crop";
            break;
        case StabilizerCore::EdgeMode::Scale:
            edge_str = "scale";
            break;
        case StabilizerCore::EdgeMode::Padding:
        default:
            edge_str = "padding";
            break;
    }
    obs_data_set_string(data, "edge_handling", edge_str);

    // Save motion thresholds
    obs_data_set_double(data, "frame_motion_threshold", info.params.frame_motion_threshold);
    obs_data_set_double(data, "max_displacement", info.params.max_displacement);
    obs_data_set_double(data, "tracking_error_threshold", info.params.tracking_error_threshold);

    // Save RANSAC parameters
    obs_data_set_double(data, "ransac_threshold_min", info.params.ransac_threshold_min);
    obs_data_set_double(data, "ransac_threshold_max", info.params.ransac_threshold_max);

    // Save point validation parameters
    obs_data_set_double(data, "min_point_spread", info.params.min_point_spread);
    obs_data_set_double(data, "max_coordinate", info.params.max_coordinate);

    return data;
}

PresetManager::PresetInfo PresetManager::obs_data_to_preset_info(obs_data_t* data) {
    PresetInfo info;

    // Load metadata
    info.name = obs_data_get_string(data, "name");
    info.description = obs_data_get_string(data, "description");

    // Load parameters
    info.params.enabled = obs_data_get_bool(data, "enabled");
    info.params.smoothing_radius = (int)obs_data_get_int(data, "smoothing_radius");
    info.params.max_correction = (float)obs_data_get_double(data, "max_correction");
    info.params.feature_count = (int)obs_data_get_int(data, "feature_count");
    info.params.quality_level = (float)obs_data_get_double(data, "quality_level");
    info.params.min_distance = (float)obs_data_get_double(data, "min_distance");
    info.params.block_size = (int)obs_data_get_int(data, "block_size");
    info.params.use_harris = obs_data_get_bool(data, "use_harris");
    info.params.k = (float)obs_data_get_double(data, "k");
    info.params.debug_mode = obs_data_get_bool(data, "debug_mode");

    // Load edge mode
    const char* edge_str = obs_data_get_string(data, "edge_handling");
    if (strcmp(edge_str, "crop") == 0) {
        info.params.edge_mode = StabilizerCore::EdgeMode::Crop;
    } else if (strcmp(edge_str, "scale") == 0) {
        info.params.edge_mode = StabilizerCore::EdgeMode::Scale;
    } else {
        info.params.edge_mode = StabilizerCore::EdgeMode::Padding;
    }

    // Load motion thresholds
    info.params.frame_motion_threshold = (float)obs_data_get_double(data, "frame_motion_threshold");
    info.params.max_displacement = (float)obs_data_get_double(data, "max_displacement");
    info.params.tracking_error_threshold = obs_data_get_double(data, "tracking_error_threshold");

    // Load RANSAC parameters
    info.params.ransac_threshold_min = (float)obs_data_get_double(data, "ransac_threshold_min");
    info.params.ransac_threshold_max = (float)obs_data_get_double(data, "ransac_threshold_max");

    // Load point validation parameters
    info.params.min_point_spread = (float)obs_data_get_double(data, "min_point_spread");
    info.params.max_coordinate = obs_data_get_double(data, "max_coordinate");

    return info;
}
#endif // HAVE_OBS_HEADERS

} // namespace STABILIZER_PRESETS

#ifdef HAVE_OBS_HEADERS
// OBS-based implementation is above in the #ifdef HAVE_OBS_HEADERS block
#else // !HAVE_OBS_HEADERS

// Re-open STABILIZER_PRESETS namespace for standalone implementation
namespace STABILIZER_PRESETS {

// Standalone implementation for testing without OBS headers
// nlohmann/json is already included at the top of the file
// Note: using namespace std is intentionally avoided to prevent namespace pollution
// All std types are fully qualified (std::string, std::ofstream, etc.)

std::string PresetManager::get_preset_directory() {
    // Use /tmp/obs-stabilizer-presets for standalone mode
    std::string preset_dir = "/tmp/obs-stabilizer-presets";

    // Create directory if it doesn't exist
    try {
        std::filesystem::create_directories(preset_dir);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create preset directory: " << e.what() << std::endl;
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
        std::cerr << "Preset name cannot be empty" << std::endl;
        return false;
    }

    std::string file_path = get_preset_file_path(preset_name);
    if (file_path.empty()) {
        std::cerr << "Failed to get preset file path" << std::endl;
        return false;
    }

    try {
        nlohmann::json j;
        j["name"] = preset_name;
        j["description"] = description;

        // Save parameters
        j["enabled"] = params.enabled;
        j["smoothing_radius"] = params.smoothing_radius;
        j["max_correction"] = params.max_correction;
        j["feature_count"] = params.feature_count;
        j["quality_level"] = params.quality_level;
        j["min_distance"] = params.min_distance;
        j["block_size"] = params.block_size;
        j["use_harris"] = params.use_harris;
        j["k"] = params.k;
        j["debug_mode"] = params.debug_mode;

        // Save edge mode
        const char* edge_str = "padding";
        switch (params.edge_mode) {
            case StabilizerCore::EdgeMode::Crop:
                edge_str = "crop";
                break;
            case StabilizerCore::EdgeMode::Scale:
                edge_str = "scale";
                break;
            case StabilizerCore::EdgeMode::Padding:
            default:
                edge_str = "padding";
                break;
        }
        j["edge_handling"] = edge_str;

        // Save motion thresholds
        j["frame_motion_threshold"] = params.frame_motion_threshold;
        j["max_displacement"] = params.max_displacement;
        j["tracking_error_threshold"] = params.tracking_error_threshold;

        // Save RANSAC parameters
        j["ransac_threshold_min"] = params.ransac_threshold_min;
        j["ransac_threshold_max"] = params.ransac_threshold_max;

        // Save point validation parameters
        j["min_point_spread"] = params.min_point_spread;
        j["max_coordinate"] = params.max_coordinate;

        // Write to file with atomic write
        std::string temp_path = file_path + ".tmp";
        std::ofstream file(temp_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << temp_path << std::endl;
            return false;
        }

        file << j.dump(4);
        file.close();

        // Atomic rename
        std::filesystem::rename(temp_path, file_path);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save preset: " << e.what() << std::endl;
        return false;
    }
}

bool PresetManager::load_preset(const std::string& preset_name,
                                StabilizerCore::StabilizerParams& params) {
    if (preset_name.empty()) {
        std::cerr << "Preset name cannot be empty" << std::endl;
        return false;
    }

    std::string file_path = get_preset_file_path(preset_name);
    if (file_path.empty() || !std::filesystem::exists(file_path)) {
        std::cerr << "Preset file does not exist: " << file_path << std::endl;
        return false;
    }

    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << file_path << std::endl;
            return false;
        }

        nlohmann::json j;
        file >> j;
        file.close();

        // Load parameters
        params.enabled = j.value("enabled", true);
        params.smoothing_radius = j.value("smoothing_radius", 30);
        params.max_correction = j.value("max_correction", 30.0f);
        params.feature_count = j.value("feature_count", 500);
        params.quality_level = j.value("quality_level", 0.01f);
        params.min_distance = j.value("min_distance", 30.0f);
        params.block_size = j.value("block_size", 3);
        params.use_harris = j.value("use_harris", false);
        params.k = j.value("k", 0.04f);
        params.debug_mode = j.value("debug_mode", false);

        // Load edge mode
        std::string edge_str = j.value("edge_handling", "padding");
        if (edge_str == "crop") {
            params.edge_mode = StabilizerCore::EdgeMode::Crop;
        } else if (edge_str == "scale") {
            params.edge_mode = StabilizerCore::EdgeMode::Scale;
        } else {
            params.edge_mode = StabilizerCore::EdgeMode::Padding;
        }

        // Load motion thresholds
        params.frame_motion_threshold = j.value("frame_motion_threshold", 0.25f);
        params.max_displacement = j.value("max_displacement", 1000.0f);
        params.tracking_error_threshold = j.value("tracking_error_threshold", 50.0);

        // Load RANSAC parameters
        params.ransac_threshold_min = j.value("ransac_threshold_min", 1.0f);
        params.ransac_threshold_max = j.value("ransac_threshold_max", 10.0f);

        // Load point validation parameters
        params.min_point_spread = j.value("min_point_spread", 10.0f);
        params.max_coordinate = j.value("max_coordinate", 100000.0f);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load preset: " << e.what() << std::endl;
        return false;
    }
}

bool PresetManager::delete_preset(const std::string& preset_name) {
    if (preset_name.empty()) {
        std::cerr << "Preset name cannot be empty" << std::endl;
        return false;
    }

    std::string file_path = get_preset_file_path(preset_name);
    if (file_path.empty() || !std::filesystem::exists(file_path)) {
        return false;
    }

    try {
        std::filesystem::remove(file_path);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to delete preset: " << e.what() << std::endl;
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
        std::cerr << "Failed to list presets: " << e.what() << std::endl;
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

} // namespace STABILIZER_PRESETS

#endif // !HAVE_OBS_HEADERS
