#pragma once

#include <SynoraEngine/renderer/types/Pass.h>

namespace SYN {
struct CompositedRenderEffectComponent {
    std::vector<Pass> passes;

    static CompositedRenderEffectComponent empty();
    CompositedRenderEffectComponent &setPass(const Pass &pass);

    // If effect doesn't exist then it creates the effect as empty and hands
    // back a reference to the empty effect.
    Pass &getPass(std::string_view effect);
};

} // namespace SYN
