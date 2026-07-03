#pragma once

#include <cstdint>
#include <glm/vec4.hpp>
#include <optional>
#include <span>
#include <string_view>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct GLFWwindow;

namespace SYN::gfx::gl {

constexpr uint32_t MAX_VERTEX_ATTRIBUTES = 16;

enum class BufferType : uint8_t { Vertex, Index, Uniform };
enum class MemoryUsage : uint8_t { GpuOnly, CpuToGPU };
enum class IndexType : uint8_t { Unsigned16, Unsigned32 };
enum class CullMode : uint8_t { None, Front, Back };
enum class PolygonMode : uint8_t { Fill, Line, Point };
enum class SampleFilter : uint8_t {
    Nearest,
    Linear,
    Nearest_Mipmap_Nearest,
    Linear_Mipmap_Linear,
};
enum class WrapMode : uint8_t { ClampToEdge, Repeat, MirroredRepeat };
enum class TextureFormat : uint8_t {
    RGBA8,
    RGB8,
    RG8,
    R8,
    Depth24,
    Depth24Stencil8
};
enum class PrimitiveTopology : uint8_t { Triangles, Lines, Points };

enum class VertexFormat : uint8_t {
    Float1,
    Float2,
    Float3,
    Float4,
};

enum class UniformType : uint8_t {
    Float1,
    Float2,
    Float3,
    Float4,
    Int1,
    Int2,
    Int3,
    Int4,
    Uint1,
    Uint2,
    Uint3,
    Uint4,
    Mat4x4
};

template <typename T> struct Handle {
    uint32_t index;
    uint32_t generation;
};

template <typename ResourceType> class GLResourceRegistry {
  public:
    using HandleType = Handle<ResourceType>;

  public:
    GLResourceRegistry() { m_Slots.emplace_back(Slot{{}, 0, false}); }
    ~GLResourceRegistry() {
        m_Slots.clear();
        m_FreeSlots.clear();
    }

    std::optional<HandleType> createHandle(ResourceType resource) {
        if (!m_FreeSlots.empty()) {
            uint32_t freeIndex = m_FreeSlots.back();
            m_FreeSlots.pop_back();
            m_Slots[freeIndex].payload = resource;
            m_Slots[freeIndex].isFree = false;
            return HandleType{freeIndex, m_Slots[freeIndex].generation};
        }
        if (m_Slots.size() == std::numeric_limits<uint32_t>().max())
            return std::nullopt;
        m_Slots.push_back({resource, 1, false});
        return HandleType{(uint32_t)(m_Slots.size() - 1),
                          m_Slots.back().generation};
    }

    bool isValidHandle(HandleType handle) const {
        if (handle.generation == 0)
            return false;
        if (handle.index == 0 || handle.index >= m_Slots.size()) {
            return false;
        }

        const Slot &slot = m_Slots.at(handle.index);
        return slot.generation == handle.generation;
    }

    std::vector<ResourceType> getAllResources() {
        std::vector<ResourceType> resources;
        for (Slot slot : m_Slots) {
            if (slot.isFree)
                continue;
            resources.push_back(slot.payload);
        }
        return std::move(resources);
    }

    void releaseHandle(HandleType handle) {
        if (!isValidHandle(handle))
            return;
        Slot &slot = m_Slots.at(handle.index);
        slot.payload = {};
        if (slot.generation == std::numeric_limits<uint32_t>().max())
            return;
        slot.generation += 1;
        slot.isFree = true;
        m_FreeSlots.push_back(handle.index);
    }

    std::optional<ResourceType> getResourceMutable(HandleType handle) {
        if (!isValidHandle(handle))
            return std::nullopt;
        return &m_Slots.at(handle.index).payload;
    }

    std::optional<ResourceType> getResource(HandleType handle) const {
        if (!isValidHandle(handle))
            return std::nullopt;
        return m_Slots.at(handle.index).payload;
    }

  private:
    struct Slot {
        ResourceType payload;
        uint32_t generation = 0;
        bool isFree = true;
    };

    std::vector<Slot> m_Slots;
    std::vector<uint32_t> m_FreeSlots;
};

struct Buffer {
    uint32_t id = 0;
    BufferType type;
};

struct Texture {
    uint32_t id = 0;
};

struct Sampler {
    uint32_t id = 0;
};

struct Renderbuffer {
    uint32_t id = 0;
};

struct Framebuffer {
    uint32_t id = 0;
};

struct VertexArray {
    uint32_t id = 0;
    std::optional<IndexType> indexType = std::nullopt;
};

struct ContextInitDesc {
    GLFWwindow *windowHandle;
    uint32_t width;
    uint32_t height;
};

struct Viewport {
    uint32_t x = 0, y = 0, width, height;
};

struct ScissorRect {
    uint32_t x = 0, y = 0, width, height;
};

struct BufferDesc {
    BufferType bufferType;
    MemoryUsage usage;
    uint32_t size;
};

struct TextureDesc {
    uint32_t width;
    uint32_t height;
    TextureFormat format;
    uint32_t mipLevel = 1;
};

struct SamplerDesc {
    SampleFilter minFilter = SampleFilter::Linear;
    SampleFilter magFilter = SampleFilter::Linear;
    WrapMode wrapU = WrapMode::Repeat;
    WrapMode wrapV = WrapMode::Repeat;
};

struct RenderbufferDesc {
    TextureFormat format;           
    uint32_t width;
    uint32_t height;
    uint32_t sampleCount = 1; 
};

struct AttachmentDesc {
    std::variant<Handle<Texture>, Handle<Renderbuffer>> handle;
};

struct FramebufferDesc {
    std::span<const AttachmentDesc> colorAttachments;
    std::optional<AttachmentDesc> depthStencilAttachment;
    bool isDepthOnly = false;
};

struct PassDesc {
    std::optional<Handle<Framebuffer>> framebufferHandle;
    std::optional<glm::vec4> clearColor;

    bool enableDepthTest;
    bool enableStencilTest;

    std::optional<Viewport> viewportOverride;
    std::optional<ScissorRect> scissorOverride;
};

struct DepthState {
    bool enabled = false;
    bool writeEnabled = true;
};

struct BlendState {
    bool enabled = false;
};

struct Shader {
    uint32_t program = 0;
    uint32_t vertex = 0;
    uint32_t fragment = 0;
};

struct PipelineState {
    Handle<Shader> shader;
    PrimitiveTopology topology = PrimitiveTopology::Triangles;
    CullMode cullMode = CullMode::None;
    PolygonMode polygonMode = PolygonMode::Fill;
    bool frontFaceCcw = true;

    // TODO: Elaborate on depth/blend state
    DepthState depth;
    BlendState blend;
};

struct VertexAttribDesc {
    uint32_t location;
    VertexFormat format;
    uint32_t offset;
    bool normalized = false;
};

struct VertexArrayDesc {
    Handle<Buffer> vertexBufferHandle;
    uint32_t vertexBufferStride;

    Handle<Buffer> indexBufferHandle;
    std::array<VertexAttribDesc, MAX_VERTEX_ATTRIBUTES> attributes;
    uint32_t attributeCount;
    IndexType indexType = IndexType::Unsigned32;
};

class Pass {
  public:
    ~Pass();

    void usePipeline(const PipelineState &pipelineState);

    void bindVertexArray(Handle<VertexArray> vertexArrayHandle);

    void bindUniformBuffer(uint32_t binding, Handle<Buffer> bufferHandle);

    // Float uniforms
    void bindUniform(std::string_view name, float v0);
    void bindUniform(std::string_view name, float v0, float v1);
    void bindUniform(std::string_view name, float v0, float v1, float v2);
    void bindUniform(std::string_view name, float v0, float v1, float v2,
                     float v3);

    // Integer uniforms
    void bindUniform(std::string_view name, int32_t v0);
    void bindUniform(std::string_view name, int32_t v0, int32_t v1);
    void bindUniform(std::string_view name, int32_t v0, int32_t v1, int32_t v2);
    void bindUniform(std::string_view name, int32_t v0, int32_t v1, int32_t v2,
                     int32_t v3);

    // Unsigned integer uniforms
    void bindUniform(std::string_view name, uint32_t v0);
    void bindUniform(std::string_view name, uint32_t v0, uint32_t v1);
    void bindUniform(std::string_view name, uint32_t v0, uint32_t v1,
                     uint32_t v2);
    void bindUniform(std::string_view name, uint32_t v0, uint32_t v1,
                     uint32_t v2, uint32_t v3);

    // Matrix uniforms
    void bindUniform(std::string_view name, const glm::mat4 &v);

    void bindTexture(uint32_t binding, Handle<Texture> textureHandle,
                     Handle<Sampler> samplerHandle);

    void draw(uint32_t vertexCount, uint32_t firstVertex = 0);
    void drawIndexed(uint32_t indexCount);

  private:
    int getShaderUniformLocation(std::string_view uniform);

  private:
    friend class Context;
    Pass(class Context *context, const PassDesc &desc);
    Context *m_ContextPtr;

    PrimitiveTopology m_CurrentDrawTopology = PrimitiveTopology::Triangles;
    IndexType m_CurrentIndexType = IndexType::Unsigned32;
    std::optional<Shader> m_CurrentShader = std::nullopt;

    std::unordered_set<uint32_t> m_TextureSlotsBound;
};

class Context {
  public:
    static std::optional<Context> createContext(const ContextInitDesc &desc);

    ~Context();

    void present();

    Pass beginPass(const PassDesc &desc);

    std::optional<Handle<Buffer>>
    createBuffer(const BufferDesc &desc, const void *initialData = nullptr);

    void updateBuffer(Handle<Buffer> bufferHandle, uint32_t offset,
                      uint32_t size, const void *data);
    void deleteBuffer(Handle<Buffer> bufferHandle);

    std::optional<Handle<Texture>>
    createTexture(const TextureDesc &desc, const void *initialData = nullptr);
    void updateTexture(Handle<Texture> textureHandle, uint32_t mipLevel,
                       uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                       TextureFormat format, const void *data);
    void deleteTexture(Handle<Texture> textureHandle);

    std::optional<Handle<Renderbuffer>> createRenderbuffer(const RenderbufferDesc& desc);
    void deleteRenderbuffer(Handle<Renderbuffer> renderbufferHandle);

    std::optional<Handle<Sampler>> createSampler(const SamplerDesc &desc);
    void deleteSampler(Handle<Sampler> samplerHandle);

    std::optional<Handle<Shader>> createShader(std::string_view vertexSource,
                                               std::string_view fragmentSource);
    std::optional<Handle<Shader>> createShader(uint32_t vertexId,
                                               uint32_t fragmentId);

    void deleteShader(Handle<Shader> shaderHandle);

    std::optional<Handle<Framebuffer>>
    createFramebuffer(const FramebufferDesc &desc);
    void deleteFramebuffer(Handle<Framebuffer> framebufferHandle);

    std::optional<Handle<VertexArray>>
    createVertexArray(const VertexArrayDesc &desc);

    void deleteVertexArray(Handle<VertexArray> vertexArrayHandle);

    void flushDeferredDeletes();

  public:
    Context(Context &&other);
    Context(const Context &other) = delete;
    Context &operator=(Context &&other);

  private:
    Context(const ContextInitDesc &desc);
    bool m_MainContext = true;

  private:
    std::optional<VertexArray>
    getVertexArray(Handle<VertexArray> vertexArrayHandle);
    std::optional<Buffer> getBuffer(Handle<Buffer> bufferHandle);
    std::optional<Shader> getShader(Handle<Shader> shaderHandle);
    std::optional<Texture> getTexture(Handle<Texture> textureHandle);
    std::optional<Sampler> getSampler(Handle<Sampler> samplerHandle);
    std::optional<Framebuffer>
    getFramebuffer(Handle<Framebuffer> framebufferHandle);
    std::optional<Renderbuffer>
    getRenderbuffer(Handle<Renderbuffer> renderbufferHandle);

  private:
    friend class Pass;
    GLResourceRegistry<VertexArray> m_VertexArrayRegistry;
    GLResourceRegistry<Buffer> m_BufferRegistry;
    GLResourceRegistry<Shader> m_ShaderRegistry;
    GLResourceRegistry<Texture> m_TextureRegistry;
    GLResourceRegistry<Sampler> m_SamplerRegistry;
    GLResourceRegistry<Framebuffer> m_FramebufferRegistry;
    GLResourceRegistry<Renderbuffer> m_RenderbufferRegistry;

    std::vector<uint32_t> m_PendingDeleteFramebuffers;
    std::vector<uint32_t> m_PendingDeleteVertexArrays;
    std::vector<uint32_t> m_PendingDeleteBuffers;
    std::vector<uint32_t> m_PendingDeleteShaders;
    std::vector<uint32_t> m_PendingDeletePrograms;
    std::vector<uint32_t> m_PendingDeleteTextures;
    std::vector<uint32_t> m_PendingDeleteSamplers;
    std::vector<uint32_t> m_PendingDeleteRenderbuffers;

  private:
    GLFWwindow *m_Window;
};

} // namespace SYN::gfx::gl
