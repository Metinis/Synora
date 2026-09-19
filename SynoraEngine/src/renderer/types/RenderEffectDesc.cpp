#include <SynoraEngine/renderer/types/RenderEffectDesc.h>

#include <spdlog/spdlog.h>

namespace SYN {
RenderEffectDesc RenderEffectDesc::empty(const std::string &name,
                                         RenderEffectDesc::Type type) {
    if (name.empty()) {
        spdlog::warn(
            "New render effect should not have an empty name. The empty name "
            "is reserved for a rendering backend's default pass.");
    }
    return {name, type};
}

RenderEffectDesc &RenderEffectDesc::setShader(const std::string &shader) {
    this->shader = shader;
    return *this;
}

RenderEffectDesc &RenderEffectDesc::setInstancedDraw(bool draw) {
    this->useInstancedDraw = draw;
    return *this;
}

RenderEffectDesc &RenderEffectDesc::setFilterMask(uint64_t filter) {
    this->filterMask = filter;
    return *this;
}

RenderEffectDesc &
RenderEffectDesc::setPipelineState(PipelineState pipelineState) {
    this->pipeline = pipelineState;
    return *this;
}

RenderEffectDesc &
RenderEffectDesc::addInput(std::string_view inputName,
                           const InputSlot::Value &value,
                           RenderEffectDesc::InputLevel level) {
    this->inputBindings.emplace(inputName, std::make_tuple(value, level));
    return *this;
}
} // namespace SYN
