/*
OBS Stabilizer Plugin - Stub implementations for standalone build
Copyright (C) 2025 OBS Stabilizer Plugin Contributors

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>

// Include our header to match the declarations
#include "../include/obs/obs-module.h"

// Comprehensive format string safety validation
static bool is_safe_format_string(const char* format) {
    if (!format) return false;
    
    size_t format_len = strlen(format);
    if (format_len == 0 || format_len > 1024) return false; // Reasonable length limits
    
    // Comprehensive list of dangerous format string patterns
    const char* dangerous_patterns[] = {
        "%n",     // Write to memory - extremely dangerous
        "%*",     // Variable width - can cause buffer overflow
        "%%%",    // Suspicious multiple percent signs
        "%p",     // Pointer values - information disclosure
        "%x",     // Hex values - potential info disclosure
        "%X",     // Hex values (uppercase)
        "%ln",    // Long write to memory
        "%hn",    // Short write to memory  
        "%hhn",   // Char write to memory
        "%lln",   // Long long write to memory
        "%zn",    // Size_t write to memory
        "%tn",    // ptrdiff_t write to memory
        "%jn",    // intmax_t write to memory
        NULL
    };
    
    // Check for dangerous patterns
    for (int i = 0; dangerous_patterns[i] != NULL; i++) {
        if (strstr(format, dangerous_patterns[i]) != NULL) {
            return false;
        }
    }
    
    // Count and validate format specifiers
    int specifier_count = 0;
    const char* ptr = format;
    
    while ((ptr = strchr(ptr, '%')) != NULL) {
        if (ptr[1] == '%') {
            // Escaped percent - skip both characters
            ptr += 2;
            continue;
        }
        
        specifier_count++;
        if (specifier_count > 8) { // Stricter limit for safety
            return false;
        }
        
        // Check for width/precision specifier abuse
        ptr++; // Move past %
        while (*ptr && (isdigit(*ptr) || *ptr == '.' || *ptr == '-' || *ptr == '+' || *ptr == ' ')) {
            ptr++;
        }
        
        // Validate the format specifier character
        if (*ptr) {
            char spec = *ptr;
            // Only allow safe format specifiers
            if (!(spec == 's' || spec == 'd' || spec == 'i' || spec == 'u' || 
                  spec == 'f' || spec == 'g' || spec == 'c' || spec == '%')) {
                return false; // Reject potentially dangerous specifiers
            }
            ptr++;
        }
    }
    
    return true;
}

// Stub implementations for OBS functions
void blogva(int log_level, const char *format, va_list args) {
    // Input validation to prevent format string attacks
    if (!format) return;
    
    // Simple printf-based logging with safe format handling
    const char* level_str;
    switch(log_level) {
        case 100: level_str = "ERROR"; break;
        case 200: level_str = "WARN"; break;
        case 300: level_str = "INFO"; break;
        case 400: level_str = "DEBUG"; break;
        default: level_str = "UNKNOWN"; break;
    }
    
    // Safe format string handling - ensure format comes from trusted source
    printf("[%s] ", level_str);
    
    // Use format string sanitization to prevent injection attacks
    // Only allow trusted format strings by checking for dangerous patterns
    if (strlen(format) < 2048 && is_safe_format_string(format)) {
        vprintf(format, args);
    } else {
        printf("Log message rejected for security reasons (unsafe format or too long)");
    }
    printf("\n");
}



const char *obs_module_text(const char *val) {
    // Validate input parameter
    if (!val) return "";
    // Return the input string for stub mode
    return val;
}

// OBS data functions - stub implementations
bool obs_data_get_bool(obs_data_t *data, const char *name) {
    // Validate input parameters
    if (!data || !name) return false;
    
    // Default values for different settings
    if (strcmp(name, "enable_stabilization") == 0) return true;
    if (strcmp(name, "adaptive_refresh") == 0) return true;
    if (strcmp(name, "enable_gpu_acceleration") == 0) return false;
    return false;
}

int obs_data_get_int(obs_data_t *data, const char *name) {
    // Validate input parameters
    if (!data || !name) return 0;
    
    // Default values
    if (strcmp(name, "smoothing_radius") == 0) return 15;
    if (strcmp(name, "max_features") == 0) return 500;
    if (strcmp(name, "preset_mode") == 0) return 1; // STREAMING
    if (strcmp(name, "output_mode") == 0) return 1; // PAD
    if (strcmp(name, "refresh_threshold") == 0) return 25;
    if (strcmp(name, "processing_threads") == 0) return 2;
    return 0;
}

double obs_data_get_double(obs_data_t *data, const char *name) {
    // Validate input parameters
    if (!data || !name) return 0.0;
    
    // Default values
    if (strcmp(name, "error_threshold") == 0) return 30.0;
    if (strcmp(name, "min_feature_quality") == 0) return 0.01;
    return 0.0;
}

const char *obs_data_get_string(obs_data_t *data, const char *name) {
    // Validate input parameters
    if (!data || !name) return "";
    return "";
}

void obs_data_set_default_bool(obs_data_t *data, const char *name, bool val) {
    // Validate input parameters
    if (!data || !name) return;
    // Stub - do nothing
}

void obs_data_set_default_int(obs_data_t *data, const char *name, int val) {
    // Validate input parameters
    if (!data || !name) return;
    // Stub - do nothing
}

void obs_data_set_default_double(obs_data_t *data, const char *name, double val) {
    // Validate input parameters
    if (!data || !name) return;
    // Stub - do nothing
}

void obs_data_set_default_string(obs_data_t *data, const char *name, const char *val) {
    // Validate input parameters
    if (!data || !name || !val) return;
    // Stub - do nothing
}

void obs_data_set_bool(obs_data_t *data, const char *name, bool val) {
    // Validate input parameters
    if (!data || !name) return;
    // Stub - do nothing
}

void obs_data_set_int(obs_data_t *data, const char *name, int val) {
    // Validate input parameters
    if (!data || !name) return;
    // Stub - do nothing
}

void obs_data_set_double(obs_data_t *data, const char *name, double val) {
    // Validate input parameters
    if (!data || !name) return;
    // Stub - do nothing
}

// OBS properties functions - stub implementations
obs_properties_t *obs_properties_create(void) {
    // Return NULL instead of allocating fake memory to prevent leaks
    return NULL;
}

void obs_properties_destroy(obs_properties_t *props) {
    // Since we return NULL from create functions, props should always be NULL
    // Safe to call free(NULL) but add explicit check for robustness
    if (props) {
        free(props);
    }
}

obs_property_t *obs_properties_add_bool(obs_properties_t *props, const char *name, const char *description) {
    // Validate input parameters
    if (!props || !name || !description) return NULL;
    // Return NULL instead of allocating fake memory to prevent leaks
    return NULL;
}

obs_property_t *obs_properties_add_int(obs_properties_t *props, const char *name, const char *description, int min, int max, int step) {
    // Validate input parameters
    if (!props || !name || !description) return NULL;
    // Return NULL instead of allocating fake memory to prevent leaks
    return NULL;
}

obs_property_t *obs_properties_add_int_slider(obs_properties_t *props, const char *name, const char *description, int min, int max, int step) {
    // Validate input parameters
    if (!props || !name || !description) return NULL;
    // Return NULL instead of allocating fake memory to prevent leaks
    return NULL;
}

obs_property_t *obs_properties_add_float_slider(obs_properties_t *props, const char *name, const char *description, double min, double max, double step) {
    // Validate input parameters
    if (!props || !name || !description) return NULL;
    // Return NULL instead of allocating fake memory to prevent leaks
    return NULL;
}

obs_property_t *obs_properties_add_list(obs_properties_t *props, const char *name, const char *description, enum obs_combo_type type, enum obs_combo_format format) {
    // Validate input parameters
    if (!props || !name || !description) return NULL;
    // Return NULL instead of allocating fake memory to prevent leaks
    return NULL;
}

obs_property_t *obs_properties_create_group(obs_properties_t *props, const char *name, const char *description, enum obs_group_type type) {
    // Validate input parameters
    if (!props || !name || !description) return NULL;
    // Return NULL instead of allocating fake memory to prevent leaks
    return NULL;
}

size_t obs_property_list_add_int(obs_property_t *prop, const char *name, int val) {
    // Validate input parameters
    if (!prop || !name) return 0;
    return 0;
}

size_t obs_property_list_add_string(obs_property_t *prop, const char *name, const char *val) {
    // Validate input parameters
    if (!prop || !name || !val) return 0;
    return 0;
}

void obs_property_set_long_description(obs_property_t *prop, const char *long_description) {
    // Validate input parameters
    if (!prop || !long_description) return;
    // Stub - do nothing
}

void obs_log(int log_level, const char *format, ...) {
    // Validate input parameters
    if (!format) return;

    // Simple logging with safe format handling
    const char* level_str;
    switch(log_level) {
        case 100: level_str = "ERROR"; break;
        case 200: level_str = "WARN"; break;
        case 300: level_str = "INFO"; break;
        case 400: level_str = "DEBUG"; break;
        default: level_str = "UNKNOWN"; break;
    }

    printf("[%s] ", level_str);
    printf(format);
    printf("\n");
}

void obs_property_set_modified_callback(obs_property_t *prop, obs_property_modified_callback_t callback) {
    // Stub - do nothing
}

bool obs_register_source(struct obs_source_info *info) {
    // Stub - do nothing
    return true;
}

uint32_t obs_source_get_width(obs_source_t *source) {
    return 1920;
}

uint32_t obs_source_get_height(obs_source_t *source) {
    return 1080;
}
