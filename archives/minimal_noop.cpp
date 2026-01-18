#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("minimal-noop", "en-US")

static const char *noop_filter_get_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "No-Op Filter";
}

static void *noop_filter_create(obs_data_t *settings, obs_source_t *source)
{
    UNUSED_PARAMETER(settings);
    UNUSED_PARAMETER(source);
    return (void*)1; // Return non-null
}

static void noop_filter_destroy(void *data)
{
    UNUSED_PARAMETER(data);
}

static struct obs_source_frame *noop_filter_video(void *data, struct obs_source_frame *frame)
{
    UNUSED_PARAMETER(data);
    return frame; // Pass through unchanged
}

static struct obs_source_info noop_filter = {
    "noop_filter",
    OBS_SOURCE_TYPE_FILTER,
    OBS_SOURCE_VIDEO,
    noop_filter_get_name,
    noop_filter_create,
    noop_filter_destroy,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    noop_filter_video,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

bool obs_module_load(void)
{
    obs_register_source(&noop_filter);
    return true;
}
