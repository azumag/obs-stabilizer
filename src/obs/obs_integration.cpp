/*
 * OBS Integration Implementation
 * Handles OBS Studio API integration and plugin lifecycle
 */

#include "obs_integration.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

// ============================================================================
// OBSIntegration Implementation
// ============================================================================

std::unique_ptr<OBSIntegration::OBSFilterData> OBSIntegration::create_filter_data(obs_data_t* settings, obs_source_t* source) {
    try {
        auto data = std::make_unique<OBSFilterData>();
        data->source = source;
        data->stabilizer = std::make_unique<StabilizerCore>();
        data->initialized = false;
        
        // Get initial parameters
        StabilizerCore::StabilizerParams params = settings_to_params(const_cast<obs_data_t*>(settings));
        
        // We'll initialize on first frame when we know the dimensions
        // For now, just validate parameters
        if (!StabilizerCore::validate_parameters(params)) {
            log_error("Invalid parameters provided during filter creation");
            return nullptr;
        }
        
        return data;
        
    } catch (const std::exception& e) {
        log_error("Exception in create_filter_data: " + std::string(e.what()));
        return nullptr;
    }
}

void OBSIntegration::destroy_filter_data(OBSFilterData* data) {
    delete data;
}

bool OBSIntegration::update_parameters(OBSFilterData* data, obs_data_t* settings) {
    if (!data || !data->stabilizer) {
        return false;
    }
    
    try {
        StabilizerCore::StabilizerParams params = settings_to_params(const_cast<obs_data_t*>(settings));
        data->stabilizer->update_parameters(params);
        return true;
        
    } catch (const std::exception& e) {
        log_error("Exception in update_parameters: " + std::string(e.what()));
        return false;
    }
}

obs_source_frame* OBSIntegration::process_video_frame(OBSFilterData* data, obs_source_frame* frame) {
    if (!data || !data->stabilizer || !frame) {
        return frame;
    }
    
    try {
        // Initialize stabilizer on first frame
        if (!data->initialized) {
            
            if (!data->stabilizer->initialize(frame->width, frame->height, data->stabilizer->get_current_params())) {
                log_error("Failed to initialize stabilizer: " + data->stabilizer->get_last_error());
                return frame;
            }
            
            data->initialized = true;
            log_info("Stabilizer initialized for " + std::to_string(frame->width) + "x" + std::to_string(frame->height));
        }
        
        // Convert OBS frame to OpenCV Mat
        cv::Mat cv_frame = obs_frame_to_cv_mat(frame);
        if (cv_frame.empty()) {
            log_error("Failed to convert OBS frame to OpenCV Mat");
            return frame;
        }
        
        // Process frame with stabilizer
        cv::Mat stabilized_frame = data->stabilizer->process_frame(cv_frame);
        
        // Convert back to OBS frame
        obs_source_frame* result = cv_mat_to_obs_frame(stabilized_frame, frame);
        
        return result ? result : frame;
        
    } catch (const std::exception& e) {
        log_error("Exception in process_video_frame: " + std::string(e.what()));
        return frame;
    }
}

void OBSIntegration::get_defaults(obs_data_t* settings) {
    // Use streaming preset as default
    StabilizerCore::StabilizerParams default_params = StabilizerCore::get_preset_streaming();
    params_to_settings(default_params, settings);
    
    obs_data_set_default_string(settings, "preset", "streaming");
}

obs_properties_t* OBSIntegration::get_properties(void* data) {
    obs_properties_t* props = obs_properties_create();
    
    // Basic properties
    obs_properties_add_bool(props, "enabled", "Enable Stabilization");
    
    // Preset selector
    obs_property_t* preset_list = obs_properties_add_list(props, "preset", "Preset", 
                                                          OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(preset_list, "Gaming", "gaming");
    obs_property_list_add_string(preset_list, "Streaming", "streaming");
    obs_property_list_add_string(preset_list, "Recording", "recording");
    obs_property_list_add_string(preset_list, "Custom", "custom");
    
    obs_property_set_modified_callback(preset_list, PresetHandler::preset_changed_callback);
    
    // Separator
    obs_properties_add_text(props, "sep1", "--- Basic Parameters ---");
    
    // Basic parameters
    obs_properties_add_int_slider(props, "smoothing_radius", "Smoothing Radius", 
                                  MIN_SMOOTHING_RADIUS, MAX_SMOOTHING_RADIUS, 1);
    obs_properties_add_float_slider(props, "max_correction", "Max Correction (%)", 
                                   MIN_MAX_CORRECTION, MAX_MAX_CORRECTION, 0.5);
    obs_properties_add_int_slider(props, "feature_count", "Feature Count", 
                                  MIN_FEATURE_COUNT, MAX_FEATURE_COUNT, 10);
    
    // Advanced parameters group
    obs_property_t* advanced_group = obs_properties_create_group(props, "advanced_group", 
                                                                  "Advanced Parameters", OBS_GROUP_CHECKABLE);
    
    obs_properties_add_float_slider(props, "quality_level", "Quality Level", 
                                     0.001, 0.1, 0.001);
    obs_properties_add_float_slider(props, "min_distance", "Min Distance", 
                                    1.0, 200.0, 1.0);
    obs_properties_add_int_slider(props, "block_size", "Block Size", 
                                   MIN_BLOCK_SIZE, MAX_BLOCK_SIZE, 2);
    
    obs_properties_add_bool(props, "use_harris", "Use Harris Detector");
    obs_properties_add_float_slider(props, "k", "Harris K Parameter", 
                                    MIN_HARRIS_K, MAX_HARRIS_K, 0.001);
    
    // Debug options
    obs_properties_add_bool(props, "debug_mode", "Debug Mode");
    
    return props;
}

const char* OBSIntegration::get_name(void* unused) {
    UNUSED_PARAMETER(unused);
    return FILTER_NAME;
}

enum obs_source_type OBSIntegration::get_type(void* unused) {
    UNUSED_PARAMETER(unused);
    return OBS_SOURCE_TYPE_FILTER;
}

void OBSIntegration::apply_preset(obs_data_t* settings, const char* preset_name) {
    StabilizerCore::StabilizerParams params;
    
    if (strcmp(preset_name, "gaming") == 0) {
        params = StabilizerCore::get_preset_gaming();
    } else if (strcmp(preset_name, "streaming") == 0) {
        params = StabilizerCore::get_preset_streaming();
    } else if (strcmp(preset_name, "recording") == 0) {
        params = StabilizerCore::get_preset_recording();
    } else {
        // Custom preset - don't change values
        return;
    }
    
    params_to_settings(params, settings);
}

StabilizerCore::StabilizerParams OBSIntegration::settings_to_params(obs_data_t* settings) {
    StabilizerCore::StabilizerParams params;
    
    params.enabled = OBSDataConverter::get_bool_safe(settings, "enabled", true);
    params.smoothing_radius = OBSDataConverter::get_int_safe(settings, "smoothing_radius", 30);
    params.max_correction = OBSDataConverter::get_double_safe(settings, "max_correction", 30.0);
    params.feature_count = OBSDataConverter::get_int_safe(settings, "feature_count", 500);
    params.quality_level = OBSDataConverter::get_double_safe(settings, "quality_level", 0.01);
    params.min_distance = OBSDataConverter::get_double_safe(settings, "min_distance", 30.0);
    params.block_size = OBSDataConverter::get_int_safe(settings, "block_size", 3);
    params.use_harris = OBSDataConverter::get_bool_safe(settings, "use_harris", false);
    params.k = OBSDataConverter::get_double_safe(settings, "k", 0.04);
    params.debug_mode = OBSDataConverter::get_bool_safe(settings, "debug_mode", false);
    
    return params;
}

void OBSIntegration::params_to_settings(const StabilizerCore::StabilizerParams& params, obs_data_t* settings) {
    obs_data_set_bool(settings, "enabled", params.enabled);
    OBSDataConverter::set_int_validated(settings, "smoothing_radius", params.smoothing_radius, 
                                       MIN_SMOOTHING_RADIUS, MAX_SMOOTHING_RADIUS);
    OBSDataConverter::set_double_validated(settings, "max_correction", params.max_correction, 
                                           MIN_MAX_CORRECTION, MAX_MAX_CORRECTION);
    OBSDataConverter::set_int_validated(settings, "feature_count", params.feature_count, 
                                       MIN_FEATURE_COUNT, MAX_FEATURE_COUNT);
    OBSDataConverter::set_double_validated(settings, "quality_level", params.quality_level, 
                                           0.001, 0.1);
    OBSDataConverter::set_double_validated(settings, "min_distance", params.min_distance, 
                                           1.0, 200.0);
    OBSDataConverter::set_int_validated(settings, "block_size", params.block_size, 
                                       MIN_BLOCK_SIZE, MAX_BLOCK_SIZE);
    obs_data_set_bool(settings, "use_harris", params.use_harris);
    OBSDataConverter::set_double_validated(settings, "k", params.k, 
                                           MIN_HARRIS_K, MAX_HARRIS_K);
    obs_data_set_bool(settings, "debug_mode", params.debug_mode);
}

// Private methods

cv::Mat OBSIntegration::obs_frame_to_cv_mat(const obs_source_frame* frame) {
    if (!frame || !frame->data[0]) {
        return cv::Mat();
    }
    
    try {
        cv::Mat mat;
        
        switch (frame->format) {
            case VIDEO_FORMAT_BGRA:
                mat = cv::Mat(frame->height, frame->width, CV_8UC4, frame->data[0], frame->linesize[0]);
                break;
                
            case VIDEO_FORMAT_BGRX:
                 mat = cv::Mat(frame->height, frame->width, CV_8UC4, frame->data[0], frame->linesize[0]);
                 // Convert X channel to Alpha
                 break;
                
            case VIDEO_FORMAT_BGR3:
                mat = cv::Mat(frame->height, frame->width, CV_8UC3, frame->data[0], frame->linesize[0]);
                break;
                
            case VIDEO_FORMAT_NV12:
                // Convert NV12 to BGRA
                {
                    cv::Mat yuv(frame->height + frame->height/2, frame->width, CV_8UC1, frame->data[0]);
                    cv::cvtColor(yuv, mat, cv::COLOR_YUV2BGRA_NV12);
                }
                break;
                
            case VIDEO_FORMAT_I420:
                // Convert I420 to BGRA
                {
                    cv::Mat yuv(frame->height + frame->height/2, frame->width, CV_8UC1, frame->data[0]);
                    cv::cvtColor(yuv, mat, cv::COLOR_YUV2BGRA_I420);
                }
                break;
                
            default:
                log_error("Unsupported frame format: " + std::to_string(frame->format));
                return cv::Mat();
        }
        
        return mat.clone();
        
    } catch (const cv::Exception& e) {
        log_error("OpenCV exception in obs_frame_to_cv_mat: " + std::string(e.what()));
        return cv::Mat();
    }
}

obs_source_frame* OBSIntegration::cv_mat_to_obs_frame(const cv::Mat& mat, const obs_source_frame* reference_frame) {
    if (mat.empty() || !reference_frame) {
        return nullptr;
    }
    
    try {
        static struct {
            std::vector<uint8_t> buffer;
            obs_source_frame frame;
            bool initialized;
        } frame_buffer = { {}, {}, false };

        if (!frame_buffer.initialized) {
            frame_buffer.frame = *reference_frame;
            frame_buffer.initialized = true;
        }

        frame_buffer.frame.width = reference_frame->width;
        frame_buffer.frame.height = reference_frame->height;
        frame_buffer.frame.format = reference_frame->format;
        frame_buffer.frame.timestamp = reference_frame->timestamp;

        cv::Mat converted;
        
        switch (reference_frame->format) {
            case VIDEO_FORMAT_BGRA:
                if (mat.channels() == 4) {
                    converted = mat;
                } else {
                    cv::cvtColor(mat, converted, cv::COLOR_BGR2BGRA);
                }
                break;
            case VIDEO_FORMAT_BGR3:
                if (mat.channels() == 3) {
                    converted = mat;
                } else {
                    cv::cvtColor(mat, converted, cv::COLOR_BGRA2BGR);
                }
                break;
            default:
                log_error("Unsupported output format: " + std::to_string(reference_frame->format));
                return nullptr;
        }
        
        size_t required_size = converted.total() * converted.elemSize();
        if (frame_buffer.buffer.size() < required_size) {
            frame_buffer.buffer.resize(required_size);
        }
        
        frame_buffer.frame.data[0] = frame_buffer.buffer.data();
        frame_buffer.frame.linesize[0] = static_cast<uint32_t>(converted.step);
        
        memcpy(frame_buffer.frame.data[0], converted.data, required_size);
        
        for (int i = 1; i < 8; i++) {
            frame_buffer.frame.data[i] = nullptr;
            frame_buffer.frame.linesize[i] = 0;
        }
        
        return &frame_buffer.frame;
        
    } catch (const std::exception& e) {
        log_error("Exception in cv_mat_to_obs_frame: " + std::string(e.what()));
        return nullptr;
    }
}

bool OBSIntegration::validate_frame_format(const obs_source_frame* frame) {
    if (!frame) {
        return false;
    }
    
    switch (frame->format) {
        case VIDEO_FORMAT_BGRA:
        case VIDEO_FORMAT_BGRX:
        case VIDEO_FORMAT_BGR3:
        case VIDEO_FORMAT_NV12:
        case VIDEO_FORMAT_I420:
            return frame->width > 0 && frame->height > 0 && frame->data[0] != nullptr;
            
        default:
            return false;
    }
}

bool OBSIntegration::validate_settings(const obs_data_t* settings) {
    if (!settings) {
        return false;
    }
    
    StabilizerCore::StabilizerParams params = settings_to_params(const_cast<obs_data_t*>(settings));
    return StabilizerCore::validate_parameters(params);
}

void OBSIntegration::log_error(const std::string& message) {
    obs_log(LOG_ERROR, "[OBSIntegration] %s", message.c_str());
}

void OBSIntegration::log_info(const std::string& message) {
    obs_log(LOG_INFO, "[OBSIntegration] %s", message.c_str());
}

// ============================================================================
// PresetHandler Implementation
// ============================================================================

bool PresetHandler::preset_changed_callback(void* priv, obs_properties_t* props, 
                                          obs_property_t* property, obs_data_t* settings) {
    UNUSED_PARAMETER(priv);
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(property);
    
    const char* preset = obs_data_get_string(settings, "preset");
    if (!preset || strlen(preset) == 0) {
        return true;
    }
    
    if (strcmp(preset, "custom") != 0) {
        OBSIntegration::apply_preset(settings, preset);
    }
    
    return true;
}

const char* PresetHandler::get_preset_list_value(int index) {
    switch (index) {
        case 0: return "gaming";
        case 1: return "streaming";
        case 2: return "recording";
        case 3: return "custom";
        default: return "custom";
    }
}

bool PresetHandler::is_valid_preset(const char* preset_name) {
    return strcmp(preset_name, "gaming") == 0 ||
           strcmp(preset_name, "streaming") == 0 ||
           strcmp(preset_name, "recording") == 0 ||
           strcmp(preset_name, "custom") == 0;
}

// ============================================================================
// OBSPerformanceMonitor Implementation
// ============================================================================

void OBSPerformanceMonitor::update_stats(PerformanceStats& stats, double processing_time) {
    stats.total_frames++;
    stats.avg_processing_time = (stats.avg_processing_time * (stats.total_frames - 1) + processing_time) / stats.total_frames;
    
    if (processing_time > CRITICAL_THRESHOLD_MS) {
        stats.dropped_frames++;
        stats.performance_warning = true;
    } else {
        stats.performance_warning = false;
    }
    
    stats.current_fps = 1000.0 / stats.avg_processing_time;
}

bool OBSPerformanceMonitor::is_performance_acceptable(const PerformanceStats& stats, double target_fps) {
    double target_time = 1000.0 / target_fps;
    return stats.avg_processing_time <= target_time && 
           stats.dropped_frames < stats.total_frames * 0.05; // Less than 5% dropped frames
}

std::string OBSPerformanceMonitor::get_performance_warning(const PerformanceStats& stats) {
    if (!stats.performance_warning) {
        return "";
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    if (stats.avg_processing_time > CRITICAL_THRESHOLD_MS) {
        oss << "Critical: Processing time (" << stats.avg_processing_time << "ms) exceeds threshold";
    } else if (stats.avg_processing_time > WARNING_THRESHOLD_MS) {
        oss << "Warning: Processing time (" << stats.avg_processing_time << "ms) approaching threshold";
    }
    
    if (stats.dropped_frames > 0) {
        double drop_rate = (double)stats.dropped_frames / stats.total_frames * 100.0;
        oss << ". Drop rate: " << drop_rate << "%";
    }
    
    return oss.str();
}

// ============================================================================
// OBSDataConverter Implementation
// ============================================================================

int OBSDataConverter::get_int_safe(const obs_data_t* data, const char* name, int default_value) {
    if (!data || !name) {
        return default_value;
    }
    
    return static_cast<int>(obs_data_get_int(const_cast<obs_data_t*>(data), name));
}

double OBSDataConverter::get_double_safe(const obs_data_t* data, const char* name, double default_value) {
    if (!data || !name) {
        return default_value;
    }
    
    return obs_data_get_double(const_cast<obs_data_t*>(data), name);
}

bool OBSDataConverter::get_bool_safe(const obs_data_t* data, const char* name, bool default_value) {
    if (!data || !name) {
        return default_value;
    }
    
    return obs_data_get_bool(const_cast<obs_data_t*>(data), name);
}

std::string OBSDataConverter::get_string_safe(obs_data_t* data, const char* name, const std::string& default_value) {
    if (!data || !name) {
        return default_value;
    }
    
    const char* value = obs_data_get_string(data, name);
    return value ? std::string(value) : default_value;
}

void OBSDataConverter::set_int_validated(obs_data_t* data, const char* name, int value, int min_value, int max_value) {
    if (!data || !name) {
        return;
    }
    
    int clamped_value = std::clamp(value, min_value, max_value);
    obs_data_set_int(data, name, clamped_value);
}

void OBSDataConverter::set_double_validated(obs_data_t* data, const char* name, double value, double min_value, double max_value) {
    if (!data || !name) {
        return;
    }
    
    double clamped_value = std::clamp(value, min_value, max_value);
    obs_data_set_double(data, name, clamped_value);
}