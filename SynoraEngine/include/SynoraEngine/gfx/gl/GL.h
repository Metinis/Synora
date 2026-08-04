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
constexpr float MIN_GAMMA = 1.8f;
constexpr float MAX_GAMMA = 2.6f;

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
enum class WrapMode : uint8_t {
    ClampToEdge,
    Repeat,
    MirroredRepeat,
    ClampToBorder
};
enum class TextureFormat : uint8_t {
    RGBA8,
    RGB8,
    RG8,
    R8,
    Depth16,
    Depth24,
    Depth24Stencil8,
    SRGB,
    SRGBA,
    RG16F,
    RG32F,
    RGB16F,
    RGBA16F,
    RGB32F,
    RGBA32F
};
enum class PrimitiveTopology : uint8_t { Triangles, Lines, Points };

enum class TextureType : uint8_t { Tex2D, Cubemap, Tex2DArray };

enum class VertexFormat : uint8_t {
    Float1,
    Float2,
    Float3,
    Float4,
    Int1,
    Int2,
    Int3,
    Int4
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

// Used by renderer's uber shader
enum class ShaderFeatures : uint32_t {
    Normal = 1,
    MetallicRoughness = 1 << 1,
    Skinned = 1 << 2
};

enum class AntiAliasMode { None, FXAA, MSAA_2x, MSAA_4x, MSAA_8x };

enum class DepthFunc : uint8_t {
    Always,
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual
};

template <typename T> struct Handle {
    uint32_t index;
    uint32_t generation;
};

template <typename ResourceType> class ResourceRegistry {
  public:
    using HandleType = Handle<ResourceType>;

  public:
    ResourceRegistry() { m_Slots.emplace_back(Slot{{}, 0, false}); }
    ~ResourceRegistry() {
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
    TextureType type;
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
    uint32_t sampleCount = 1;
    TextureType type = TextureType::Tex2D;
    uint32_t arraySize = 0;
};

struct SamplerDesc {
    SampleFilter minFilter = SampleFilter::Linear;
    SampleFilter magFilter = SampleFilter::Linear;
    WrapMode wrapU = WrapMode::Repeat;
    WrapMode wrapV = WrapMode::Repeat;
    WrapMode wrapW = WrapMode::Repeat;
    glm::vec4 borderColor = glm::vec4(1.0f);
    bool compareMode = false;
    float anisotropicLevel = 1.0f;
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
    bool writeEnabled = true;
    DepthFunc test = DepthFunc::Less;
};

struct ColorMask {
    bool r = true;
    bool g = true;
    bool b = true;
    bool a = true;
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

    // Determines which channels are written.
    // NOT for clearing color.
    ColorMask color;

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

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 tangent;
    glm::ivec4 boneIndices = glm::ivec4(0);
    glm::vec4 boneWeights = glm::vec4(0);
};

// For renderer
struct TextureData {
    TextureDesc info;
    std::span<uint8_t> data;
    std::string sourcePath;
};

struct MaterialData {
    std::optional<TextureData> albedoData;
    std::optional<TextureData> normalData;
    std::optional<TextureData> metallicRoughnessData;

    float metallic = 1.0f;
    float roughness = 1.0f;
    glm::vec4 tint = glm::vec4(1.0f);

    bool alphaMasked = false;
};

struct Material {
    std::optional<Handle<Texture>> albedo;
    std::optional<Handle<Texture>> normalMap;
    std::optional<Handle<Texture>> metallicRoughnessMap;
    std::optional<Handle<Sampler>> sampler;
    std::optional<SamplerDesc> samplerDesc;

    float metallic = 0.0f;
    float roughness = 0.0f;
    glm::vec4 tint = glm::vec4(1.0f);
    bool alphaMasked = false;
};

struct MaterialOverride {
    uint32_t meshIndex;
    Material material;
};

struct Camera {
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up = {0, 1, 0};
    float fovYDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 500.0f;
};

struct DirectionalLight {
    glm::vec3 direction =
        glm::vec3(1.0, 0.0, 0.0); // normalized, pointing FROM light TO scene
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    bool castsShadows = true;
};

struct RendererConfig {
    AntiAliasMode aaMode = AntiAliasMode::MSAA_4x;
    uint32_t shadowMapFarResolution = 1024;
    uint32_t shadowMapNearResolution = 2048;
    float shadowDistance = 20.0f; // frustum-fit range from camera
    float exposure = 1.0f;
    bool bloomEnabled = true;
    float bloomThreshold = 1.0f;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    MaterialData material;
    glm::mat4 localTransform = glm::mat4(1.0);

    bool hasSkin;
};

struct ModelData {
    std::vector<MeshData> meshes;
};

struct Mesh {
    Handle<VertexArray> vao;
    Handle<Buffer> vbo;
    Handle<Buffer> ebo;

    Handle<Shader> shader;
    uint32_t shaderFeatures;

    Material material;

    uint32_t indexCount;

    glm::mat4 localTransform;
};

struct Model {
    std::vector<Mesh> meshesOpaque;
    std::vector<Mesh> meshesMasked;
};

struct Environment {
    Handle<Texture> cubemap;
    std::optional<Handle<Texture>> irradianceMap = std::nullopt;
    std::optional<Handle<Texture>> prefilteredMap = std::nullopt;
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
    void drawInstancedIndexed(uint32_t indexCount, uint32_t instanceCount);

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
                       uint32_t face = 0,
                       TextureFormat format = TextureFormat::RGBA8,
                       const void *data = nullptr);
    void generateMipmap(Handle<Texture> textureHandle);
    void deleteTexture(Handle<Texture> textureHandle);

    // Main use case is for ImGui debug
    std::optional<uint32_t> getTextureId(Handle<Texture> textureHandle);

    std::optional<Handle<Renderbuffer>>
    createRenderbuffer(const RenderbufferDesc &desc);
    void deleteRenderbuffer(Handle<Renderbuffer> renderbufferHandle);

    std::optional<Handle<Sampler>> createSampler(const SamplerDesc &desc);
    void updateSampler(Handle<Sampler> samplerHandle, const SamplerDesc &desc);
    void deleteSampler(Handle<Sampler> samplerHandle);

    std::optional<Handle<Shader>> createShader(std::string_view vertexSource,
                                               std::string_view fragmentSource);
    std::optional<Handle<Shader>> createShader(uint32_t vertexId,
                                               uint32_t fragmentId);

    void deleteShader(Handle<Shader> shaderHandle);

    std::optional<Handle<Framebuffer>>
    createFramebuffer(const FramebufferDesc &desc);
    void deleteFramebuffer(Handle<Framebuffer> framebufferHandle);

    void setColorAttachment(Handle<Framebuffer> handle, uint32_t index,
                            Handle<Texture> texture, uint32_t mip = 0,
                            std::optional<uint32_t> layer = std::nullopt);

    void setDepthAttachment(Handle<Framebuffer> handle, Handle<Texture> texture,
                            uint32_t mip = 0,
                            std::optional<uint32_t> layer = std::nullopt);

    // TODO: Allow configuration of defaults: mask(GL_COLOR_BUFFER_BIT),
    // filter(GL_NEAREST). Currently only for MSAA.
    void blitFramebuffer(std::optional<Handle<Framebuffer>> readHandle,
                         std::optional<Handle<Framebuffer>> writeHandle,
                         Viewport sourceRect, Viewport destRect);

    std::optional<Handle<VertexArray>>
    createVertexArray(const VertexArrayDesc &desc);

    void deleteVertexArray(Handle<VertexArray> vertexArrayHandle);

    void flushDeferredDeletes();

    void enableVSync(bool vsync);

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
    ResourceRegistry<VertexArray> m_VertexArrayRegistry;
    ResourceRegistry<Buffer> m_BufferRegistry;
    ResourceRegistry<Shader> m_ShaderRegistry;
    ResourceRegistry<Texture> m_TextureRegistry;
    ResourceRegistry<Sampler> m_SamplerRegistry;
    ResourceRegistry<Framebuffer> m_FramebufferRegistry;
    ResourceRegistry<Renderbuffer> m_RenderbufferRegistry;

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

class ShaderCache {
  public:
    ShaderCache();
    Handle<Shader> getShaderHandle(Context &context, uint32_t featureFlags);

  private:
    std::unordered_map<uint32_t, Handle<Shader>> m_ShaderCache;
    std::string m_VertexSource;
    std::string m_FragmentSource;
};

class Renderer {
  public:
    explicit Renderer(const RendererConfig &config);
    ~Renderer();

    void init(Context &context);

    std::vector<uint32_t> getCSMTextures(Context &context);

    std::optional<Handle<Model>> createModel(Context &context,
                                             const ModelData &data);
    // Cubemap faces must be RGBA8!
    Handle<Texture> loadCubemap(Context &context,
                                const std::array<uint8_t *, 6> &faces,
                                uint32_t width, uint32_t height);

    // Must be HDR
    Handle<Texture> loadCubemapFromEquirectangularTexture(Context &context,
                                                          float *data,
                                                          uint32_t width,
                                                          uint32_t height);

    // hdrMap must be a cubemap
    Handle<Texture> createIrradianceMap(Context &context,
                                        Handle<Texture> hdrMap);

    Handle<Texture> createPrefilteredEnvironmentMap(Context &context,
                                                    Handle<Texture> hdrMap);

    // TODO: Just wrte to disk
    Handle<Texture> createBRDFLut(Context &context);

    // If no environment is set just use the renderer clear color
    void setEnvironment(std::optional<Environment> environment);

    void destroyModel(Context &context, Handle<Model> meshHandle);

    void setClearColor(const glm::vec4 &clearColor);
    void beginFrame(const Camera &camera);
    void setDirectionalLight(const DirectionalLight &light);
    void submit(Handle<Model> modelHandle, const glm::mat4 &transform,
                std::span<const MaterialOverride> materialOverride = {});
    void endFrame(Context &context); // runs shadow pass -> opaque pass -> post
                                     // -> writes to default FBO

    // Clamped between MIN_GAMMA and MAX_GAMMA
    void setGamma(float gamma);

    void setCSMDistance(float distance);

    void setExposure(float exposure);
    void setBloomEnabled(bool enabled);
    void setAntiAliasMode(AntiAliasMode mode);
    void setAnisotropicFiltering(float filter);

    void resize(int width, int height);

  private:
    struct {
        std::optional<Handle<Framebuffer>> handle;
        std::optional<Handle<Renderbuffer>> rbAttachment;
        std::optional<Handle<Texture>> colorAttachment;
        bool update = true;
    } m_MsaaFramebuffer;

    void updateMsaaFramebuffer(Context &context);

    struct {
        std::optional<Handle<Framebuffer>> handle;
        std::optional<Handle<Texture>> colorAttachment;
        Handle<Sampler> colorSampler;
        bool update = true;
    } m_HdrFramebuffer;

    void createHdrShader(Context &context);
    std::optional<Handle<Shader>> m_HdrShader;

    void updateHdrFramebuffer(Context &context);

    void createScreenQuad(Context &context);
    void createSkybox(Context &context);
    void createIrradianceShader(Context &context);
    void createPrefilterShader(Context &context);
    void createBRDFLutShader(Context &context);
    void createDefaultPrefilterMap(Context &context);
    void createZPrepassShader(Context &context);
    void createCSM(Context &context);

    std::optional<Handle<VertexArray>> m_ScreenQuad;
    std::optional<Handle<VertexArray>> m_SkyboxCube;

    float m_Exposure = 1.0f;
    float m_Gamma = 2.2f;

  private:
    Camera m_MainCamera;
    glm::vec4 m_ClearColor;
    Viewport m_ScreenViewport;
    DirectionalLight m_DirectionalLight;

    std::optional<Environment> m_Environment;
    std::optional<Handle<Shader>> m_SkyboxShader;
    std::optional<Handle<Shader>> m_EquirectangularToCubemapShader;
    std::optional<Handle<Shader>> m_IrradianceShader;
    std::optional<Handle<Shader>> m_PrefilterShader;
    std::optional<Handle<Shader>> m_BRDFLutShader;

    std::optional<Handle<Shader>> m_ZPrepassShaderOpaque;
    std::optional<Handle<Shader>> m_ZPrepassShaderMasked;

    RendererConfig m_RenderConfig;

  private:
    struct DrawCommand {
        Handle<Model> modelHandle;
        glm::mat4 transform;
        std::vector<MaterialOverride> materialOverride;
    };

    ShaderCache m_ShaderCache;

    std::unordered_map<std::string, Handle<Texture>> m_TextureCache;
    Handle<Texture> m_DefaultWhite;
    Handle<Texture> m_DefaultPrefilterMap;
    Handle<Texture> m_DefaultIrradianceMap;
    Handle<Texture> m_BRDFLut;

    Handle<Sampler> m_DefaultModelSampler;
    std::optional<Handle<Sampler>> m_DefaultSampler;
    std::optional<Handle<Sampler>> m_CubemapSampler;
    std::optional<Handle<Sampler>> m_MipmapCubeSampler;
    SamplerDesc m_DefaultModelSamplerDesc;
    float m_AnisotropicFilter;
    bool m_AnisotropicUpdate = false;

  private:
    void drawDirectionalCSM(Context &context, const DirectionalLight &light);

    struct {
        Handle<Framebuffer> fboNear;
        Handle<Framebuffer> fboFar;
        Handle<Texture> depthTextureNear;
        Handle<Texture> depthTextureFar;
        Handle<Sampler> shadowSampler;
        std::vector<float> planeDistances;
        std::vector<glm::mat4> lightSpaceMatrices;
        std::vector<float> cascadeTexelWorldSize;
        bool isInstanced;
    } m_CascadedShadowmap;

    std::optional<Handle<Shader>> m_ShadowMapShaderOpaque;
    std::optional<Handle<Shader>> m_ShadowMapShaderMasked;
    void createShadowmapShader(Context &context);

    void drawInstancedCSMDepth(Context &context, Handle<Framebuffer> fbo,
                               Handle<Texture> depth, bool isNear,
                               uint32_t resolution);
    void drawCSMDepth(Context &context, Handle<Framebuffer> fbo,
                      Handle<Texture> depth, bool isNear, uint32_t resolution);

    std::vector<glm::vec4> getFrustumCornersWorldSpace(const Camera &camera);
    glm::mat4 calculateTightLightFrustum(const DirectionalLight &light,
                                         uint32_t resolution,
                                         const Camera &camera,
                                         float &texelWorld);

  private:
    std::vector<DrawCommand> m_DrawCommandList;
    ResourceRegistry<Model> m_ModelRegistry;

    struct RenderItem {
        glm::mat4 transform;
        Material material;
        Handle<VertexArray> vao;
        uint32_t indexCount;
    };

    void drawRenderItems(Pass &pass,
                         const std::vector<RenderItem> &renderItems);

    // Possible permutations of shaders so far
    // Update as needed or turn into hashmap
    std::array<std::vector<RenderItem>, 8> m_RenderBucketsOpaque;
    std::array<std::vector<RenderItem>, 8> m_RenderBucketsMasked;

    struct alignas(16) CameraConstants {
        glm::mat4 u_viewProjection;
        glm::mat4 u_view;
        glm::vec3 u_cameraPos;
    };

    struct alignas(16) ShadowConstants {
        std::array<glm::mat4, 4> u_lightSpaceMatrices;
        glm::vec4 u_cascadePlaneDistances;
        glm::vec4 u_cascadeTexelWorldSize;
    };

    struct alignas(16) LightConstants {
        glm::vec3 direction;
        alignas(16) glm::vec3 color;
        float intensity;
        int32_t castShadow;
    };

    Handle<Buffer> m_CameraConstants;
    Handle<Buffer> m_ShadowConstants;
    Handle<Buffer> m_LightConstants;
};

} // namespace SYN::gfx::gl
