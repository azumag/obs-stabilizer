#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct obs_data obs_data_t;

// Basic data functions
obs_data_t *obs_data_create(void);
void obs_data_release(obs_data_t *data);

// Getters
long long obs_data_get_int(obs_data_t *data, const char *name);
double obs_data_get_double(obs_data_t *data, const char *name);
bool obs_data_get_bool(obs_data_t *data, const char *name);
const char *obs_data_get_string(obs_data_t *data, const char *name);

// Setters
void obs_data_set_int(obs_data_t *data, const char *name, long long val);
void obs_data_set_double(obs_data_t *data, const char *name, double val);
void obs_data_set_bool(obs_data_t *data, const char *name, bool val);
void obs_data_set_string(obs_data_t *data, const char *name, const char *val);
void obs_data_set_default_int(obs_data_t *data, const char *name, long long val);
void obs_data_set_default_double(obs_data_t *data, const char *name, double val);
void obs_data_set_default_bool(obs_data_t *data, const char *name, bool val);

#ifdef __cplusplus
}
#endif