/******************************************************************************
    Minimal OBS Plugin Header for Standalone Compilation
    This file provides only the essential OBS API definitions needed for plugin compilation.
    All OBS API calls will be resolved at runtime when the plugin is loaded by OBS.

    This approach uses -undefined dynamic_lookup linking, which allows compilation
    without linking against OBS, while the actual symbols are resolved by OBS at load time.
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Basic types */
typedef void* obs_source_t;
typedef void* obs_data_t;
typedef void* obs_properties_t;
typedef void* obs_property_t;
typedef void* obs_module_t;
typedef void* gs_effect_t;
typedef void* calldata_t;
typedef int64_t profiler_name_store_t;
typedef void* lookup_t;
typedef void* audio_t;
typedef void* video_t;
typedef void* obs_data_array_t;

/* Video format constants */
#define VIDEO_FORMAT_NONE 0
#define VIDEO_FORMAT_I420 1
#define VIDEO_FORMAT_NV12 2
#define VIDEO_FORMAT_YVYU 3
#define VIDEO_FORMAT_YUY2 4
#define VIDEO_FORMAT_UYVY 5
#define VIDEO_FORMAT_RGBA 6
#define VIDEO_FORMAT_BGRA 7
#define VIDEO_FORMAT_BGRX 8
#define VIDEO_FORMAT_Y800 9
#define VIDEO_FORMAT_I40A 10
#define VIDEO_FORMAT_I42A 11
#define VIDEO_FORMAT_I422 12
#define VIDEO_FORMAT_I444 13
#define VIDEO_FORMAT_BGR3 14

/* Obs source frame structure */
typedef struct obs_source_frame {
    uint8_t *data[8];
    uint32_t linesize[8];
    uint32_t width, height;
    uint64_t timestamp;
    int format;
    float full_range;
    uint8_t flip;
    uint32_t flags;
} obs_source_frame;

/* Source type */
typedef enum obs_source_type {
    OBS_SOURCE_TYPE_INPUT,
    OBS_SOURCE_TYPE_FILTER,
    OBS_SOURCE_TYPE_TRANSITION,
    OBS_SOURCE_TYPE_SCENE,
    OBS_SOURCE_TYPE_SCENEITEM,
} obs_source_type;

/* Color space */
typedef enum video_colorspace {
    VIDEO_CS_DEFAULT,
    VIDEO_CS_601,
    VIDEO_CS_709,
    VIDEO_CS_SRGB,
} video_colorspace;

/* Log level */
enum log_level {
    LOG_ERROR = 100,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
};

/* OBS logging macros */
#define blog(level, format, ...) printf("[OBS] " format "\n", ##__VA_ARGS__)
#define obs_log(level, format, ...) blog(level, format, ##__VA_ARGS__)

/* OBS source info structure */
struct obs_source_info {
    const char *id;
    enum obs_source_type type;
    uint32_t output_flags;

    const char *(*get_name)(void *type_data);
    void *(*create)(obs_data_t *settings, obs_source_t *source);
    void (*destroy)(void *data);
    void (*update)(void *data, obs_data_t *settings);

    void (*video_render)(void *data, gs_effect_t *effect);
    struct obs_source_frame *(*filter_video)(void *data, struct obs_source_frame *frame);

    obs_properties_t *(*get_properties)(void *data);
    void (*get_defaults)(obs_data_t *settings);
};

/* Obs data structure (minimal) */
struct obs_data_item {
    /* Minimal stub - actual structure is opaque */
};

/* Export attribute */
#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __attribute__((visibility("default")))
#endif

#define MODULE_EXPORT extern "C" EXPORT

/* OBS API functions (stubs - actual implementation provided by OBS) */
#define obs_log(level, format, ...) blog(level, format, ##__VA_ARGS__)
#define obs_properties_create() ((obs_properties_t*)0)
#define obs_properties_add_bool(props, name, desc) ((obs_property_t*)0)
#define obs_properties_add_int_slider(props, name, desc, min, max, step) ((obs_property_t*)0)
#define obs_properties_add_float_slider(props, name, desc, min, max, step) ((obs_property_t*)0)
#define obs_properties_add_list(props, name, desc, type, format) ((obs_property_t*)0)
#define obs_property_list_add_string(p, name, val) (void)0
#define obs_property_set_modified_callback(p, cb) (void)0
#define obs_data_set_bool(settings, name, val) (void)0
#define obs_data_set_int(settings, name, val) (void)0
#define obs_data_set_double(settings, name, val) (void)0
#define obs_data_set_string(settings, name, val) (void)0
#define obs_data_set_default_string(settings, name, val) (void)0
#define obs_data_get_bool(settings, name) (false)
#define obs_data_get_int(settings, name) (0)
#define obs_data_get_double(settings, name) (0.0)
#define obs_data_get_string(settings, name) ("")
#define obs_register_source(info) (0)
#define obs_source_get_width(source) (0)
#define obs_source_get_height(source) (0)
#define obs_data_create() ((obs_data_t*)0)
#define obs_data_create_from_json_file(path) ((obs_data_t*)0)
#define obs_data_save_json_safe(data, path, temp_ext, backup_ext) (false)
#define obs_data_release(data) (void)0

static inline const char* obs_get_config_path(const char* name) {
    (void)name;
    return "";
}

/* OBS source flags */
#define OBS_SOURCE_VIDEO (1 << 0)
#define OBS_SOURCE_AUDIO (1 << 1)
#define OBS_SOURCE_ASYNC (1 << 2)

/* Combo list types */
enum obs_combo_type {
    OBS_COMBO_TYPE_INVALID,
    OBS_COMBO_TYPE_EDITABLE,
    OBS_COMBO_TYPE_LIST,
};

enum obs_combo_format {
    OBS_COMBO_FORMAT_INVALID,
    OBS_COMBO_FORMAT_INT,
    OBS_COMBO_FORMAT_FLOAT,
    OBS_COMBO_FORMAT_STRING,
};

/* Unused parameter macro */
#define UNUSED_PARAMETER(x) (void)(x)

#ifdef __cplusplus
}
#endif
