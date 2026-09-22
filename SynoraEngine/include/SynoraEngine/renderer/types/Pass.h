#pragma once

#include "ClearOptions.h"
#include "InputSlot.h"

namespace SYN {
struct Pass {
    using Target = std::variant<std::string, AssetRef>;
    std::string effect = "";
    Target output{};
    std::vector<InputSlot> inputs;
    ClearOptions clearOptions{};
    Target blitTarget{};

    static Pass empty();
    Pass &setEffect(std::string_view effect);
    Pass &setOutput(Target output);
    Pass &setBlitTarget(Target target);
    Pass &setInput(std::string_view name, const InputSlot::Value &value);
    Pass &clearColor(glm::vec4 color);
    Pass &clearDepth(float depth);
    Pass &clearStencil(uint8_t stencil);
};
} // namespace SYN
