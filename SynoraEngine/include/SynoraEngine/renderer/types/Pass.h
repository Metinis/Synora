#pragma once

#include "ClearOptions.h"
#include "InputSlot.h"

namespace SYN {
struct Pass {
    std::string effect = "";
    std::variant<std::string, AssetRef> output{};
    std::vector<InputSlot> inputs;
    ClearOptions clearOptions{};

    static Pass empty();
    Pass &setEffect(std::string_view effect);
    Pass &setOutput(AssetRef output);
    Pass &setInput(std::string_view name, const InputSlot::Value &value);
    Pass &clearColor(glm::vec4 color);
    Pass &clearDepth(float depth);
    Pass &clearStencil(uint8_t stencil);
};
} // namespace SYN
