#include <SynoraEngine/renderer/types/Pass.h>

namespace SYN {
Pass Pass::empty() { return {}; }
Pass &Pass::setEffect(std::string_view effect) {
    this->effect = effect;
    return *this;
}
Pass &Pass::setOutput(AssetRef output) {
    this->output = output;
    return *this;
}
Pass &Pass::addInput(std::string_view name, const InputSlot::Value &value) {
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
