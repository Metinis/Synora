#include <SynoraEngine/scene/components/CustomRenderDataComponent.h>

namespace SYN {
CustomRenderDataComponent CustomRenderDataComponent::empty() { return {}; }

CustomRenderDataComponent &
CustomRenderDataComponent::setEffectInput(std::string_view effect,
                                          std::string_view inputName,
                                          const InputSlot::Value &value) {
    std::string effectName = std::string(effect);
    std::string inputNameStr = std::string(inputName);
    if (auto it = perInstanceEffectData.find(effectName);
        it != perInstanceEffectData.cend()) {
        auto inputSlot = std::find_if(it->second.begin(), it->second.end(),
                                      [&inputNameStr](const InputSlot &slot) {
                                          return slot.inputName == inputNameStr;
                                      });

        if (inputSlot != it->second.cend()) {
            *inputSlot = InputSlot{inputNameStr, value};
            return *this;
        }

        it->second.emplace_back(inputNameStr, value);
        return *this;
    }
    perInstanceEffectData.emplace(
        effect, std::vector<InputSlot>{InputSlot{inputNameStr, value}});
    return *this;
}
} // namespace SYN
