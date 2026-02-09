# QA Report - Build Failure

## Issue

The project fails to build due to a type mismatch in the OBS API calls.

## Details

The `obs_data_get_*` functions are called with a `const obs_data_t *` as the first argument, but the function declarations in the OBS headers expect a non-const `obs_data_t *`.

This is a violation of the OBS API and needs to be fixed.

### Error Log

```
/Users/azumag/work/obs-stabilizer/src/stabilizer_opencv.cpp:304:22: error: no matching function for call to 'obs_data_get_bool'
  304 |     params.enabled = obs_data_get_bool(settings, "enabled");
      |                      ^~~~~~~~~~~~~~~~~
/Users/azumag/work/obs-stabilizer/include/obs/obs-module.h:132:6: note: candidate function not viable: 1st argument ('const obs_data_t *' (aka 'const obs_data *')) would lose const qualifier
  132 | bool obs_data_get_bool(obs_data_t *data, const char *name);
      |      ^                 ~~~~~~~~~~~~~~~~
... (and so on for other obs_data_get_* calls)
```
