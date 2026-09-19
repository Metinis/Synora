#pragma once

#include <SynoraEngine/renderer/types/Pass.h>

namespace SYN {
struct CompositedRenderEffectComponent {
    std::vector<Pass> passes;

    static CompositedRenderEffectComponent empty();
    CompositedRenderEffectComponent &addPass(const Pass &pass);
};

} // namespace SYN
