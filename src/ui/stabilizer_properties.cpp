// OpenCV must be included before the OBS compatibility header because the
// OBS headers define an EXPORT macro that would otherwise collide with
// OpenCV's CV_EXPORTS declarations.
#include <opencv2/opencv.hpp>

#include "ui/stabilizer_properties.hpp"

#include <cstdio>
#include <cstring>

#include "core/preset_manager.hpp"
#include "core/parameter_validation.hpp"

#ifdef HAVE_OBS_HEADERS

namespace {

using StabilizerParams = StabilizerCore::StabilizerParams;

// Parameter conversion helpers shared with the OBS integration layer.
namespace OBS_WRAPPER {
    inline bool get_bool(const obs_data_t* settings, const char* name) {
        return obs_data_get_bool(const_cast<obs_data_t*>(settings), name);
    }

    inline int64_t get_int(const obs_data_t* settings, const char* name) {
        return obs_data_get_int(const_cast<obs_data_t*>(settings), name);
    }

    inline double get_double(const obs_data_t* settings, const char* name) {
        return obs_data_get_double(const_cast<obs_data_t*>(settings), name);
    }

    inline const char* get_string(const obs_data_t* settings, const char* name) {
        return obs_data_get_string(const_cast<obs_data_t*>(settings), name);
    }
}

} // namespace

StabilizerParams settings_to_params(const obs_data_t *settings)
{
    StabilizerParams params;

    params.enabled = OBS_WRAPPER::get_bool(settings, "enabled");
    params.smoothing_radius = static_cast<int>(OBS_WRAPPER::get_int(settings, "smoothing_radius"));
    params.max_correction = static_cast<float>(OBS_WRAPPER::get_double(settings, "max_correction"));
    params.feature_count = static_cast<int>(OBS_WRAPPER::get_int(settings, "feature_count"));
    params.quality_level = static_cast<float>(OBS_WRAPPER::get_double(settings, "quality_level"));
    params.min_distance = static_cast<float>(OBS_WRAPPER::get_double(settings, "min_distance"));
    params.block_size = static_cast<int>(OBS_WRAPPER::get_int(settings, "block_size"));
    params.use_harris = OBS_WRAPPER::get_bool(settings, "use_harris");
    params.k = static_cast<float>(OBS_WRAPPER::get_double(settings, "k"));
    params.debug_mode = OBS_WRAPPER::get_bool(settings, "debug_mode");

    const char* edge_str = OBS_WRAPPER::get_string(settings, "edge_handling");
    if (strcmp(edge_str, "crop") == 0) {
        params.edge_mode = StabilizerCore::EdgeMode::Crop;
    } else if (strcmp(edge_str, "scale") == 0) {
        params.edge_mode = StabilizerCore::EdgeMode::Scale;
    } else {
        params.edge_mode = StabilizerCore::EdgeMode::Padding;
    }

    const char* smoothing_str = OBS_WRAPPER::get_string(settings, "smoothing_mode");
    if (smoothing_str && strcmp(smoothing_str, "kalman") == 0) {
        params.smoothing_mode = StabilizerCore::SmoothingMode::Kalman;
    } else {
        params.smoothing_mode = StabilizerCore::SmoothingMode::MovingAverage;
    }

    params = VALIDATION::validate_parameters(params);
    return params;
}

void params_to_settings(const StabilizerParams& params, obs_data_t *settings)
{
    obs_data_set_bool(settings, "enabled", params.enabled);
    obs_data_set_int(settings, "smoothing_radius", params.smoothing_radius);
    obs_data_set_double(settings, "max_correction", params.max_correction);
    obs_data_set_int(settings, "feature_count", params.feature_count);
    obs_data_set_double(settings, "quality_level", params.quality_level);
    obs_data_set_double(settings, "min_distance", params.min_distance);
    obs_data_set_int(settings, "block_size", params.block_size);
    obs_data_set_bool(settings, "use_harris", params.use_harris);
    obs_data_set_double(settings, "k", params.k);
    obs_data_set_bool(settings, "debug_mode", params.debug_mode);

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
    obs_data_set_string(settings, "edge_handling", edge_str);

    const char* smoothing_str = "moving_average";
    if (params.smoothing_mode == StabilizerCore::SmoothingMode::Kalman) {
        smoothing_str = "kalman";
    }
    obs_data_set_string(settings, "smoothing_mode", smoothing_str);
}

void apply_preset(obs_data_t *settings, const char *preset_name)
{
    StabilizerParams params;

    if (strcmp(preset_name, "gaming") == 0) {
        params = StabilizerCore::get_preset_gaming();
    } else if (strcmp(preset_name, "streaming") == 0) {
        params = StabilizerCore::get_preset_streaming();
    } else if (strcmp(preset_name, "recording") == 0) {
        params = StabilizerCore::get_preset_recording();
    } else {
        return;
    }

    params_to_settings(params, settings);
}

bool preset_changed_callback(obs_properties_t *props, obs_property_t *property,
                             obs_data_t *settings)
{
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(property);

    const char* preset = obs_data_get_string(settings, "preset");
    if (!preset || strlen(preset) == 0) {
        return true;
    }

    if (strcmp(preset, "custom") != 0) {
        apply_preset(settings, preset);
    }
    return true;
}

bool save_custom_preset(obs_properties_t *props, obs_property_t *property,
                        void *data)
{
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(property);
    try {
        auto *context = static_cast<struct stabilizer_filter *>(data);
        if (!context) {
            return false;
        }
        obs_data_t *settings = obs_data_create();
        params_to_settings(context->params, settings);

        const char *name = obs_data_get_string(settings, "custom_preset_name");
        if (!name || strlen(name) == 0) {
            blog(LOG_WARNING, "[obs-stabilizer] Custom preset name is empty");
            obs_data_release(settings);
            return false;
        }

        const bool saved = STABILIZER_PRESETS::PresetManager::save_preset(
            name, context->params);
        if (saved) {
            blog(LOG_INFO, "[obs-stabilizer] Saved custom preset '%s'", name);
        } else {
            blog(LOG_ERROR, "[obs-stabilizer] Failed to save custom preset '%s'", name);
        }
        obs_data_release(settings);
        return saved;
    } catch (const std::exception& e) {
        blog(LOG_ERROR, "[obs-stabilizer] Exception in save custom preset: %s", e.what());
        return false;
    }
}

bool load_custom_preset(obs_properties_t *props, obs_property_t *property,
                        void *data)
{
    UNUSED_PARAMETER(property);
    try {
        auto *context = static_cast<struct stabilizer_filter *>(data);
        if (!context) {
            return false;
        }
        obs_data_t *settings = obs_data_create();
        params_to_settings(context->params, settings);

        const char *name = obs_data_get_string(settings, "custom_preset_name");
        if (!name || strlen(name) == 0) {
            blog(LOG_WARNING, "[obs-stabilizer] Custom preset name is empty");
            obs_data_release(settings);
            return false;
        }

        StabilizerParams loaded;
        const bool loaded_ok = STABILIZER_PRESETS::PresetManager::load_preset(name, loaded);
        if (loaded_ok) {
            context->params = loaded;
            obs_data_t *updated = obs_data_create();
            params_to_settings(loaded, updated);
            obs_data_set_string(updated, "preset", "custom");
            obs_properties_apply_settings(props, updated);
            obs_data_release(updated);
            blog(LOG_INFO, "[obs-stabilizer] Loaded custom preset '%s'", name);
        } else {
            blog(LOG_ERROR, "[obs-stabilizer] Failed to load custom preset '%s'", name);
        }
        obs_data_release(settings);
        return loaded_ok;
    } catch (const std::exception& e) {
        blog(LOG_ERROR, "[obs-stabilizer] Exception in load custom preset: %s", e.what());
        return false;
    }
}

obs_properties_t *build_stabilizer_properties(struct stabilizer_filter *context)
{
    try {
        obs_properties_t *props = obs_properties_create();

        obs_properties_add_bool(props, "enabled", "Enable Stabilization");

        obs_property_t* preset_list = obs_properties_add_list(props, "preset", "Preset",
                                                            OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
        obs_property_list_add_string(preset_list, "Gaming", "gaming");
        obs_property_list_add_string(preset_list, "Streaming", "streaming");
        obs_property_list_add_string(preset_list, "Recording", "recording");
        obs_property_list_add_string(preset_list, "Custom", "custom");
        obs_property_set_modified_callback(preset_list, preset_changed_callback);

        // Custom preset management (Issue #302)
        obs_properties_add_text(props, "custom_preset_name", "Custom Preset Name",
                                OBS_TEXT_DEFAULT);
        obs_properties_add_button(props, "save_custom_preset", "Save Custom Preset",
                                  save_custom_preset);
        obs_properties_add_button(props, "load_custom_preset", "Load Custom Preset",
                                  load_custom_preset);

        obs_properties_add_int_slider(props, "smoothing_radius", "Smoothing Radius", 5, 200, 1);
        obs_properties_add_float_slider(props, "max_correction", "Max Correction (%)", 1.0, 100.0, 0.5);
        obs_properties_add_int_slider(props, "feature_count", "Feature Count", 50, 2000, 10);
        obs_properties_add_float_slider(props, "quality_level", "Quality Level", 0.001, 0.1, 0.001);
        obs_properties_add_float_slider(props, "min_distance", "Min Distance", 1.0, 200.0, 1.0);
        obs_properties_add_int_slider(props, "block_size", "Block Size", 3, 31, 2);

        obs_property_t* edge_mode = obs_properties_add_list(props, "edge_handling",
                        "Edge Handling",
                        OBS_COMBO_TYPE_LIST,
                        OBS_COMBO_FORMAT_STRING);
        obs_property_list_add_string(edge_mode, "Black Padding", "padding");
        obs_property_list_add_string(edge_mode, "Crop Borders", "crop");
        obs_property_list_add_string(edge_mode, "Scale to Fit", "scale");

        // Trajectory smoothing strategy (Issue #303)
        obs_property_t* smoothing_mode = obs_properties_add_list(props, "smoothing_mode",
                        "Smoothing Mode",
                        OBS_COMBO_TYPE_LIST,
                        OBS_COMBO_FORMAT_STRING);
        obs_property_list_add_string(smoothing_mode, "Moving Average", "moving_average");
        obs_property_list_add_string(smoothing_mode, "Kalman", "kalman");

        obs_properties_add_bool(props, "use_harris", "Use Harris Detector");
        obs_properties_add_float_slider(props, "k", "Harris K Parameter", 0.01, 0.1, 0.001);
        obs_properties_add_bool(props, "debug_mode", "Debug Mode");

        // Performance metrics (Issue #304)
        if (context && context->frame_count > 0) {
            char metrics_text[256];
            snprintf(metrics_text, sizeof(metrics_text),
                     "Frames processed: %llu\n"
                     "Avg processing: %.2f ms/frame",
                     static_cast<unsigned long long>(context->frame_count),
                     context->avg_processing_time);
            obs_properties_add_text(props, "metrics_info", "Performance",
                                    OBS_TEXT_INFO);
            obs_property_t *metrics_prop =
                obs_properties_get(props, "metrics_info");
            if (metrics_prop) {
                obs_property_set_long_description(metrics_prop, metrics_text);
            }
        }

        return props;
    } catch (const std::exception& e) {
        blog(LOG_ERROR, "[obs-stabilizer] Exception in get properties: %s", e.what());
        return obs_properties_create();
    }
}

void set_stabilizer_defaults(obs_data_t *settings)
{
    try {
        StabilizerParams default_params = StabilizerCore::get_preset_streaming();
        params_to_settings(default_params, settings);
        obs_data_set_default_string(settings, "preset", "streaming");
        obs_data_set_default_string(settings, "smoothing_mode", "moving_average");
        obs_data_set_default_string(settings, "edge_handling", "padding");
    } catch (const std::exception& e) {
        blog(LOG_ERROR, "[obs-stabilizer] Exception in get defaults: %s", e.what());
    }
}

#endif // HAVE_OBS_HEADERS
