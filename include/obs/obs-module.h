#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

// Basic OBS types
typedef struct obs_source obs_source_t;
typedef struct obs_data obs_data_t;
typedef struct obs_properties obs_properties_t;
typedef struct obs_property obs_property_t;
typedef struct gs_effect gs_effect_t;
typedef void (*obs_source_enum_proc_t)(obs_source_t *parent, obs_source_t *child, void *param);

// Property/combo enums
enum obs_combo_type {
    OBS_COMBO_TYPE_INVALID,
    OBS_COMBO_TYPE_EDITABLE,
    OBS_COMBO_TYPE_LIST,
    OBS_COMBO_TYPE_RADIO
};

enum obs_combo_format {
    OBS_COMBO_FORMAT_INVALID,
    OBS_COMBO_FORMAT_INT,
    OBS_COMBO_FORMAT_FLOAT,
    OBS_COMBO_FORMAT_STRING
};

enum obs_group_type {
    OBS_GROUP_NORMAL,
    OBS_GROUP_CHECKABLE
};

// Utility macros
#define UNUSED_PARAMETER(param) ((void)(param))

// Source type enumeration
enum obs_source_type {
    OBS_SOURCE_TYPE_INPUT,
    OBS_SOURCE_TYPE_FILTER,
    OBS_SOURCE_TYPE_TRANSITION,
    OBS_SOURCE_TYPE_SCENE
};

// Log level enumeration
enum log_type {
    LOG_ERROR = 100,
    LOG_WARNING = 200,
    LOG_INFO = 300,
    LOG_DEBUG = 400
};

// Video format types
enum video_format {
    VIDEO_FORMAT_NONE,
    VIDEO_FORMAT_I420,
    VIDEO_FORMAT_NV12,
    VIDEO_FORMAT_YVYU,
    VIDEO_FORMAT_YUY2,
    VIDEO_FORMAT_UYVY,
    VIDEO_FORMAT_RGBA,
    VIDEO_FORMAT_BGRA,
    VIDEO_FORMAT_BGRX,
    VIDEO_FORMAT_Y800,
    VIDEO_FORMAT_I444,
    VIDEO_FORMAT_BGR3,
    VIDEO_FORMAT_I422,
    VIDEO_FORMAT_I40A,
    VIDEO_FORMAT_I42A,
    VIDEO_FORMAT_YUVA,
    VIDEO_FORMAT_AYUV,
};

// Video frame structure
struct obs_source_frame {
    uint8_t *data[8];
    uint32_t linesize[8];
    uint32_t width;
    uint32_t height;
    uint64_t timestamp;
    enum video_format format;
    float color_matrix[16];
    bool full_range;
    float color_range_min[3];
    float color_range_max[3];
    bool flip;
};

// Source info structure
struct obs_source_info {
    const char *id;
    enum obs_source_type type;
    uint32_t output_flags;
    const char *(*get_name)(void *unused);
    void *(*create)(obs_data_t *settings, obs_source_t *source);
    void (*destroy)(void *data);
    uint32_t (*get_width)(void *data);
    uint32_t (*get_height)(void *data);
    void (*update)(void *data, obs_data_t *settings);
    void (*activate)(void *data);
    void (*deactivate)(void *data);
    void (*show)(void *data);
    void (*hide)(void *data);
    void (*video_tick)(void *data, float seconds);
    void (*video_render)(void *data, gs_effect_t *effect);
    struct obs_source_frame *(*filter_video)(void *data, struct obs_source_frame *frame);
    obs_properties_t *(*get_properties)(void *data);
    void (*get_defaults)(obs_data_t *settings);
    void (*save)(void *data, obs_data_t *settings);
    void (*load)(void *data, obs_data_t *settings);
    void (*enum_active_sources)(void *data, obs_source_enum_proc_t enum_callback, void *param);
    void (*enum_all_sources)(void *data, obs_source_enum_proc_t enum_callback, void *param);
    uint32_t (*get_output_flags)(void *data);
};

// Add missing includes
#include <stddef.h>

// Function declarations
void obs_log(int log_level, const char *format, ...);
void blogva(int log_level, const char *format, va_list args);
bool obs_register_source(struct obs_source_info *info);
bool obs_register_source_s(struct obs_source_info *info, size_t size);
const char *obs_module_text(const char *val);

// OBS data functions
bool obs_data_get_bool(obs_data_t *data, const char *name);
int obs_data_get_int(obs_data_t *data, const char *name);
double obs_data_get_double(obs_data_t *data, const char *name);
const char *obs_data_get_string(obs_data_t *data, const char *name);

void obs_data_set_default_bool(obs_data_t *data, const char *name, bool val);
void obs_data_set_default_int(obs_data_t *data, const char *name, int val);
void obs_data_set_default_double(obs_data_t *data, const char *name, double val);
void obs_data_set_default_string(obs_data_t *data, const char *name, const char *val);

// Non-default setters
void obs_data_set_int(obs_data_t *data, const char *name, int val);
void obs_data_set_double(obs_data_t *data, const char *name, double val);
void obs_data_set_bool(obs_data_t *data, const char *name, bool val);
void obs_data_set_string(obs_data_t *data, const char *name, const char *val);

// OBS properties functions
obs_properties_t *obs_properties_create(void);
void obs_properties_destroy(obs_properties_t *props);
obs_property_t *obs_properties_add_bool(obs_properties_t *props, const char *name, const char *description);
obs_property_t *obs_properties_add_int(obs_properties_t *props, const char *name, const char *description, int min, int max, int step);
obs_property_t *obs_properties_add_int_slider(obs_properties_t *props, const char *name, const char *description, int min, int max, int step);
obs_property_t *obs_properties_add_float_slider(obs_properties_t *props, const char *name, const char *description, double min, double max, double step);
obs_property_t *obs_properties_add_list(obs_properties_t *props, const char *name, const char *description, enum obs_combo_type type, enum obs_combo_format format);
obs_property_t *obs_properties_create_group(obs_properties_t *props, const char *name, const char *description, enum obs_group_type type);
size_t obs_property_list_add_int(obs_property_t *prop, const char *name, int val);
size_t obs_property_list_add_string(obs_property_t *prop, const char *name, const char *val);
void obs_property_set_long_description(obs_property_t *prop, const char *long_description);
obs_property_t *obs_properties_add_text(obs_properties_t *props, const char *name, const char *text);

// Property callback function
typedef bool (*obs_property_modified_callback_t)(void *priv, obs_properties_t *props, obs_property_t *property, obs_data_t *settings);
void obs_property_set_modified_callback(obs_property_t *prop, obs_property_modified_callback_t callback);

// Source functions
uint32_t obs_source_get_width(obs_source_t *source);
uint32_t obs_source_get_height(obs_source_t *source);

// Output flags
#define OBS_SOURCE_VIDEO                (1<<0)
#define OBS_SOURCE_AUDIO                (1<<1)
#define OBS_SOURCE_ASYNC_VIDEO          (1<<2)
#define OBS_SOURCE_CUSTOM_DRAW          (1<<3)
#define OBS_SOURCE_INTERACTION          (1<<4)
#define OBS_SOURCE_COMPOSITE            (1<<5)
#define OBS_SOURCE_DO_NOT_DUPLICATE     (1<<6)
#define OBS_SOURCE_DEPRECATED           (1<<7)
#define OBS_SOURCE_DO_NOT_SELF_MONITOR  (1<<8)

// Function declarations
bool obs_module_load(void);
void obs_module_unload(void);

// Module export macros
#ifndef MODULE_EXPORT
    #ifdef _WIN32
        #define MODULE_EXPORT __declspec(dllexport)
    #else
        #define MODULE_EXPORT __attribute__((visibility("default")))
    #endif
#endif

// OBS Module macros
typedef struct obs_module obs_module_t;

#define OBS_DECLARE_MODULE() \
    static obs_module_t *obs_module_pointer; \
    extern "C" { \
        MODULE_EXPORT void obs_module_set_pointer(obs_module_t *module); \
        void obs_module_set_pointer(obs_module_t *module) \
        { \
            obs_module_pointer = module; \
        } \
        obs_module_t *obs_current_module(void) \
        { \
            return obs_module_pointer; \
        } \
    } \
    bool obs_module_load(void); \
    void obs_module_unload(void);

#define OBS_MODULE_USE_DEFAULT_LOCALE(module_name, default_locale) \
    extern "C" MODULE_EXPORT const char *obs_module_name(void) { return module_name; }

#ifdef __cplusplus
}
#endif