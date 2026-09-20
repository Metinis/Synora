#pragma once

#include <SynoraEngine/renderer/types/InputSlot.h>

namespace SYN {
struct CustomRenderDataComponent {
    using EffectBindingsTable =
        std::unordered_map<std::string, std::vector<InputSlot>>;
    EffectBindingsTable perInstanceEffectData;

    static CustomRenderDataComponent empty();
    CustomRenderDataComponent &setEffectInput(std::string_view effect,
                                              std::string_view inputName,
                                              const InputSlot::Value &value);
};
} // namespace SYN
