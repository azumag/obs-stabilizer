#pragma once

#include <cstdint>

#include "core/stabilizer_core.hpp"
#include "core/stabilizer_wrapper.hpp"
#include "core/rolling_average.hpp"

#ifdef HAVE_OBS_HEADERS
#include "obs_compat.h"

/**
 * Shared filter runtime state used by both the OBS integration layer and the
 * properties UI. Keeping the context in one header lets the UI module build
 * properties without depending on stabilizer_opencv.cpp internals (Issue #314).
 */
struct stabilizer_filter {
    obs_source_t *source;
    StabilizerWrapper stabilizer;
    bool initialized;
    StabilizerCore::StabilizerParams params;

    uint64_t frame_count;
    double avg_processing_time;
    STABILIZER_METRICS::RollingAverage processing_time_average;
};
#endif // HAVE_OBS_HEADERS
