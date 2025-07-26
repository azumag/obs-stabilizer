/*
OBS Stabilizer Plugin - OBS Studio Integration Layer
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include <obs-module.h>
#include "../core/stabilizer_core.hpp"

#ifdef ENABLE_STABILIZATION
#include <opencv2/opencv.hpp>
#endif

namespace obs_stabilizer {

// OBS filter data structure
struct StabilizerFilter {
    obs_source_t* source;
    bool enabled;
    
    // Core stabilization engine
    std::unique_ptr<StabilizerCore> stabilizer_core;
    
    // Configuration
    StabilizerConfig config;
    
    // Constructor/Destructor
    StabilizerFilter();
    ~StabilizerFilter();
    
    // Configuration methods
    void update_settings(obs_data_t* settings);
    void set_default_settings(obs_data_t* settings);
    
    // Preset configuration management
    void apply_preset_configuration(StabilizerConfig::PresetMode preset);
};

// OBS Integration Class
class OBSIntegration {
public:
    // Plugin lifecycle
    static bool plugin_load();
    static void plugin_unload();
    
    // Filter callbacks
    static const char* filter_get_name(void* unused);
    static void* filter_create(obs_data_t* settings, obs_source_t* source);
    static void filter_destroy(void* data);
    static struct obs_source_frame* filter_video(void* data, struct obs_source_frame* frame);
    static void filter_update(void* data, obs_data_t* settings);
    static obs_properties_t* filter_properties(void* data);
    static void filter_defaults(obs_data_t* settings);
    
    // Frame transformation utilities
    static void apply_transform_to_frame(struct obs_source_frame* frame, const TransformMatrix& transform);
    
private:
    // Format-specific transformation methods
    static void apply_transform_nv12(struct obs_source_frame* frame, const TransformMatrix& transform);
    static void apply_transform_i420(struct obs_source_frame* frame, const TransformMatrix& transform);
    
    // Validation utilities
    static bool validate_frame_data(struct obs_source_frame* frame);
    static bool validate_transform_matrix(const TransformMatrix& transform);
};

} // namespace obs_stabilizer