#pragma once
#include <glm/glm.hpp>
#include <string_view>

namespace {
template <typename T> void hashCombine(uint64_t &seed, const T &val) {
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

    bool isValid() const { return id != UINT32_MAX; }
};

enum class TextureFormat {
    invalid,
    rgba8,
    bgra8,
    rgba16f,
    rgba32f,
    r32f,
    r32ui
};
enum class TextureType { invalid, srgb, depth, rgba };
enum class PipelineStage { invalid, compute, fragment };

struct TextureDesc {
    uint32_t width;
    uint32_t height;

    TextureType type{TextureType::invalid};
    TextureFormat format{TextureFormat::invalid};
    bool hasMipChain{true};
    bool isCubeMap{false}; // layer count must be 6 if this is true
};

enum class AttachmentSize : uint32_t { fixed, relative };

struct AttachmentDesc {
    uint32_t width{};  // ignored if size is relative
    uint32_t height{}; // ignored if size is relative
    AttachmentSize size{AttachmentSize::relative};

    TextureType type{TextureType::invalid};
    TextureFormat format{TextureFormat::invalid};
    uint32_t msaaSamples{1}; // 1 is no multisampling
    bool isCubeMap{false};   // layer count must be 6 if this is true
    bool isStorageImage{false};
};

struct BufferDesc {
    size_t size;
};

enum class CullMode { disabled, frontFace, backFace };
enum class PolygonMode { fill, line };

struct GraphicsPipelineDesc {
    CullMode cullMode{CullMode::backFace};
    PolygonMode polygonMode{PolygonMode::fill};
    bool hasAlphaBlending{false};

    uint32_t nColorAttachments{};
    // if either of these are true, there must be a depth attachment / these
    // being true imply a depth attachment
    bool hasDepthTesting{};
    bool hasDepthWriting{};

    uint32_t msaaSamples{
        1}; // all attachments must have the same number of samples

    std::optional<std::string_view> vertexShaderPath{};
    std::optional<std::string_view> fragmentShaderPath{};

    bool operator==(const GraphicsPipelineDesc &other) const = default;
};

struct ComputePipelineDesc {
    std::string_view shaderPath{};

    bool operator==(const ComputePipelineDesc &other) const = default;
};

struct Receipt;

// we use the descriptions to keep the handle types different
using TextureHandle = GPUResourceHandle<TextureDesc>;
using AttachmentHandle = GPUResourceHandle<AttachmentDesc>;
using BufferHandle = GPUResourceHandle<BufferDesc>;
using PipelineHandle =
    GPUResourceHandle<GraphicsPipelineDesc>; // used for graphics and compute

struct Viewport {
    uint32_t width;
    uint32_t height;
};

enum class StoreOp { store, dontCare };
enum class LoadOp { load, clear, dontCare };

struct WriteAttachmentInfo {
    AttachmentHandle handle;
    std::optional<AttachmentHandle> resolveHandle{};

    uint32_t layer{0};
    uint32_t resolveLayer{0};

    StoreOp storeOp{StoreOp::store};
    LoadOp loadOp{LoadOp::clear};
    union {
        glm::vec4 clearColor;
        float clearDepth;
    };
};

struct RenderPassDesc {
    std::string debugName{"Default Pass"};

    std::span<AttachmentHandle> readAttachments;
    std::span<WriteAttachmentInfo> colorAttachments;
    std::optional<WriteAttachmentInfo> depthAttachment;
};

struct DispatchDesc {
    std::string debugName{"Default Pass"};

    std::span<AttachmentHandle> readonlyAttachments; // accessed in textures
    std::span<AttachmentHandle>
        readWriteAttachments; // accessed in storage images

    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
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
