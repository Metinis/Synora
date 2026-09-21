#pragma once

#include <SynoraEngine/renderer/types/Pass.h>

namespace SYN {
struct CompositedRenderEffectComponent {
    std::vector<Pass> passes;

    // Use if you just want to run the default pass in the current
    // render backend and render the output to a render target
    static CompositedRenderEffectComponent defaultPass(AssetRef renderTarget);

    static CompositedRenderEffectComponent empty();
    CompositedRenderEffectComponent &setPass(const Pass &pass);

    // If effect doesn't exist then it creates the effect as empty and hands
    // back a reference to the empty effect.
    Pass &getPass(std::string_view effect);
};

} // namespace SYN
