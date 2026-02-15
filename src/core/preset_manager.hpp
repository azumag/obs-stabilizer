/**
 * Preset Manager for OBS Stabilizer Plugin
 *
 * Handles persistence of custom presets to JSON files.
 * Allows users to save custom parameter configurations and load them later.
 *
 * Implementation uses OBS's obs_data API for JSON serialization,
 * which is compatible with OBS's existing configuration system.
 */

#ifndef PRESET_MANAGER_HPP
#define PRESET_MANAGER_HPP

#include "stabilizer_core.hpp"
#include <string>
#include <vector>

#ifdef HAVE_OBS_HEADERS
#include <obs-data.h>
#endif

namespace STABILIZER_PRESETS {

/**
 * Preset information structure
 */
struct PresetInfo {
    std::string name;
    std::string description;
    StabilizerCore::StabilizerParams params;
};

/**
 * Preset Manager class
 *
 * Provides methods to save and load custom presets to/from JSON files.
 * Uses OBS config directory for storing preset files.
 */
class PresetManager {
public:
    /**
     * Get the preset configuration directory
     * Returns the path where preset JSON files are stored
     */
    static std::string get_preset_directory();

    /**
     * Save a custom preset to JSON file
     *
     * @param preset_name Name of the preset
     * @param params Stabilizer parameters to save
     * @param description Optional description
     * @return true on success, false on failure
     */
    static bool save_preset(const std::string& preset_name,
                          const StabilizerCore::StabilizerParams& params,
                          const std::string& description = "");

    /**
     * Load a custom preset from JSON file
     *
     * @param preset_name Name of the preset to load
     * @param params Output parameter for loaded parameters
     * @return true on success, false on failure
     */
    static bool load_preset(const std::string& preset_name,
                          StabilizerCore::StabilizerParams& params);

    /**
     * Delete a custom preset
     *
     * @param preset_name Name of the preset to delete
     * @return true on success, false if preset doesn't exist
     */
    static bool delete_preset(const std::string& preset_name);

    /**
     * List all available custom presets
     *
     * @return Vector of preset names
     */
    static std::vector<std::string> list_presets();

    /**
     * Check if a preset exists
     *
     * @param preset_name Name of the preset to check
     * @return true if preset exists, false otherwise
     */
    static bool preset_exists(const std::string& preset_name);

    /**
     * Get preset file path for a given preset name
     *
     * @param preset_name Name of the preset
     * @return Full path to the preset JSON file
     */
    static std::string get_preset_file_path(const std::string& preset_name);

#ifdef HAVE_OBS_HEADERS
    /**
     * Convert PresetInfo to obs_data_t for saving
     *
     * @param info PresetInfo to convert
     * @return obs_data_t object (caller must release)
     */
    static obs_data_t* preset_info_to_obs_data(const PresetInfo& info);

    /**
     * Convert obs_data_t to PresetInfo
     *
     * @param data obs_data_t to convert
     * @return PresetInfo object
     */
    static PresetInfo obs_data_to_preset_info(obs_data_t* data);
#endif

private:
    // Prevent instantiation
    PresetManager() = default;
};

} // namespace STABILIZER_PRESETS

#endif // PRESET_MANAGER_HPP
