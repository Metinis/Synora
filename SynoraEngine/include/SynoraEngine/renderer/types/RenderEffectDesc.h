#pragma once

#include "InputSlot.h"
#include "PipelineState.h"

namespace SYN {
struct RenderEffectDesc {
  public:
    enum class Type { Geometry, Screen };
    enum class InputLevel { PerPass, PerInstance };

    using InputBindingTable =
        std::unordered_map<std::string,
                           std::tuple<InputSlot::Value, InputLevel>>;

    std::string name{};
    Type type = Type::Screen;
    PipelineState pipeline{};

    std::string shader;

    uint64_t filterMask = std::numeric_limits<uint64_t>().max();

    bool useFrustumCulling = true;
    bool useInstancedDraw = false;

    InputBindingTable inputBindings{};

    static RenderEffectDesc empty(const std::string &name, Type type);
    RenderEffectDesc &setShader(const std::string &shader);
    RenderEffectDesc &setInstancedDraw(bool draw);
    RenderEffectDesc &setFilterMask(uint64_t filter);
    RenderEffectDesc &setPipelineState(PipelineState pipelineState);
    RenderEffectDesc &addInput(std::string_view inputName,
                               const InputSlot::Value &value, InputLevel level);
};
} // namespace SYN
