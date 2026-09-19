#include <SynoraEngine/scene/components/CompositedRenderEffectComponent.h>

namespace SYN {
CompositedRenderEffectComponent CompositedRenderEffectComponent::empty() {
    return {};
}

CompositedRenderEffectComponent &
CompositedRenderEffectComponent::addPass(const Pass &pass) {
    this->passes.push_back(pass);
    return *this;
}
} // namespace SYN
