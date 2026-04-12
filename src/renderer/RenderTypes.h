#pragma once
#include <glm/glm.hpp>
#include <string_view>

namespace {
template <typename T> void hashCombine(size_t &seed, const T &val) {
    seed ^= std::hash<T>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
} // namespace

namespace SYN {

// this is a struct 1. for type safety and 2. so we can add stuff like
// generation later
template <typename T> struct GPUResourceHandle {
    uint32_t id{UINT32_MAX};
    bool operator==(const GPUResourceHandle<T> &other) const {
        return id == other.id;
    }
};

enum class TextureType { invalid, srgb, depth, rgba };
enum class PipelineStage { invalid, compute, fragment };

struct TextureDesc {
    uint32_t width;
    uint32_t height;

    TextureType type;
    bool hasMipChain{true};
};

enum class AttachmentSize : uint32_t { fixed, relative };

struct AttachmentDesc {
    uint32_t width{};  // ignored if size is relative
    uint32_t height{}; // ignored if size is relative
    AttachmentSize size{AttachmentSize::relative};

    TextureType type;
    bool isSampleable;
    uint32_t msaaSamples{1}; // 1 is no multisampling
};

struct BufferDesc {
    size_t size;
};

enum class CullMode { disabled, frontFace, backFace };
enum class PolygonMode { fill, line };

struct GraphicsPipelineDesc {
    CullMode cullMode{CullMode::backFace};
    PolygonMode polygonMode{PolygonMode::fill};

    uint32_t nColorAttachments{};
    bool hasDepthAttachment{}; // if this is true, depth testing is enabled

    std::optional<std::string_view> vertexShaderPath{};
    std::optional<std::string_view> fragmentShaderPath{};

    bool operator==(const GraphicsPipelineDesc &other) const = default;
};

// we use the descriptions to keep the handle types different
using TextureHandle = GPUResourceHandle<TextureDesc>;
using AttachmentHandle = GPUResourceHandle<AttachmentDesc>;
using BufferHandle = GPUResourceHandle<BufferDesc>;
using PipelineHandle = GPUResourceHandle<GraphicsPipelineDesc>;

struct Viewport {
    uint32_t width;
    uint32_t height;
};

enum class StoreOp { store, dontCare };
enum class LoadOp { load, clear, dontCare };

struct WriteAttachment {
    AttachmentHandle handle;
    std::optional<AttachmentHandle> resolveHandle{};
    StoreOp storeOp{StoreOp::store};
    LoadOp loadOp{LoadOp::clear};
    union {
        glm::vec4 clearColor;
        float clearDepth;
    };
};

struct RenderPassDesc {
    std::span<const AttachmentHandle> readAttachments;
    std::span<const WriteAttachment> colorAttachments;
    std::optional<const WriteAttachment> depthAttachment;
};

} // namespace SYN

template <typename T> struct std::hash<SYN::GPUResourceHandle<T>> {
    size_t operator()(const SYN::GPUResourceHandle<T> &handle) const {
        return std::hash<uint32_t>{}(static_cast<uint32_t>(handle.id));
    }
};
template <> struct std::hash<SYN::GraphicsPipelineDesc> {
    size_t operator()(const SYN::GraphicsPipelineDesc &desc) const {
        uint64_t seed{};
        hashCombine(seed, desc.fragmentShaderPath);
        hashCombine(seed, desc.vertexShaderPath);

        return seed;
    }
};
