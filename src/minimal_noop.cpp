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
    .id = "noop_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = noop_filter_get_name,
    .create = noop_filter_create,
    .destroy = noop_filter_destroy,
    .filter_video = noop_filter_video
};

bool obs_module_load(void)
{
    obs_register_source(&noop_filter);
    return true;
}
