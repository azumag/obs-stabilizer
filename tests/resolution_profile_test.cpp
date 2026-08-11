#include "../src/core/resolution_profile.hpp"

#include <cassert>
#include <cstdint>
#include <limits>

using stabilizer::ResolutionTier;
using stabilizer::classify_resolution;
using stabilizer::make_resolution_profile;

int main() {
    assert(classify_resolution(0, 1080) == ResolutionTier::Invalid);
    assert(classify_resolution(1920, 0) == ResolutionTier::Invalid);
    assert(classify_resolution(std::numeric_limits<std::uint32_t>::max(), 2) ==
           ResolutionTier::Invalid);

    assert(classify_resolution(1280, 720) == ResolutionTier::Hd);
    assert(classify_resolution(1920, 1080) == ResolutionTier::FullHd);
    assert(classify_resolution(2560, 1440) == ResolutionTier::QuadHd);
    assert(classify_resolution(3840, 2160) == ResolutionTier::UltraHd);
    assert(classify_resolution(7680, 4320) == ResolutionTier::BeyondUltraHd);

    const auto ultra_hd = make_resolution_profile(3840, 2160);
    assert(ultra_hd.valid);
    assert(ultra_hd.tier == ResolutionTier::UltraHd);
    assert(ultra_hd.feature_count == 800);
    assert(ultra_hd.smoothing_radius == 30);
    assert(ultra_hd.detection_downsample == 2);

    const auto invalid = make_resolution_profile(0, 0);
    assert(!invalid.valid);
    assert(invalid.feature_count == 0);
    assert(invalid.smoothing_radius == 0);

    return 0;
}
