#include <SynoraEngine/renderer/types/Pass.h>

namespace SYN {
Pass Pass::empty() { return {}; }
Pass &Pass::setEffect(std::string_view effect) {
    this->effect = effect;
    return *this;
}
Pass &Pass::setOutput(Target output) {
    this->output = output;
    return *this;
}

Pass &Pass::setBlitTarget(Target target) {
    this->blitTarget = target;
    return *this;
}

Pass &Pass::setInput(std::string_view name, const InputSlot::Value &value) {
    auto it = std::find_if(
        inputs.begin(), inputs.end(),
        [&name](const InputSlot &slot) { return slot.inputName == name; });

    if (it != inputs.cend()) {
        it->inputName = name;
        it->value = value;
        return *this;
    }

    this->inputs.emplace_back(std::string(name), value);
    return *this;
}

Pass &Pass::clearColor(glm::vec4 color) {
    clearOptions.clearColor = color;
    return *this;
}
Pass &Pass::clearDepth(float depth) {
    clearOptions.clearDepth = depth;
    return *this;
}
Pass &Pass::clearStencil(uint8_t stencil) {
    clearOptions.clearStencil = stencil;
    return *this;
}
} // namespace SYN
