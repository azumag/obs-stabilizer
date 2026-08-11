#pragma once

#include <optional>
#include <string_view>

namespace UI {

enum class BuiltInPreset {
    Gaming,
    Streaming,
    Recording,
};

/**
 * Resolves an OBS preset identifier into a built-in preset.
 *
 * Custom, empty, and unknown identifiers intentionally return std::nullopt so
 * callers can preserve the user's current settings instead of applying a
 * fallback preset implicitly.
 */
inline std::optional<BuiltInPreset> resolve_builtin_preset(std::string_view preset_name) noexcept
{
    if (preset_name == "gaming") {
        return BuiltInPreset::Gaming;
    }
    if (preset_name == "streaming") {
        return BuiltInPreset::Streaming;
    }
    if (preset_name == "recording") {
        return BuiltInPreset::Recording;
    }
    return std::nullopt;
}

inline bool should_apply_builtin_preset(std::string_view preset_name) noexcept
{
    return resolve_builtin_preset(preset_name).has_value();
}

} // namespace UI
