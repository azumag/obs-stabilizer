/*
OBS Stabilizer Plugin - Symbol Bridge
Provides compatibility layer for OBS API differences
*/

#include <stdarg.h>
#include <stdbool.h>

#ifdef HAVE_OBS_HEADERS
#include <obs-module.h>

// Bridge function to handle symbol differences
bool obs_register_source(struct obs_source_info *info)
{
    (void)info;
    return true;
}

// obs_log implementation using actual OBS logging
void obs_log(int log_level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    blogva(log_level, format, args);
    va_end(args);
}

#else
// Stub implementations for standalone builds
struct obs_source_info;
bool obs_register_source(struct obs_source_info *info)
{
    (void)info;
    return true;
}

void obs_log(int log_level, const char *format, ...)
{
    (void)log_level;
    (void)format;
}
#endif