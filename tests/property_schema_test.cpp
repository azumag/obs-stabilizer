#include "ui/stabilizer_property_schema.hpp"

#include <cassert>
#include <string_view>

int main()
{
    using namespace STABILIZER_UI;

    static_assert(kStabilizerProperties.size() == 11);
    static_assert(has_unique_property_keys());

    const auto* preset = find_property("preset");
    assert(preset != nullptr);
    assert(preset->label == std::string_view{"Preset"});
    assert(preset->kind == PropertyKind::StringList);

    const auto* smoothing = find_property("smoothing_radius");
    assert(smoothing != nullptr);
    assert(smoothing->kind == PropertyKind::IntegerSlider);

    const auto* quality = find_property("quality_level");
    assert(quality != nullptr);
    assert(quality->kind == PropertyKind::FloatSlider);

    assert(find_property("unknown_property") == nullptr);
    assert(find_property("") == nullptr);

    return 0;
}
