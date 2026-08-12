#pragma once

#ifdef HAVE_OBS_HEADERS
#include "obs_compat.h"

#include "ui/stabilizer_filter_context.hpp"

/**
 * Shared OBS settings <-> StabilizerParams conversion used by both the
 * properties panel and the filter update path.
 */
StabilizerCore::StabilizerParams settings_to_params(const obs_data_t *settings);
void params_to_settings(const StabilizerCore::StabilizerParams& params,
                        obs_data_t *settings);

/**
 * Build the OBS properties panel for the stabilizer filter.
 *
 * Owns all property creation, preset selection, and custom preset
 * save/load callbacks so stabilizer_opencv.cpp stays focused on the
 * OBS filter lifecycle and frame processing (Issue #301/#314).
 */
obs_properties_t *build_stabilizer_properties(struct stabilizer_filter *context);

/**
 * Populate OBS settings defaults for the stabilizer filter.
 */
void set_stabilizer_defaults(obs_data_t *settings);
#endif // HAVE_OBS_HEADERS
