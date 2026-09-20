#include <SynoraEngine/scene/components/CompositedRenderEffectComponent.h>

#include <assert.h>

namespace SYN {
CompositedRenderEffectComponent CompositedRenderEffectComponent::empty() {
    return {};
}

CompositedRenderEffectComponent &
CompositedRenderEffectComponent::setPass(const Pass &pass) {
    assert(!pass.effect.empty() &&
           "Cannot have default effect in a composited render effect");

    auto it = std::find_if(passes.begin(), passes.end(),
                           [&pass](const Pass &otherPass) {
                               return pass.effect == otherPass.effect;
                           });

    if (it != passes.cend()) {
        *it = pass;
    } else {
        passes.emplace_back(pass);
    }

    return *this;
}

Pass &CompositedRenderEffectComponent::getPass(std::string_view effect) {
    assert(!effect.empty() &&
           "Cannot have default effect in a composited render effect");

    auto it = std::find_if(passes.begin(), passes.end(),
                           [&effect](const Pass &otherPass) {
                               return effect == otherPass.effect;
                           });

    if (it != passes.cend()) {
        return *it;
    }

    return passes.emplace_back(Pass::empty().setEffect(effect));
}

} // namespace SYN
