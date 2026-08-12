#pragma once

#include <cstdint>
#include <limits>

namespace stabilizer {

/** Coarse resolution classes used to select stabilization defaults. */
enum class ResolutionTier {
    Invalid,
    Hd,
    FullHd,
    QuadHd,
    UltraHd,
    BeyondUltraHd,
};

/** Recommended processing parameters derived from input resolution. */
struct ResolutionProfile {
    /** Classified resolution tier. */
    ResolutionTier tier{ResolutionTier::Invalid};
    /** Recommended maximum feature count. */
    std::uint32_t feature_count{0};
    /** Recommended transform smoothing radius. */
    std::uint32_t smoothing_radius{0};
    /** Recommended feature-detection downsample factor. */
    std::uint32_t detection_downsample{1};
    /** True when width and height produced a usable profile. */
    bool valid{false};
};

/** Return whether multiplying two 32-bit dimensions would overflow. */
constexpr bool multiplication_overflows(std::uint32_t lhs, std::uint32_t rhs) noexcept {
    return rhs != 0 && lhs > std::numeric_limits<std::uint32_t>::max() / rhs;
}

/** Classify a frame geometry into a resolution tier. */
constexpr ResolutionTier classify_resolution(std::uint32_t width, std::uint32_t height) noexcept {
    if (width == 0 || height == 0 || multiplication_overflows(width, height)) {
        return ResolutionTier::Invalid;
    }

    const std::uint32_t pixels = width * height;
    if (pixels <= 1280u * 720u) {
        return ResolutionTier::Hd;
    }
    if (pixels <= 1920u * 1080u) {
        return ResolutionTier::FullHd;
    }
    if (pixels <= 2560u * 1440u) {
        return ResolutionTier::QuadHd;
    }
    if (pixels <= 3840u * 2160u) {
        return ResolutionTier::UltraHd;
    }
    return ResolutionTier::BeyondUltraHd;
}

/** Build the recommended stabilization profile for a frame geometry. */
constexpr ResolutionProfile make_resolution_profile(std::uint32_t width,
                                                    std::uint32_t height) noexcept {
    switch (classify_resolution(width, height)) {
    case ResolutionTier::Hd:
        return {ResolutionTier::Hd, 300, 15, 1, true};
    case ResolutionTier::FullHd:
        return {ResolutionTier::FullHd, 500, 20, 1, true};
    case ResolutionTier::QuadHd:
        return {ResolutionTier::QuadHd, 650, 24, 2, true};
    case ResolutionTier::UltraHd:
        return {ResolutionTier::UltraHd, 800, 30, 2, true};
    case ResolutionTier::BeyondUltraHd:
        return {ResolutionTier::BeyondUltraHd, 1000, 36, 4, true};
    case ResolutionTier::Invalid:
    default:
        return {};
    }
}

} // namespace stabilizer
