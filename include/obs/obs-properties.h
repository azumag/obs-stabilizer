#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct obs_properties obs_properties_t;
typedef struct obs_property obs_property_t;
typedef struct obs_data obs_data_t;

// Property types
enum obs_property_type {
    OBS_PROPERTY_INVALID,
    OBS_PROPERTY_BOOL,
    OBS_PROPERTY_INT,
    OBS_PROPERTY_FLOAT,
    OBS_PROPERTY_TEXT,
    OBS_PROPERTY_PATH,
    OBS_PROPERTY_LIST,
    OBS_PROPERTY_COLOR,
    OBS_PROPERTY_BUTTON,
    OBS_PROPERTY_FONT,
    OBS_PROPERTY_EDITABLE_LIST,
    OBS_PROPERTY_FRAME_RATE,
    OBS_PROPERTY_GROUP,
    OBS_PROPERTY_COLOR_ALPHA,
};

// Text type
enum obs_text_type {
    OBS_TEXT_DEFAULT,
    OBS_TEXT_PASSWORD,
    OBS_TEXT_MULTILINE,
};

// List type
enum obs_combo_type {
    OBS_COMBO_TYPE_INVALID,
    OBS_COMBO_TYPE_EDITABLE,
    OBS_COMBO_TYPE_LIST,
};

// List format
enum obs_combo_format {
    OBS_COMBO_FORMAT_INVALID,
    OBS_COMBO_FORMAT_INT,
    OBS_COMBO_FORMAT_FLOAT,
    OBS_COMBO_FORMAT_STRING,
};

// Number type
enum obs_number_type {
    OBS_NUMBER_SCROLLER,
    OBS_NUMBER_SLIDER,
};

// Property functions
obs_properties_t *obs_properties_create(void);
void obs_properties_destroy(obs_properties_t *props);

obs_property_t *obs_properties_add_bool(obs_properties_t *props, const char *name, const char *description);
obs_property_t *obs_properties_add_int(obs_properties_t *props, const char *name, const char *description, int min, int max, int step);
obs_property_t *obs_properties_add_float(obs_properties_t *props, const char *name, const char *description, double min, double max, double step);
obs_property_t *obs_properties_add_text(obs_properties_t *props, const char *name, const char *description, enum obs_text_type type);
obs_property_t *obs_properties_add_list(obs_properties_t *props, const char *name, const char *description, enum obs_combo_type type, enum obs_combo_format format);

// List item functions
size_t obs_property_list_add_string(obs_property_t *p, const char *name, const char *val);
size_t obs_property_list_add_int(obs_property_t *p, const char *name, long long val);

// Property modification
void obs_property_set_visible(obs_property_t *p, bool visible);
void obs_property_set_enabled(obs_property_t *p, bool enabled);
void obs_property_set_description(obs_property_t *p, const char *description);

// Number property modification  
void obs_property_int_set_limits(obs_property_t *p, int min, int max, int step);
void obs_property_float_set_limits(obs_property_t *p, double min, double max, double step);
void obs_property_int_set_type(obs_property_t *p, enum obs_number_type type);
void obs_property_float_set_type(obs_property_t *p, enum obs_number_type type);

#ifdef __cplusplus
}
#endif