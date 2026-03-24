#pragma once
#include <glm/glm.hpp>

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

struct TextureDesc {
    uint32_t width;
    uint32_t height;

    TextureType type;
};

enum class AttachmentSize : uint32_t { fixed, relative };

struct AttachmentDesc {
    uint32_t width{};  // ignored if size is relative
    uint32_t height{}; // ignored if size is relative
    AttachmentSize size{AttachmentSize::relative};

    TextureType type;
    bool isSampleable;
};

struct BufferDesc {
    size_t size;
};

// we use the descriptions to keep the handle types different
using TextureHandle = GPUResourceHandle<TextureDesc>;
using AttachmentHandle = GPUResourceHandle<AttachmentDesc>;
using BufferHandle = GPUResourceHandle<BufferDesc>;

struct Viewport {
    uint32_t width;
    uint32_t height;
};

enum class StoreOp { store, dontCare };
enum class LoadOp { load, clear, dontCare };

struct WriteAttachment {
    AttachmentHandle handle;
    StoreOp storeOp{StoreOp::store};
    LoadOp loadOp{LoadOp::clear};
    union {
        glm::vec4 clearColor;
        float clearDepth;
    };
};

struct RenderPassDesc {
    std::span<AttachmentHandle> readAttachments;
    std::span<WriteAttachment> colorAttachments;
    std::optional<WriteAttachment> depthAttachment;
    Viewport viewport;
};

} // namespace SYN

template <typename T> struct std::hash<SYN::GPUResourceHandle<T>> {
    size_t operator()(const SYN::GPUResourceHandle<T> &handle) const {
        return std::hash<uint32_t>{}(static_cast<uint32_t>(handle.id));
    }
};
