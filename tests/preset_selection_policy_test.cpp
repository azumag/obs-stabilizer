#include "ui/preset_selection.hpp"

#include <cassert>
#include <iostream>

int main()
{
    using UI::BuiltInPreset;
    using UI::resolve_builtin_preset;
    using UI::should_apply_builtin_preset;

    assert(resolve_builtin_preset("gaming") == BuiltInPreset::Gaming);
    assert(resolve_builtin_preset("streaming") == BuiltInPreset::Streaming);
    assert(resolve_builtin_preset("recording") == BuiltInPreset::Recording);

    assert(!resolve_builtin_preset("custom").has_value());
    assert(!resolve_builtin_preset("").has_value());
    assert(!resolve_builtin_preset("unknown").has_value());
    assert(!resolve_builtin_preset("Gaming").has_value());

    assert(should_apply_builtin_preset("gaming"));
    assert(should_apply_builtin_preset("streaming"));
    assert(should_apply_builtin_preset("recording"));
    assert(!should_apply_builtin_preset("custom"));
    assert(!should_apply_builtin_preset(""));

    std::cout << "preset selection policy tests passed\n";
    return 0;
}
