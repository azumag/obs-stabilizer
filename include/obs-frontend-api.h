#pragma once

#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

void obs_frontend_source_stabilize(obs_source_t *source);
void obs_frontend_source_stabilization_stop(obs_source_t *source);

#ifdef __cplusplus
}
#endif
