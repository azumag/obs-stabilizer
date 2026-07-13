#pragma once

#include <array>
#include <string_view>

namespace STABILIZER_UI {

enum class PropertyKind {
    Boolean,
    IntegerSlider,
    FloatSlider,
    StringList,
};

struct PropertyDescriptor {
    std::string_view key;
    std::string_view label;
    PropertyKind kind;
};

inline constexpr std::array<PropertyDescriptor, 11> kStabilizerProperties{{
    {"enabled", "Enable Stabilization", PropertyKind::Boolean},
    {"preset", "Preset", PropertyKind::StringList},
    {"smoothing_radius", "Smoothing Radius", PropertyKind::IntegerSlider},
    {"max_correction", "Max Correction (%)", PropertyKind::FloatSlider},
    {"feature_count", "Feature Count", PropertyKind::IntegerSlider},
    {"quality_level", "Quality Level", PropertyKind::FloatSlider},
    {"min_distance", "Min Distance", PropertyKind::FloatSlider},
    {"block_size", "Block Size", PropertyKind::IntegerSlider},
    {"edge_handling", "Edge Handling", PropertyKind::StringList},
    {"use_harris", "Use Harris Detector", PropertyKind::Boolean},
    {"debug_mode", "Debug Mode", PropertyKind::Boolean},
}};

constexpr const PropertyDescriptor* find_property(std::string_view key) noexcept
{
    for (const auto& property : kStabilizerProperties) {
        if (property.key == key) {
            return &property;
        }
    }
    return nullptr;
}

constexpr bool has_unique_property_keys() noexcept
{
    for (std::size_t left = 0; left < kStabilizerProperties.size(); ++left) {
        for (std::size_t right = left + 1; right < kStabilizerProperties.size(); ++right) {
            if (kStabilizerProperties[left].key == kStabilizerProperties[right].key) {
                return false;
            }
        }
    }
    return true;
}

static_assert(has_unique_property_keys(), "OBS property keys must be unique");

} // namespace STABILIZER_UI
