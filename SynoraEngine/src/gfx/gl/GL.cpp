#include <SynoraEngine/gfx/gl/GL.h>

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include <fstream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

struct {
    using UniformLocations = std::unordered_map<std::string_view, int>;
    std::unordered_map<uint32_t, UniformLocations> shaderUniformCache;
} Globals;

// Adds shader uniform to cache if it doesn't exist. Retrieves uniform
// location which may be -1 if it doesn't exist in the shader.
int SYN::gfx::gl::Pass::getShaderUniformLocation(std::string_view uniform) {
    assert(m_CurrentShader.has_value() &&
           "Shader not set. Are you binding a pipeline, and does it have its "
           "shader set?");

    Shader shader = m_CurrentShader.value();

    if (Globals.shaderUniformCache[shader.program].find(uniform) ==
        Globals.shaderUniformCache[shader.program].cend()) {
        int uniformLoc = glGetUniformLocation(shader.program, uniform.data());
        if (uniformLoc == -1)
            return -1;
        Globals.shaderUniformCache[shader.program][uniform] = uniformLoc;
    }

    return Globals.shaderUniformCache[shader.program].at(uniform);
}

GLenum getInternalTextureFormat(SYN::gfx::gl::TextureFormat format) {
    switch (format) {
    case SYN::gfx::gl::TextureFormat::RGBA8:
        return GL_RGBA8;
    case SYN::gfx::gl::TextureFormat::RGB8:
        return GL_RGB8;
    case SYN::gfx::gl::TextureFormat::RG8:
    case SYN::gfx::gl::TextureFormat::R8:
        return GL_R8;
    case SYN::gfx::gl::TextureFormat::Depth24:
        return GL_DEPTH_COMPONENT24;
    case SYN::gfx::gl::TextureFormat::Depth24Stencil8:
        return GL_DEPTH24_STENCIL8;
    case SYN::gfx::gl::TextureFormat::SRGB:
        return GL_SRGB8;
    case SYN::gfx::gl::TextureFormat::SRGBA:
        return GL_SRGB8_ALPHA8;
    case SYN::gfx::gl::TextureFormat::RG16F:
        return GL_RG16F;
    case SYN::gfx::gl::TextureFormat::RG32F:
        return GL_RG32F;
    case SYN::gfx::gl::TextureFormat::RGB16F:
        return GL_RGB16F;
    case SYN::gfx::gl::TextureFormat::RGBA16F:
        return GL_RGBA16F;
    case SYN::gfx::gl::TextureFormat::RGB32F:
        return GL_RGB32F;
    case SYN::gfx::gl::TextureFormat::RGBA32F:
        return GL_RGBA32F;
    }
}

GLenum getTextureFormat(SYN::gfx::gl::TextureFormat format) {
    switch (format) {
    case SYN::gfx::gl::TextureFormat::RGBA8:
        return GL_RGBA;
    case SYN::gfx::gl::TextureFormat::RGB8:
        return GL_RGB;
    case SYN::gfx::gl::TextureFormat::RG8:
        return GL_RG;
    case SYN::gfx::gl::TextureFormat::R8:
        return GL_RED;
    case SYN::gfx::gl::TextureFormat::Depth24:
        return GL_DEPTH_COMPONENT;
    case SYN::gfx::gl::TextureFormat::Depth24Stencil8:
        return GL_DEPTH_STENCIL;
    case SYN::gfx::gl::TextureFormat::SRGB:
        return GL_RGB;
    case SYN::gfx::gl::TextureFormat::SRGBA:
        return GL_RGBA;
    case SYN::gfx::gl::TextureFormat::RG16F:
        return GL_RG;
    case SYN::gfx::gl::TextureFormat::RG32F:
        return GL_RG;
    case SYN::gfx::gl::TextureFormat::RGB16F:
        return GL_RGB;
    case SYN::gfx::gl::TextureFormat::RGBA16F:
        return GL_RGBA;
    case SYN::gfx::gl::TextureFormat::RGB32F:
        return GL_RGB;
    case SYN::gfx::gl::TextureFormat::RGBA32F:
        return GL_RGBA;
    }
}

GLenum getTextureDataTypeFromFormat(SYN::gfx::gl::TextureFormat format) {
    GLenum dataType = GL_UNSIGNED_BYTE;

    switch (format) {
    case SYN::gfx::gl::TextureFormat::Depth24:
        dataType = GL_FLOAT;
        break;
    case SYN::gfx::gl::TextureFormat::Depth24Stencil8:
        dataType = GL_UNSIGNED_INT_24_8;
        break;
    case SYN::gfx::gl::TextureFormat::RGB16F:
        dataType = GL_FLOAT;
        break;
    case SYN::gfx::gl::TextureFormat::RGB32F:
        dataType = GL_FLOAT;
        break;
    case SYN::gfx::gl::TextureFormat::RGBA16F:
        dataType = GL_FLOAT;
        break;
    case SYN::gfx::gl::TextureFormat::RGBA32F:
        dataType = GL_FLOAT;
        break;
    case SYN::gfx::gl::TextureFormat::RG16F:
        dataType = GL_FLOAT;
    case SYN::gfx::gl::TextureFormat::RG32F:
        dataType = GL_FLOAT;
    default:
        break;
    }

    return dataType;
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name, float v0) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniform1f(loc, v0);
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name, float v0,
                                     float v1) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniform2f(loc, v0, v1);
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name, float v0, float v1,
                                     float v2) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniform3f(loc, v0, v1, v2);
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name, float v0, float v1,
                                     float v2, float v3) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniform4f(loc, v0, v1, v2, v3);
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name, int32_t v0) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniform1i(loc, v0);
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name, int32_t v0,
                                     int32_t v1) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniform2i(loc, v0, v1);
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name, int32_t v0,
                                     int32_t v1, int32_t v2) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniform3i(loc, v0, v1, v2);
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name, int32_t v0,
                                     int32_t v1, int32_t v2, int32_t v3) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniform4i(loc, v0, v1, v2, v3);
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name, uint32_t v0) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniform1ui(loc, v0);
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name, uint32_t v0,
                                     uint32_t v1) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniform2ui(loc, v0, v1);
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name, uint32_t v0,
                                     uint32_t v1, uint32_t v2) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniform3ui(loc, v0, v1, v2);
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name, uint32_t v0,
                                     uint32_t v1, uint32_t v2, uint32_t v3) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniform4ui(loc, v0, v1, v2, v3);
}

void SYN::gfx::gl::Pass::bindUniform(std::string_view name,
                                     const glm::mat4 &v) {
    int loc = getShaderUniformLocation(name);
    if (loc == -1) {
        spdlog::warn("Shader uniform: {} does not exist!", name);
        return;
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(v));
}

SYN::gfx::gl::Context::Context(const ContextInitDesc &desc) {
    m_Window = desc.windowHandle;
}

SYN::gfx::gl::Context::~Context() {
    if (!m_MainContext)
        return;
    std::vector<VertexArray> vertexArrays =
        m_VertexArrayRegistry.getAllResources();
    for (VertexArray vao : vertexArrays)
        glDeleteVertexArrays(1, &vao.id);

    std::vector<Buffer> buffers = m_BufferRegistry.getAllResources();
    for (Buffer buffer : buffers)
        glDeleteBuffers(1, &buffer.id);

    std::vector<Shader> shaders = m_ShaderRegistry.getAllResources();
    for (Shader shader : shaders) {
        glDeleteProgram(shader.program);
        if (shader.vertex != 0)
            glDeleteShader(shader.vertex);
        if (shader.fragment != 0)
            glDeleteShader(shader.fragment);
    }

    std::vector<Texture> textures = m_TextureRegistry.getAllResources();
    for (Texture texture : textures)
        glDeleteTextures(1, &texture.id);

    std::vector<Sampler> samplers = m_SamplerRegistry.getAllResources();
    for (Sampler sampler : samplers)
        glDeleteSamplers(1, &sampler.id);

    std::vector<Framebuffer> framebuffers =
        m_FramebufferRegistry.getAllResources();
    for (Framebuffer framebuffer : framebuffers)
        glDeleteFramebuffers(1, &framebuffer.id);

    std::vector<Renderbuffer> renderbuffers =
        m_RenderbufferRegistry.getAllResources();
    for (Renderbuffer renderbuffer : renderbuffers)
        glDeleteRenderbuffers(1, &renderbuffer.id);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

SYN::gfx::gl::Context::Context(Context &&other) {
    m_MainContext = true;
    m_Window = other.m_Window;

    other.m_MainContext = false;
    other.m_Window = nullptr;
}

SYN::gfx::gl::Context &SYN::gfx::gl::Context::operator=(Context &&other) {
    if (this != &other) {
        m_MainContext = true;
        m_Window = other.m_Window;

        other.m_Window = nullptr;
        other.m_MainContext = false;
    }
    return *this;
}

void SYN::gfx::gl::Context::present() { glfwSwapBuffers(m_Window); }

std::optional<SYN::gfx::gl::Context>
SYN::gfx::gl::Context::createContext(const ContextInitDesc &desc) {
    if (desc.windowHandle == nullptr) {
        spdlog::error(
            "Unable to establish OpenGL context with NULL window handle.");
        return std::nullopt;
    }

    glfwMakeContextCurrent(desc.windowHandle);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        spdlog::error(
            "Unable to load OpenGL functions. Cannot proceed with context.");
        return std::nullopt;
    }

    glViewport(0, 0, desc.width, desc.height);

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // keyboard controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad; // gamepad controls (optional)

    ImGui_ImplGlfw_InitForOpenGL(desc.windowHandle, false);

    ImGui_ImplOpenGL3_Init("#version 450");

    return Context(desc);
}

SYN::gfx::gl::Pass SYN::gfx::gl::Context::beginPass(const PassDesc &desc) {
    return Pass(this, desc);
}

SYN::gfx::gl::Pass::Pass(Context *context, const PassDesc &desc) {
    assert(context != nullptr && "OpenGL pass context cannot be NULL!");

    m_ContextPtr = context;

    if (desc.framebufferHandle.has_value()) {
        std::optional<Framebuffer> framebuffer =
            context->getFramebuffer(desc.framebufferHandle.value());

        assert(framebuffer.has_value() &&
               "Framebuffer in render pass doesn't exist!");

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.value().id);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLbitfield clearMask = 0;

    if (desc.clearColor.has_value()) {
        clearMask |= GL_COLOR_BUFFER_BIT;
    }

    if (desc.enableDepthTest) {
        glEnable(GL_DEPTH_TEST);
        clearMask |= GL_DEPTH_BUFFER_BIT;
    } else
        glDisable(GL_DEPTH_TEST);

    if (desc.enableStencilTest) {
        glEnable(GL_STENCIL_TEST);
        clearMask |= GL_STENCIL_BUFFER_BIT;
    } else
        glDisable(GL_STENCIL_TEST);

    if (desc.viewportOverride.has_value()) {
        Viewport viewport = desc.viewportOverride.value();
        glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    }
    if (desc.scissorOverride.has_value()) {
        ScissorRect scissor = desc.scissorOverride.value();
        glScissor(scissor.x, scissor.y, scissor.width, scissor.height);
    }

    if (clearMask != 0) {
        if (desc.clearColor.has_value()) {
            glm::vec4 clearColor = desc.clearColor.value();
            glClearColor(clearColor.r, clearColor.g, clearColor.b,
                         clearColor.a);
        }
        glClear(clearMask);
    }
}

std::optional<SYN::gfx::gl::Handle<SYN::gfx::gl::Buffer>>
SYN::gfx::gl::Context::createBuffer(const BufferDesc &desc,
                                    const void *initialData) {
    GLenum memoryUsage = 0;
    if (desc.usage == MemoryUsage::CpuToGPU) {
        memoryUsage = GL_DYNAMIC_STORAGE_BIT;
    }

    Buffer buffer;
    buffer.type = desc.bufferType;
    glCreateBuffers(1, &buffer.id);
    glNamedBufferStorage(buffer.id, desc.size, initialData, memoryUsage);

    return m_BufferRegistry.createHandle(buffer);
}

void SYN::gfx::gl::Context::updateBuffer(Handle<Buffer> bufferHandle,
                                         uint32_t offset, uint32_t size,
                                         const void *data) {
    std::optional<Buffer> buffer = m_BufferRegistry.getResource(bufferHandle);
    if (!buffer.has_value())
        return;
    glNamedBufferSubData(buffer.value().id, offset, size, data);
}

void SYN::gfx::gl::Context::deleteBuffer(Handle<Buffer> bufferHandle) {
    std::optional<Buffer> buffer = m_BufferRegistry.getResource(bufferHandle);
    if (!buffer.has_value())
        return;
    uint32_t bufferId = buffer.value().id;
    m_PendingDeleteBuffers.push_back(bufferId);
    m_BufferRegistry.releaseHandle(bufferHandle);
}

std::optional<SYN::gfx::gl::Handle<SYN::gfx::gl::Framebuffer>>
SYN::gfx::gl::Context::createFramebuffer(const FramebufferDesc &desc) {
    Framebuffer framebuffer;
    glCreateFramebuffers(1, &framebuffer.id);

    size_t colorIndex = 0;
    for (const AttachmentDesc &colorAttachment : desc.colorAttachments) {
        if (std::holds_alternative<Handle<Texture>>(colorAttachment.handle)) {
            Texture texture = m_TextureRegistry
                                  .getResource(std::get<Handle<Texture>>(
                                      colorAttachment.handle))
                                  .value();
            glNamedFramebufferTexture(framebuffer.id,
                                      GL_COLOR_ATTACHMENT0 + colorIndex,
                                      texture.id, 0);
        }
        if (std::holds_alternative<Handle<Renderbuffer>>(
                colorAttachment.handle)) {
            Renderbuffer renderbuffer =
                m_RenderbufferRegistry
                    .getResource(
                        std::get<Handle<Renderbuffer>>(colorAttachment.handle))
                    .value();
            glNamedFramebufferRenderbuffer(framebuffer.id,
                                           GL_COLOR_ATTACHMENT0 + colorIndex,
                                           GL_RENDERBUFFER, renderbuffer.id);
        }

        ++colorIndex;
    }

    if (desc.depthStencilAttachment.has_value()) {
        const AttachmentDesc &attachmentDesc =
            desc.depthStencilAttachment.value();

        GLenum attachmentType = desc.isDepthOnly ? GL_DEPTH_ATTACHMENT
                                                 : GL_DEPTH_STENCIL_ATTACHMENT;

        if (std::holds_alternative<Handle<Texture>>(attachmentDesc.handle)) {
            Texture texture = m_TextureRegistry
                                  .getResource(std::get<Handle<Texture>>(
                                      attachmentDesc.handle))
                                  .value();

            glNamedFramebufferTexture(framebuffer.id, attachmentType,
                                      texture.id, 0);
        }
        if (std::holds_alternative<Handle<Renderbuffer>>(
                attachmentDesc.handle)) {
            Renderbuffer renderbuffer =
                m_RenderbufferRegistry
                    .getResource(
                        std::get<Handle<Renderbuffer>>(attachmentDesc.handle))
                    .value();
            glNamedFramebufferRenderbuffer(framebuffer.id, attachmentType,
                                           GL_RENDERBUFFER, renderbuffer.id);
        }
    }

    if (glCheckNamedFramebufferStatus(framebuffer.id, GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
        return std::nullopt;
    }

    return m_FramebufferRegistry.createHandle(framebuffer);
}

void SYN::gfx::gl::Context::setColorAttachment(Handle<Framebuffer> handle,
                                               uint32_t index,
                                               Handle<Texture> texture,
                                               uint32_t mip,
                                               std::optional<uint32_t> layer) {
    std::optional<Framebuffer> framebuffer =
        m_FramebufferRegistry.getResource(handle);
    if (!framebuffer.has_value())
        return;
    std::optional<Texture> textureOpt = m_TextureRegistry.getResource(texture);
    if (!textureOpt.has_value())
        return;

    uint32_t framebufferId = framebuffer.value().id;
    uint32_t textureId = textureOpt.value().id;

    if (layer.has_value()) {
        glNamedFramebufferTextureLayer(framebufferId,
                                       GL_COLOR_ATTACHMENT0 + index, textureId,
                                       mip, layer.value());
    } else {
        glNamedFramebufferTexture(framebufferId, GL_COLOR_ATTACHMENT0 + index,
                                  textureId, mip);
    }

    if (glCheckNamedFramebufferStatus(framebufferId, GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("Unable to set color attachment for framebuffer!");
    }
}

void SYN::gfx::gl::Context::deleteFramebuffer(
    Handle<Framebuffer> framebufferHandle) {
    std::optional<Framebuffer> framebuffer =
        m_FramebufferRegistry.getResource(framebufferHandle);

    if (!framebuffer.has_value())
        return;

    uint32_t framebufferId = framebuffer.value().id;
    m_PendingDeleteFramebuffers.push_back(framebufferId);

    m_FramebufferRegistry.releaseHandle(framebufferHandle);
}

void SYN::gfx::gl::Context::blitFramebuffer(
    std::optional<Handle<Framebuffer>> readHandle,
    std::optional<Handle<Framebuffer>> writeHandle, Viewport sourceRect,
    Viewport destRect) {

    if (!readHandle) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    } else {
        Framebuffer framebuffer =
            m_FramebufferRegistry.getResource(readHandle.value()).value();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.id);
    }

    if (!writeHandle) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    } else {
        Framebuffer framebuffer =
            m_FramebufferRegistry.getResource(writeHandle.value()).value();

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.id);
    }

    glBlitFramebuffer(sourceRect.x, sourceRect.y, sourceRect.width,
                      sourceRect.height, destRect.x, destRect.y, destRect.width,
                      destRect.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

std::optional<SYN::gfx::gl::Handle<SYN::gfx::gl::Renderbuffer>>
SYN::gfx::gl::Context::createRenderbuffer(const RenderbufferDesc &desc) {
    Renderbuffer renderbuffer;
    glCreateRenderbuffers(1, &renderbuffer.id);
    if (desc.sampleCount > 1) {
        glNamedRenderbufferStorageMultisample(
            renderbuffer.id, desc.sampleCount,
            getInternalTextureFormat(desc.format), desc.width, desc.height);
    } else {
        glNamedRenderbufferStorage(renderbuffer.id,
                                   getInternalTextureFormat(desc.format),
                                   desc.width, desc.height);
    }
    return m_RenderbufferRegistry.createHandle(renderbuffer);
}

void SYN::gfx::gl::Context::deleteRenderbuffer(
    Handle<Renderbuffer> renderbufferHandle) {
    std::optional<Renderbuffer> renderbufferOpt =
        m_RenderbufferRegistry.getResource(renderbufferHandle);

    if (!renderbufferOpt.has_value())
        return;

    Renderbuffer renderbuffer = renderbufferOpt.value();

    m_PendingDeleteRenderbuffers.push_back(renderbuffer.id);
    m_RenderbufferRegistry.releaseHandle(renderbufferHandle);
}

bool validateShaderCompileStatus(uint32_t shader, bool isVertex) {
    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        constexpr size_t LOG_BUFFER_SIZE = 512;
        char buf[LOG_BUFFER_SIZE];
        glGetShaderInfoLog(shader, LOG_BUFFER_SIZE, nullptr, buf);
        std::string_view shaderType = isVertex ? "VERTEX" : "FRAGMENT";
        spdlog::error("{} SHADER ERROR:\n{}\n", shaderType, buf);
        return false;
    }
    return true;
}

bool validateProgramLinkStatus(uint32_t program) {
    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        constexpr size_t LOG_BUFFER_SIZE = 512;
        char buf[LOG_BUFFER_SIZE];
        glGetProgramInfoLog(program, LOG_BUFFER_SIZE, nullptr, buf);
        spdlog::error("PROGRAM LINK ERROR:\n{}\n", buf);
        return false;
    }
    return true;
}

std::optional<SYN::gfx::gl::Handle<SYN::gfx::gl::Shader>>
SYN::gfx::gl::Context::createShader(std::string_view vertexSource,
                                    std::string_view fragmentSource) {
    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    const char *vertexSourceChar = vertexSource.data();
    const char *fragmentSourceChar = fragmentSource.data();

    glShaderSource(vertexShader, 1, &vertexSourceChar, nullptr);
    glShaderSource(fragmentShader, 1, &fragmentSourceChar, nullptr);

    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);

    bool isVertexSuccessful = validateShaderCompileStatus(vertexShader, true);
    bool isFragmentSuccessful =
        validateShaderCompileStatus(fragmentShader, false);

    if (!isVertexSuccessful || !isFragmentSuccessful) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return std::nullopt;
    }

    uint32_t program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    if (!validateProgramLinkStatus(program)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return std::nullopt;
    }

    return m_ShaderRegistry.createHandle(
        Shader{program, vertexShader, fragmentShader});
}

std::optional<SYN::gfx::gl::Handle<SYN::gfx::gl::Shader>>
SYN::gfx::gl::Context::createShader(uint32_t vertexId, uint32_t fragmentId) {
    uint32_t program = glCreateProgram();
    glAttachShader(program, vertexId);
    glAttachShader(program, fragmentId);
    glLinkProgram(program);

    if (!validateProgramLinkStatus(program)) {
        glDeleteProgram(program);
        return std::nullopt;
    }

    return m_ShaderRegistry.createHandle(Shader{program, 0, 0});
}

void SYN::gfx::gl::Context::deleteShader(Handle<Shader> shaderHandle) {
    std::optional<Shader> shader = m_ShaderRegistry.getResource(shaderHandle);
    if (!shader.has_value())
        return;
    uint32_t programId = shader.value().program;
    uint32_t vertexId = shader.value().vertex;
    uint32_t fragmentId = shader.value().fragment;

    if (Globals.shaderUniformCache.find(programId) !=
        Globals.shaderUniformCache.cend()) {
        Globals.shaderUniformCache.erase(programId);
    }

    m_PendingDeletePrograms.push_back(programId);
    if (vertexId != 0)
        m_PendingDeleteShaders.push_back(vertexId);
    if (fragmentId != 0)
        m_PendingDeleteShaders.push_back(fragmentId);

    m_ShaderRegistry.releaseHandle(shaderHandle);
}

std::optional<SYN::gfx::gl::Handle<SYN::gfx::gl::VertexArray>>
SYN::gfx::gl::Context::createVertexArray(const VertexArrayDesc &desc) {
    std::optional<Buffer> vertexBuffer =
        m_BufferRegistry.getResource(desc.vertexBufferHandle);
    if (!vertexBuffer.has_value()) {
        return std::nullopt;
    }

    uint32_t vaoId;
    glCreateVertexArrays(1, &vaoId);

    for (uint32_t i = 0; i < desc.attributeCount; ++i) {
        VertexAttribDesc attribute = desc.attributes[i];
        glEnableVertexArrayAttrib(vaoId, attribute.location);
        glVertexArrayAttribBinding(vaoId, attribute.location, 0);

        GLenum type = GL_FLOAT;

        uint32_t size = 1;
        switch (attribute.format) {
        case VertexFormat::Float1:
            size = 1;
            break;
        case VertexFormat::Float2:
            size = 2;
            break;
        case VertexFormat::Float3:
            size = 3;
            break;
        case VertexFormat::Float4:
            size = 4;
            break;
        case VertexFormat::Int1:
            type = GL_INT;
            size = 1;
            break;
        case VertexFormat::Int2:
            type = GL_INT;
            size = 2;
            break;
        case VertexFormat::Int3:
            type = GL_INT;
            size = 3;
            break;
        case VertexFormat::Int4:
            type = GL_INT;
            size = 4;
            break;
        };

        glVertexArrayAttribFormat(vaoId, attribute.location, size, type,
                                  attribute.normalized, attribute.offset);
    }

    assert(vertexBuffer.value().type == BufferType::Vertex);
    glVertexArrayVertexBuffer(vaoId, 0, vertexBuffer.value().id, 0,
                              desc.vertexBufferStride);
    std::optional<Buffer> indexBuffer =
        m_BufferRegistry.getResource(desc.indexBufferHandle);
    std::optional<IndexType> indexType = std::nullopt;
    if (indexBuffer.has_value()) {
        indexType = desc.indexType;
        assert(indexBuffer.value().type == BufferType::Index);
        glVertexArrayElementBuffer(vaoId, indexBuffer.value().id);
    }

    return m_VertexArrayRegistry.createHandle(VertexArray{vaoId, indexType});
}

void SYN::gfx::gl::Context::deleteVertexArray(
    Handle<VertexArray> vertexArrayHandle) {
    std::optional<VertexArray> vertexArray =
        m_VertexArrayRegistry.getResource(vertexArrayHandle);
    if (!vertexArray.has_value())
        return;
    uint32_t vaoId = vertexArray.value().id;
    m_PendingDeleteVertexArrays.push_back(vaoId);

    m_VertexArrayRegistry.releaseHandle(vertexArrayHandle);
}

void SYN::gfx::gl::Context::flushDeferredDeletes() {
    for (uint32_t shader : m_PendingDeleteShaders) {
        glDeleteShader(shader);
    }
    for (uint32_t program : m_PendingDeletePrograms) {
        glDeleteProgram(program);
    }

    if (!m_PendingDeleteVertexArrays.empty()) {
        glDeleteVertexArrays(m_PendingDeleteVertexArrays.size(),
                             &m_PendingDeleteVertexArrays[0]);
    }

    if (!m_PendingDeleteBuffers.empty()) {
        glDeleteBuffers(m_PendingDeleteBuffers.size(),
                        &m_PendingDeleteBuffers[0]);
    }

    if (!m_PendingDeleteTextures.empty()) {
        glDeleteTextures(m_PendingDeleteTextures.size(),
                         &m_PendingDeleteTextures[0]);
    }

    if (!m_PendingDeleteSamplers.empty()) {
        glDeleteSamplers(m_PendingDeleteSamplers.size(),
                         &m_PendingDeleteSamplers[0]);
    }

    if (!m_PendingDeleteFramebuffers.empty()) {
        glDeleteFramebuffers(m_PendingDeleteFramebuffers.size(),
                             &m_PendingDeleteFramebuffers[0]);
    }

    if (!m_PendingDeleteRenderbuffers.empty()) {
        glDeleteRenderbuffers(m_PendingDeleteRenderbuffers.size(),
                              &m_PendingDeleteRenderbuffers[0]);
    }

    m_PendingDeleteShaders.clear();
    m_PendingDeletePrograms.clear();
    m_PendingDeleteVertexArrays.clear();
    m_PendingDeleteBuffers.clear();
    m_PendingDeleteTextures.clear();
    m_PendingDeleteSamplers.clear();
    m_PendingDeleteFramebuffers.clear();
    m_PendingDeleteRenderbuffers.clear();
}

std::optional<SYN::gfx::gl::VertexArray>
SYN::gfx::gl::Context::getVertexArray(Handle<VertexArray> vertexArrayHandle) {
    return m_VertexArrayRegistry.getResource(vertexArrayHandle);
}
std::optional<SYN::gfx::gl::Buffer>
SYN::gfx::gl::Context::getBuffer(Handle<Buffer> bufferHandle) {
    return m_BufferRegistry.getResource(bufferHandle);
}

std::optional<SYN::gfx::gl::Texture>
SYN::gfx::gl::Context::getTexture(Handle<Texture> textureHandle) {
    return m_TextureRegistry.getResource(textureHandle);
}
std::optional<SYN::gfx::gl::Sampler>
SYN::gfx::gl::Context::getSampler(Handle<Sampler> samplerHandle) {
    return m_SamplerRegistry.getResource(samplerHandle);
}

std::optional<SYN::gfx::gl::Shader>
SYN::gfx::gl::Context::getShader(Handle<Shader> shaderHandle) {
    return m_ShaderRegistry.getResource(shaderHandle);
}

std::optional<SYN::gfx::gl::Framebuffer>
SYN::gfx::gl::Context::getFramebuffer(Handle<Framebuffer> framebufferHandle) {
    return m_FramebufferRegistry.getResource(framebufferHandle);
}

std::optional<SYN::gfx::gl::Renderbuffer>
SYN::gfx::gl::Context::getRenderbuffer(
    Handle<Renderbuffer> renderbufferHandle) {
    return m_RenderbufferRegistry.getResource(renderbufferHandle);
}

SYN::gfx::gl::Pass::~Pass() {
    glUseProgram(0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    for (uint32_t slot : m_TextureSlotsBound) {
        glBindTextureUnit(slot, 0);
        glBindSampler(slot, 0);
    }
}

void SYN::gfx::gl::Pass::usePipeline(const PipelineState &pipelineState) {
    std::optional<Shader> currentShaderOpt =
        m_ContextPtr->getShader(pipelineState.shader);
    assert(currentShaderOpt.has_value() &&
           "Shader handle is invalid. Cannot set pipeline.");
    Shader shader = currentShaderOpt.value();

    m_CurrentShader = shader;

    switch (pipelineState.cullMode) {
    case CullMode::None:
        glDisable(GL_CULL_FACE);
        break;
    case CullMode::Front:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        break;
    case CullMode::Back:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        break;
    }

    if (pipelineState.frontFaceCcw)
        glFrontFace(GL_CCW);
    else
        glFrontFace(GL_CW);

    switch (pipelineState.polygonMode) {
    case PolygonMode::Fill:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    case PolygonMode::Line:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case PolygonMode::Point:
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
    }

    m_CurrentDrawTopology = pipelineState.topology;

    glUseProgram(shader.program);
}

void SYN::gfx::gl::Pass::bindVertexArray(
    Handle<VertexArray> vertexArrayHandle) {
    std::optional<VertexArray> currentVaoOpt =
        m_ContextPtr->getVertexArray(vertexArrayHandle);
    assert(currentVaoOpt.has_value() && "Vertex Array handle is invalid.");
    VertexArray vertexArray = currentVaoOpt.value();
    glBindVertexArray(vertexArray.id);
    if (vertexArray.indexType.has_value()) {
        m_CurrentIndexType = vertexArray.indexType.value();
    } else {
        m_CurrentIndexType = IndexType::Unsigned32;
    }
}

void SYN::gfx::gl::Pass::draw(uint32_t vertexCount, uint32_t firstVertex) {
    GLenum drawMode;
    switch (m_CurrentDrawTopology) {
    case PrimitiveTopology::Triangles:
        drawMode = GL_TRIANGLES;
        break;
    case PrimitiveTopology::Lines:
        drawMode = GL_LINES;
        break;
    case PrimitiveTopology::Points:
        drawMode = GL_POINTS;
        break;
    }
    glDrawArrays(drawMode, firstVertex, vertexCount);
}

void SYN::gfx::gl::Pass::drawIndexed(uint32_t indexCount) {
    GLenum drawMode;
    switch (m_CurrentDrawTopology) {
    case PrimitiveTopology::Triangles:
        drawMode = GL_TRIANGLES;
        break;
    case PrimitiveTopology::Lines:
        drawMode = GL_LINES;
        break;
    case PrimitiveTopology::Points:
        drawMode = GL_POINTS;
        break;
    }

    GLenum indexType;
    switch (m_CurrentIndexType) {
    case IndexType::Unsigned16:
        indexType = GL_UNSIGNED_SHORT;
        break;
    case IndexType::Unsigned32:
        indexType = GL_UNSIGNED_INT;
        break;
    }

    glDrawElements(drawMode, indexCount, indexType, nullptr);
}

std::optional<SYN::gfx::gl::Handle<SYN::gfx::gl::Texture>>
SYN::gfx::gl::Context::createTexture(const TextureDesc &desc,
                                     const void *initialData) {
    Texture texture{};

    GLenum internalFormat = getInternalTextureFormat(desc.format);
    GLenum format = getTextureFormat(desc.format);
    GLenum dataType = getTextureDataTypeFromFormat(desc.format);

    if (desc.type == TextureType::Tex2D) {
        if (desc.sampleCount > 1) {
            glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &texture.id);
            glTextureStorage2DMultisample(texture.id, desc.sampleCount,
                                          internalFormat, desc.width,
                                          desc.height, GL_TRUE);
        } else {
            glCreateTextures(GL_TEXTURE_2D, 1, &texture.id);
            glTextureStorage2D(texture.id, desc.mipLevel, internalFormat,
                               desc.width, desc.height);
        }

        if (initialData != nullptr) {
            glTextureSubImage2D(texture.id, 0, 0, 0, desc.width, desc.height,
                                format, dataType, initialData);
        }
    }

    if (desc.type == TextureType::Cubemap) {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture.id);
        assert(desc.width == desc.height &&
               "Width and height must be the same for cubemap textures!");
        glTextureStorage2D(texture.id, desc.mipLevel, internalFormat,
                           desc.width, desc.height);
        if (initialData != nullptr) {
            glTextureSubImage3D(texture.id, 0, 0, 0, 0, desc.width, desc.height,
                                6, format, dataType, initialData);
        }
    }

    if (desc.mipLevel > 1) {
        glGenerateTextureMipmap(texture.id);
    }

    texture.type = desc.type;

    return m_TextureRegistry.createHandle(texture);
}

void SYN::gfx::gl::Context::deleteTexture(Handle<Texture> textureHandle) {
    std::optional<Texture> textureOpt =
        m_TextureRegistry.getResource(textureHandle);

    if (!textureOpt.has_value())
        return;

    Texture texture = textureOpt.value();

    m_PendingDeleteTextures.push_back(texture.id);
    m_TextureRegistry.releaseHandle(textureHandle);
}

std::optional<SYN::gfx::gl::Handle<SYN::gfx::gl::Sampler>>
SYN::gfx::gl::Context::createSampler(const SamplerDesc &desc) {
    Sampler sampler;

    auto getFilterFormat = [](SampleFilter filter) {
        switch (filter) {
        case SampleFilter::Nearest:
            return GL_NEAREST;
        case SampleFilter::Linear:
            return GL_LINEAR;
        case SampleFilter::Nearest_Mipmap_Nearest:
            return GL_NEAREST_MIPMAP_NEAREST;
        default:
            return GL_LINEAR_MIPMAP_LINEAR;
        }
    };

    auto getWrapFormat = [](WrapMode wrap) {
        switch (wrap) {
        case WrapMode::ClampToEdge:
            return GL_CLAMP_TO_EDGE;
        case WrapMode::Repeat:
            return GL_REPEAT;
        default:
            return GL_MIRRORED_REPEAT;
        }
    };

    glCreateSamplers(1, &sampler.id);
    glSamplerParameteri(sampler.id, GL_TEXTURE_MIN_FILTER,
                        getFilterFormat(desc.minFilter));
    glSamplerParameteri(sampler.id, GL_TEXTURE_MAG_FILTER,
                        getFilterFormat(desc.magFilter));
    glSamplerParameteri(sampler.id, GL_TEXTURE_WRAP_S,
                        getWrapFormat(desc.wrapU));
    glSamplerParameteri(sampler.id, GL_TEXTURE_WRAP_T,
                        getWrapFormat(desc.wrapV));
    glSamplerParameteri(sampler.id, GL_TEXTURE_WRAP_R,
                        getWrapFormat(desc.wrapW));

    // TODO: Allow setting anisotropy level
    float maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    glSamplerParameterf(sampler.id, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);

    return m_SamplerRegistry.createHandle(sampler);
}

void SYN::gfx::gl::Context::deleteSampler(Handle<Sampler> samplerHandle) {
    std::optional<Sampler> samplerOpt =
        m_SamplerRegistry.getResource(samplerHandle);

    if (!samplerOpt.has_value())
        return;

    Sampler sampler = samplerOpt.value();
    m_PendingDeleteSamplers.push_back(sampler.id);

    m_SamplerRegistry.releaseHandle(samplerHandle);
}

void SYN::gfx::gl::Pass::bindTexture(uint32_t binding,
                                     Handle<Texture> textureHandle,
                                     Handle<Sampler> samplerHandle) {
    std::optional<Texture> textureOpt = m_ContextPtr->getTexture(textureHandle);
    std::optional<Sampler> samplerOpt = m_ContextPtr->getSampler(samplerHandle);

    assert(textureOpt.has_value() && "Texture handle is invalid");
    assert(samplerOpt.has_value() && "Sampler handle is invalid");

    Texture texture = textureOpt.value();
    Sampler sampler = samplerOpt.value();

    glBindTextureUnit(binding, texture.id);
    glBindSampler(binding, sampler.id);

    m_TextureSlotsBound.insert(binding);
}

void SYN::gfx::gl::Context::updateTexture(Handle<Texture> textureHandle,
                                          uint32_t mipLevel, uint32_t x,
                                          uint32_t y, uint32_t width,
                                          uint32_t height, uint32_t face,
                                          TextureFormat format,
                                          const void *data) {
    std::optional<Texture> textureOpt =
        m_TextureRegistry.getResource(textureHandle);

    if (!textureOpt.has_value())
        return;

    Texture texture = textureOpt.value();

    GLenum glFormat = getTextureFormat(format);
    GLenum dataType = getTextureDataTypeFromFormat(format);

    if (texture.type == TextureType::Tex2D) {
        glTextureSubImage2D(texture.id, mipLevel, x, y, width, height, glFormat,
                            dataType, data);
    }
    if (texture.type == TextureType::Cubemap) {
        glTextureSubImage3D(texture.id, mipLevel, x, y, face, width, height, 1,
                            glFormat, dataType, data);
    }
}

void SYN::gfx::gl::Context::generateMipmap(Handle<Texture> textureHandle) {
    std::optional<Texture> textureOpt =
        m_TextureRegistry.getResource(textureHandle);

    if (!textureOpt.has_value())
        return;

    Texture texture = textureOpt.value();

    glGenerateTextureMipmap(texture.id);
}

void SYN::gfx::gl::Pass::bindUniformBuffer(uint32_t binding,
                                           Handle<Buffer> bufferHandle) {
    std::optional<Buffer> buffer = m_ContextPtr->getBuffer(bufferHandle);
    if (!buffer.has_value())
        return;
    if (buffer.value().type != BufferType::Uniform)
        return;
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer.value().id);
}

SYN::gfx::gl::Renderer::Renderer(const RendererConfig &config) {
    m_RenderConfig = config;
}

SYN::gfx::gl::Renderer::~Renderer() {}

SYN::gfx::gl::Handle<SYN::gfx::gl::Texture>
createDefaultColoredTexture(SYN::gfx::gl::Context &context,
                            std::array<uint8_t, 4> color) {
    SYN::gfx::gl::TextureDesc desc{
        .width = 1,
        .height = 1,
        .format = SYN::gfx::gl::TextureFormat::SRGBA,
    };
    return context.createTexture(desc, &color[0]).value();
}

int getMipLevel(int width, int height) {
    return 1 + static_cast<int>(std::floor(std::log2(std::max(width, height))));
}

SYN::gfx::gl::Material loadMaterial(
    SYN::gfx::gl::Context &context,
    const SYN::gfx::gl::MaterialData &materialData,
    std::unordered_map<std::string, SYN::gfx::gl::Handle<SYN::gfx::gl::Texture>>
        &textureCache) {
    SYN::gfx::gl::Material material;
    if (materialData.albedoData) {
        SYN::gfx::gl::TextureData albedoTexture =
            materialData.albedoData.value();
        if (textureCache.find(albedoTexture.sourcePath) !=
            textureCache.cend()) {
            material.albedo = textureCache.at(albedoTexture.sourcePath);
        } else {
            albedoTexture.info.mipLevel = getMipLevel(
                albedoTexture.info.width, albedoTexture.info.height);
            material.albedo =
                context
                    .createTexture(albedoTexture.info, &albedoTexture.data[0])
                    .value();
            textureCache[albedoTexture.sourcePath] = material.albedo.value();
        }
    }
    if (materialData.normalData) {
        SYN::gfx::gl::TextureData normalTexture =
            materialData.normalData.value();
        if (textureCache.find(normalTexture.sourcePath) !=
            textureCache.cend()) {
            material.normalMap = textureCache.at(normalTexture.sourcePath);
        } else {
            normalTexture.info.mipLevel = getMipLevel(
                normalTexture.info.width, normalTexture.info.height);
            material.normalMap =
                context
                    .createTexture(normalTexture.info, &normalTexture.data[0])
                    .value();
            textureCache[normalTexture.sourcePath] = material.normalMap.value();
        }
    }
    if (materialData.metallicRoughnessData) {
        SYN::gfx::gl::TextureData metallicRoughnessTexture =
            materialData.metallicRoughnessData.value();
        if (textureCache.find(metallicRoughnessTexture.sourcePath) !=
            textureCache.cend()) {
            material.metallicRoughnessMap =
                textureCache.at(metallicRoughnessTexture.sourcePath);
        } else {
            metallicRoughnessTexture.info.mipLevel =
                getMipLevel(metallicRoughnessTexture.info.width,
                            metallicRoughnessTexture.info.height);
            material.metallicRoughnessMap =
                context
                    .createTexture(metallicRoughnessTexture.info,
                                   &metallicRoughnessTexture.data[0])
                    .value();
            textureCache[metallicRoughnessTexture.sourcePath] =
                material.metallicRoughnessMap.value();
        }
    }
    material.metallic = materialData.metallic;
    material.roughness = materialData.roughness;
    material.tint = materialData.tint;

    return material;
}

uint32_t getShaderFeatures(bool hasNormal, bool hasMetallicRoughness,
                           bool hasSkin) {
    uint32_t featureFlag = 0;

    featureFlag |=
        hasSkin ? (uint32_t)SYN::gfx::gl::ShaderFeatures::Skinned : 0;

    featureFlag |=
        hasMetallicRoughness
            ? (uint32_t)SYN::gfx::gl::ShaderFeatures::MetallicRoughness
            : 0;

    featureFlag |=
        hasNormal ? (uint32_t)SYN::gfx::gl::ShaderFeatures::Normal : 0;

    return featureFlag;
}

SYN::gfx::gl::Mesh createMesh(
    SYN::gfx::gl::Context &context, const SYN::gfx::gl::MeshData &meshData,
    std::unordered_map<std::string, SYN::gfx::gl::Handle<SYN::gfx::gl::Texture>>
        &textureCache,
    SYN::gfx::gl::ShaderCache &shaderCache) {
    SYN::gfx::gl::Mesh mesh;

    mesh.vbo = context
                   .createBuffer({SYN::gfx::gl::BufferType::Vertex,
                                  SYN::gfx::gl::MemoryUsage::CpuToGPU,
                                  uint32_t(sizeof(SYN::gfx::gl::Vertex) *
                                           meshData.vertices.size())},
                                 &meshData.vertices[0])
                   .value();

    mesh.ebo = context
                   .createBuffer(
                       {SYN::gfx::gl::BufferType::Index,
                        SYN::gfx::gl::MemoryUsage::CpuToGPU,
                        uint32_t(sizeof(uint32_t) * meshData.indices.size())},
                       &meshData.indices[0])
                   .value();

    mesh.vao = context
                   .createVertexArray(
                       {mesh.vbo,
                        sizeof(SYN::gfx::gl::Vertex),
                        mesh.ebo,
                        {{
                            {0, SYN::gfx::gl::VertexFormat::Float3,
                             offsetof(SYN::gfx::gl::Vertex, position)},

                            {1, SYN::gfx::gl::VertexFormat::Float3,
                             offsetof(SYN::gfx::gl::Vertex, normal)},

                            {2, SYN::gfx::gl::VertexFormat::Float2,
                             offsetof(SYN::gfx::gl::Vertex, uv)},

                            {3, SYN::gfx::gl::VertexFormat::Float4,
                             offsetof(SYN::gfx::gl::Vertex, tangent)},

                            {4, SYN::gfx::gl::VertexFormat::Int4,
                             offsetof(SYN::gfx::gl::Vertex, boneIndices)},

                            {5, SYN::gfx::gl::VertexFormat::Float4,
                             offsetof(SYN::gfx::gl::Vertex, boneWeights)},
                        }},
                        6})
                   .value();

    mesh.indexCount = meshData.indices.size();
    mesh.localTransform = meshData.localTransform;
    mesh.material = loadMaterial(context, meshData.material, textureCache);

    mesh.shader = shaderCache.getShaderHandle(
        context,
        getShaderFeatures(mesh.material.normalMap.has_value(),
                          mesh.material.metallicRoughnessMap.has_value(),
                          meshData.hasSkin));

    return mesh;
}

std::optional<SYN::gfx::gl::Handle<SYN::gfx::gl::Model>>
SYN::gfx::gl::Renderer::createModel(Context &context, const ModelData &data) {
    Model model;
    for (const MeshData &meshData : data.meshes) {
        model.meshes.emplace_back(
            createMesh(context, meshData, m_TextureCache, m_ShaderCache));
        Mesh &mesh = model.meshes.back();
        if (!mesh.material.albedo) {
            mesh.material.albedo = m_DefaultWhite;
        }
        if (!m_DefaultModelSampler) {
            m_DefaultModelSampler = context.createSampler(
                {SampleFilter::Linear_Mipmap_Linear, SampleFilter::Linear,
                 WrapMode::Repeat, WrapMode::Repeat});
        }
        mesh.material.sampler = m_DefaultModelSampler;
    }
    return m_ModelRegistry.createHandle(model);
}

// TODO: Clean up environment function!
// Order of faces: Right, Left, Top, Bottom, Back, Front
SYN::gfx::gl::Handle<SYN::gfx::gl::Texture>
SYN::gfx::gl::Renderer::loadCubemap(Context &context,
                                    const std::array<uint8_t *, 6> &faces,
                                    uint32_t width, uint32_t height) {
    Handle<Texture> cubemap =
        context
            .createTexture({width, height, TextureFormat::SRGBA, 1, 1,
                            TextureType::Cubemap})
            .value();
    for (uint32_t i = 0; i < faces.size(); ++i) {
        context.updateTexture(cubemap, 0, 0, 0, width, height, i,
                              TextureFormat::SRGBA, faces.at(i));
    }

    return cubemap;
}

SYN::gfx::gl::Handle<SYN::gfx::gl::Texture>
SYN::gfx::gl::Renderer::loadCubemapFromEquirectangularTexture(Context &context,
                                                              float *data,
                                                              uint32_t width,
                                                              uint32_t height) {
    Handle<Texture> envCubemap =
        context
            .createTexture(
                {512, 512, TextureFormat::RGBA16F, 10, 1, TextureType::Cubemap})
            .value();

    Handle<Texture> hdrTexture =
        context.createTexture({width, height, TextureFormat::RGBA16F}, data)
            .value();

    Handle<Framebuffer> equirectangularProjection =
        context.createFramebuffer({std::array{AttachmentDesc{envCubemap}}})
            .value();

    Handle<Sampler> mipSampler =
        context
            .createSampler({SampleFilter::Linear_Mipmap_Linear,
                            SampleFilter::Linear, WrapMode::ClampToEdge,
                            WrapMode::ClampToEdge, WrapMode::ClampToEdge})
            .value();

    // https://learnopengl.com/code_viewer_gh.php?code=src/6.pbr/2.1.1.ibl_irradiance_conversion/ibl_irradiance_conversion.cpp
    glm::mat4 captureProjection =
        glm::perspectiveRH_NO(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                      glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                      glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                      glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                      glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                      glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                      glm::vec3(0.0f, -1.0f, 0.0f))};

    PipelineState pipeline;
    pipeline.shader = m_EquirectangularToCubemapShader.value();
    for (uint32_t i = 0; i < 6; ++i) {
        context.setColorAttachment(equirectangularProjection, 0, envCubemap, 0,
                                   i);
        Pass pass =
            context.beginPass({equirectangularProjection, glm::vec4(1.0f),
                               false, false, Viewport{0, 0, 512, 512}});

        pass.usePipeline(pipeline);
        pass.bindTexture(0, hdrTexture, mipSampler);
        pass.bindUniform("u_ViewProjection",
                         captureProjection * captureViews[i]);
        pass.bindUniform("u_equirectangularMap", 0);
        pass.bindVertexArray(m_SkyboxCube.value());
        pass.draw(36);
    }

    context.generateMipmap(envCubemap);

    context.deleteTexture(hdrTexture);
    context.deleteFramebuffer(equirectangularProjection);
    context.deleteSampler(mipSampler);

    return envCubemap;
}

void SYN::gfx::gl::Renderer::createIrradianceShader(Context &context) {
    if (m_IrradianceShader.has_value())
        return;

    std::fstream vertexFile("resources/shaders/gl_skybox.vert");
    if (!vertexFile) {
        spdlog::error("OpenGL renderer could not open vertex shader (Skybox).");
        return;
    }
    std::fstream fragmentFile("resources/shaders/gl_irradiance.frag");
    if (!fragmentFile) {
        spdlog::error(
            "OpenGL renderer could not open fragment shader (Irradiance).");
        return;
    }

    m_IrradianceShader =
        context
            .createShader(
                std::string(std::istreambuf_iterator<char>(vertexFile),
                            std::istreambuf_iterator<char>()),
                std::string(std::istreambuf_iterator<char>(fragmentFile),
                            std::istreambuf_iterator<char>()))
            .value();

    float irradianceData[] = {0.3f, 0.3f, 0.3f, 1.0f};
    m_DefaultIrradianceMap = context
                                 .createTexture({1, 1, TextureFormat::RGBA16F,
                                                 1, 1, TextureType::Cubemap})
                                 .value();
    for (uint32_t i = 0; i < 6; ++i) {
        context.updateTexture(m_DefaultIrradianceMap, 0, 0, 0, 1, 1, i,
                              TextureFormat::RGBA16F, &irradianceData[0]);
    }
}

void SYN::gfx::gl::Renderer::createPrefilterShader(Context &context) {
    if (m_PrefilterShader.has_value())
        return;

    std::fstream vertexFile("resources/shaders/gl_skybox.vert");
    if (!vertexFile) {
        spdlog::error("OpenGL renderer could not open vertex shader (Skybox).");
        return;
    }
    std::fstream fragmentFile("resources/shaders/gl_prefiltered_env.frag");
    if (!fragmentFile) {
        spdlog::error("OpenGL renderer could not open fragment shader "
                      "(Prefiltered map).");
        return;
    }

    m_PrefilterShader =
        context
            .createShader(
                std::string(std::istreambuf_iterator<char>(vertexFile),
                            std::istreambuf_iterator<char>()),
                std::string(std::istreambuf_iterator<char>(fragmentFile),
                            std::istreambuf_iterator<char>()))
            .value();
}

void SYN::gfx::gl::Renderer::createBRDFLutShader(Context &context) {
    if (m_BRDFLutShader.has_value())
        return;

    std::fstream vertexFile("resources/shaders/hdr.vert");
    if (!vertexFile) {
        spdlog::error("OpenGL renderer could not open vertex shader (HDR).");
        return;
    }
    std::fstream fragmentFile("resources/shaders/gl_brdf_lut.frag");
    if (!fragmentFile) {
        spdlog::error("OpenGL renderer could not open fragment shader "
                      "(BRDF LUT).");
        return;
    }

    m_BRDFLutShader =
        context
            .createShader(
                std::string(std::istreambuf_iterator<char>(vertexFile),
                            std::istreambuf_iterator<char>()),
                std::string(std::istreambuf_iterator<char>(fragmentFile),
                            std::istreambuf_iterator<char>()))
            .value();
}

void SYN::gfx::gl::Renderer::createDefaultPrefilterMap(Context &context) {
    float irradianceData[] = {0.0f, 0.0f, 0.0f, 0.0f};
    m_DefaultPrefilterMap = context
                                .createTexture({1, 1, TextureFormat::RGBA16F, 1,
                                                1, TextureType::Cubemap})
                                .value();
}

void SYN::gfx::gl::Renderer::init(Context &context) {
    createScreenQuad(context);
    createHdrShader(context);
    createSkybox(context);
    createIrradianceShader(context);
    createPrefilterShader(context);
    createBRDFLutShader(context);

    m_DefaultWhite = createDefaultColoredTexture(context, {255, 255, 255, 255});
    createDefaultPrefilterMap(context);

    m_BRDFLut = createBRDFLut(context);
    m_DefaultSampler = context.createSampler({}).value();

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void SYN::gfx::gl::Renderer::setEnvironment(
    std::optional<Environment> environment) {
    m_Environment = environment;
}

// TODO: (Implement this!)
void SYN::gfx::gl::Renderer::destroyModel(Context &context,
                                          Handle<Model> meshHandle) {}

void SYN::gfx::gl::Renderer::beginFrame(const Camera &camera) {
    m_MainCamera = camera;
}

void SYN::gfx::gl::Renderer::setDirectionalLight(
    const DirectionalLight &light) {
    m_DirectionalLight = light;
    m_DirectionalLight.direction = glm::normalize(m_DirectionalLight.direction);
}

void SYN::gfx::gl::Renderer::submit(
    Handle<Model> modelHandle, const glm::mat4 &transform,
    std::span<const MaterialOverride> materialOverride) {
    m_DrawCommandList.emplace_back(
        modelHandle, transform,
        std::vector<MaterialOverride>(materialOverride.begin(),
                                      materialOverride.end()));
}

void SYN::gfx::gl::Renderer::updateMsaaFramebuffer(Context &context) {
    if (!m_MsaaFramebuffer.update)
        return;

    m_MsaaFramebuffer.update = false;

    if (m_MsaaFramebuffer.rbAttachment) {
        context.deleteRenderbuffer(m_MsaaFramebuffer.rbAttachment.value());
    }
    if (m_MsaaFramebuffer.colorAttachment) {
        context.deleteTexture(m_MsaaFramebuffer.colorAttachment.value());
    }
    if (m_MsaaFramebuffer.handle) {
        context.deleteFramebuffer(m_MsaaFramebuffer.handle.value());
    }

    uint32_t samples = 1;
    switch (m_RenderConfig.aaMode) {
    case AntiAliasMode::None:
        break;
    case AntiAliasMode::FXAA:
        break;
    case AntiAliasMode::MSAA_2x:
        samples = 2;
        break;
    case AntiAliasMode::MSAA_4x:
        samples = 4;
        break;
    case AntiAliasMode::MSAA_8x:
        samples = 8;
        break;
    }

    m_MsaaFramebuffer.rbAttachment =
        context
            .createRenderbuffer({TextureFormat::Depth24Stencil8,
                                 m_ScreenViewport.width,
                                 m_ScreenViewport.height, samples})
            .value();

    m_MsaaFramebuffer.colorAttachment =
        context
            .createTexture({m_ScreenViewport.width, m_ScreenViewport.height,
                            TextureFormat::RGBA16F, 1, samples})
            .value();

    m_MsaaFramebuffer.handle =
        context
            .createFramebuffer(
                {std::array{
                     AttachmentDesc{m_MsaaFramebuffer.colorAttachment.value()}},
                 AttachmentDesc{m_MsaaFramebuffer.rbAttachment.value()}})
            .value();
}

void SYN::gfx::gl::Renderer::updateHdrFramebuffer(Context &context) {
    if (!m_HdrFramebuffer.update)
        return;

    m_HdrFramebuffer.update = false;

    if (m_HdrFramebuffer.colorAttachment) {
        context.deleteTexture(m_HdrFramebuffer.colorAttachment.value());
    }
    if (m_HdrFramebuffer.handle) {
        context.deleteFramebuffer(m_HdrFramebuffer.handle.value());
    }

    m_HdrFramebuffer.colorAttachment =
        context
            .createTexture({m_ScreenViewport.width, m_ScreenViewport.height,
                            TextureFormat::RGBA16F})
            .value();

    m_HdrFramebuffer.handle =
        context
            .createFramebuffer({
                std::array{
                    AttachmentDesc{m_HdrFramebuffer.colorAttachment.value()}},
            })
            .value();
}

void SYN::gfx::gl::Renderer::createScreenQuad(Context &context) {
    if (m_ScreenQuad.has_value()) {
        return;
    }

    float vertices[] = {-1.0, 1.0,  0.0, 1.0, 1.0, 1.0,  1.0, 1.0,
                        -1.0, -1.0, 0.0, 0.0, 1.0, -1.0, 1.0, 0.0};

    uint16_t indices[] = {0, 1, 2, 2, 3, 1};

    Handle<Buffer> vbo =
        context
            .createBuffer(
                {BufferType::Vertex, MemoryUsage::CpuToGPU, sizeof(vertices)},
                vertices)
            .value();

    Handle<Buffer> ebo =
        context
            .createBuffer(
                {BufferType::Index, MemoryUsage::CpuToGPU, sizeof(indices)},
                indices)
            .value();

    m_ScreenQuad =
        context
            .createVertexArray(
                {vbo,
                 sizeof(float) * 4,
                 ebo,
                 {VertexAttribDesc{0, VertexFormat::Float2, 0},
                  VertexAttribDesc{1, VertexFormat::Float2, sizeof(float) * 2}},
                 2,
                 IndexType::Unsigned16})
            .value();

    m_HdrFramebuffer.colorSampler = context.createSampler({}).value();
}

void SYN::gfx::gl::Renderer::createHdrShader(Context &context) {
    if (m_HdrShader.has_value())
        return;

    std::fstream vertexFile("resources/shaders/hdr.vert");
    if (!vertexFile) {
        spdlog::error("OpenGL renderer could not open vertex shader (HDR).");
        return;
    }
    std::fstream fragmentFile("resources/shaders/hdr.frag");
    if (!fragmentFile) {
        spdlog::error("OpenGL renderer could not open fragment shader (HDR).");
        return;
    }

    m_HdrShader =
        context
            .createShader(
                std::string(std::istreambuf_iterator<char>(vertexFile),
                            std::istreambuf_iterator<char>()),
                std::string(std::istreambuf_iterator<char>(fragmentFile),
                            std::istreambuf_iterator<char>()))
            .value();
}

void SYN::gfx::gl::Renderer::createSkybox(Context &context) {
    if (m_SkyboxShader.has_value())
        return;

    std::fstream vertexFile("resources/shaders/gl_skybox.vert");
    if (!vertexFile) {
        spdlog::error("OpenGL renderer could not open vertex shader (Skybox).");
        return;
    }
    std::fstream fragmentFile("resources/shaders/gl_skybox.frag");
    if (!fragmentFile) {
        spdlog::error(
            "OpenGL renderer could not open fragment shader (Skybox).");
        return;
    }

    std::string vertexSrc =
        std::string(std::istreambuf_iterator<char>(vertexFile),
                    std::istreambuf_iterator<char>());

    m_SkyboxShader =
        context
            .createShader(
                vertexSrc,
                std::string(std::istreambuf_iterator<char>(fragmentFile),
                            std::istreambuf_iterator<char>()))
            .value();

    if (!m_EquirectangularToCubemapShader.has_value()) {
        fragmentFile =
            std::fstream("resources/shaders/equirectangularToCubemap.frag");
        if (!fragmentFile) {
            spdlog::error("OpenGL renderer could not open fragment shader "
                          "(Equirectangular to Cubemap).");
            return;
        }

        m_EquirectangularToCubemapShader =
            context
                .createShader(
                    vertexSrc,
                    std::string(std::istreambuf_iterator<char>(fragmentFile),
                                std::istreambuf_iterator<char>()))
                .value();
    }

    if (m_SkyboxCube.has_value())
        return;

    // https://learnopengl.com/code_viewer.php?code=advanced/cubemaps_skybox_data
    float skyboxVertices[] = {
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
        -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

    Handle<Buffer> m_VertexBuffer =
        context
            .createBuffer({BufferType::Vertex, MemoryUsage::CpuToGPU,
                           sizeof(skyboxVertices)},
                          skyboxVertices)
            .value();

    m_SkyboxCube =
        context
            .createVertexArray({m_VertexBuffer,
                                sizeof(float) * 3,
                                {},
                                {VertexAttribDesc{0, VertexFormat::Float3, 0}},
                                1})
            .value();

    if (m_CubemapSampler.has_value())
        return;

    m_CubemapSampler = context
                           .createSampler({
                               SampleFilter::Linear,
                               SampleFilter::Linear,
                               WrapMode::ClampToEdge,
                               WrapMode::ClampToEdge,
                               WrapMode::ClampToEdge,
                           })
                           .value();

    m_MipmapCubeSampler = context
                              .createSampler({
                                  SampleFilter::Linear_Mipmap_Linear,
                                  SampleFilter::Linear,
                                  WrapMode::ClampToEdge,
                                  WrapMode::ClampToEdge,
                                  WrapMode::ClampToEdge,
                              })
                              .value();
}

SYN::gfx::gl::Handle<SYN::gfx::gl::Texture>
SYN::gfx::gl::Renderer::createIrradianceMap(Context &context,
                                            Handle<Texture> hdrMap) {
    // https://learnopengl.com/code_viewer_gh.php?code=src/6.pbr/2.1.1.ibl_irradiance_conversion/ibl_irradiance_conversion.cpp
    glm::mat4 captureProjection =
        glm::perspectiveRH_NO(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                      glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                      glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                      glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                      glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                      glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                      glm::vec3(0.0f, -1.0f, 0.0f))};

    constexpr uint32_t mapResolution = 32;
    Handle<Texture> irradianceMap =
        context
            .createTexture({mapResolution, mapResolution,
                            TextureFormat::RGBA16F, 1, 1, TextureType::Cubemap})
            .value();
    Handle<Framebuffer> captureFramebuffer =
        context.createFramebuffer({std::array{AttachmentDesc{irradianceMap}}})
            .value();

    Handle<Sampler> mipSampler =
        context
            .createSampler({SampleFilter::Linear_Mipmap_Linear,
                            SampleFilter::Linear, WrapMode::ClampToEdge,
                            WrapMode::ClampToEdge, WrapMode::ClampToEdge})
            .value();

    PipelineState pipeline;
    pipeline.shader = m_IrradianceShader.value();
    for (uint32_t i = 0; i < 6; ++i) {
        context.setColorAttachment(captureFramebuffer, 0, irradianceMap, 0, i);
        Pass pass =
            context.beginPass({captureFramebuffer, glm::vec4(1.0), false, false,
                               Viewport{0, 0, mapResolution, mapResolution}});
        pass.usePipeline(pipeline);
        pass.bindTexture(0, hdrMap, mipSampler);
        pass.bindUniform("u_hdrMap", 0);
        pass.bindUniform("u_ViewProjection",
                         captureProjection * captureViews[i]);
        pass.bindVertexArray(m_SkyboxCube.value());
        pass.draw(36);
    }

    context.deleteFramebuffer(captureFramebuffer);
    context.deleteSampler(mipSampler);

    return irradianceMap;
}

SYN::gfx::gl::Handle<SYN::gfx::gl::Texture>
SYN::gfx::gl::Renderer::createPrefilteredEnvironmentMap(
    Context &context, Handle<Texture> hdrMap) {
    // https://learnopengl.com/code_viewer_gh.php?code=src/6.pbr/2.1.1.ibl_irradiance_conversion/ibl_irradiance_conversion.cpp
    glm::mat4 captureProjection =
        glm::perspectiveRH_NO(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                      glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                      glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                      glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                      glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                      glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                      glm::vec3(0.0f, -1.0f, 0.0f))};

    uint32_t maxMipCount = 5;

    constexpr uint32_t mapResolution = 128;
    Handle<Texture> prefilteredMap =
        context
            .createTexture({mapResolution, mapResolution,
                            TextureFormat::RGBA16F, maxMipCount, 1,
                            TextureType::Cubemap})
            .value();

    Handle<Framebuffer> captureFramebuffer =
        context.createFramebuffer({std::array{AttachmentDesc{prefilteredMap}}})
            .value();

    Handle<Sampler> mipSampler =
        context
            .createSampler({SampleFilter::Linear_Mipmap_Linear,
                            SampleFilter::Linear, WrapMode::ClampToEdge,
                            WrapMode::ClampToEdge, WrapMode::ClampToEdge})
            .value();

    PipelineState pipeline;
    pipeline.shader = m_PrefilterShader.value();

    for (uint32_t i = 0; i < maxMipCount; ++i) {
        uint32_t mipSize =
            (uint32_t)((float)mapResolution * std::pow(0.5f, (float)i));
        for (int j = 0; j < 6; ++j) {
            context.setColorAttachment(captureFramebuffer, 0, prefilteredMap, i,
                                       j);
            Pass pass =
                context.beginPass({captureFramebuffer, glm::vec4(1.0), false,
                                   false, Viewport{0, 0, mipSize, mipSize}});

            pass.usePipeline(pipeline);
            pass.bindTexture(0, hdrMap, mipSampler);
            pass.bindUniform("u_hdrMap", 0);
            pass.bindUniform("u_ViewProjection",
                             captureProjection * captureViews[j]);
            float roughness = (float)i / (float)(maxMipCount - 1);
            pass.bindUniform("u_roughness", roughness);
            pass.bindVertexArray(m_SkyboxCube.value());
            pass.draw(36);
        }
    }

    context.deleteFramebuffer(captureFramebuffer);
    context.deleteSampler(mipSampler);

    return prefilteredMap;
}

SYN::gfx::gl::Handle<SYN::gfx::gl::Texture>
SYN::gfx::gl::Renderer::createBRDFLut(Context &context) {
    Handle<Texture> brdfLUT =
        context.createTexture({512, 512, TextureFormat::RG16F}).value();

    Handle<Framebuffer> captureFb =
        context.createFramebuffer({std::array{AttachmentDesc{brdfLUT}}})
            .value();

    PipelineState pipeline;
    pipeline.shader = m_BRDFLutShader.value();

    Pass pass = context.beginPass(
        {captureFb, glm::vec4(1.0f), false, false, Viewport{0, 0, 512, 512}});
    pass.usePipeline(pipeline);
    pass.bindVertexArray(m_ScreenQuad.value());
    pass.drawIndexed(6);

    context.deleteFramebuffer(captureFb);

    return brdfLUT;
}

void SYN::gfx::gl::Renderer::endFrame(Context &context) {
    glm::mat4 viewMatrix = glm::lookAtRH(m_MainCamera.position,
                                         m_MainCamera.target, m_MainCamera.up);

    glm::mat4 projMatrix = glm::perspectiveRH_NO(
        glm::radians(m_MainCamera.fovYDegrees),
        (float)m_ScreenViewport.width / m_ScreenViewport.height,
        m_MainCamera.nearPlane, m_MainCamera.farPlane);

    updateMsaaFramebuffer(context);
    updateHdrFramebuffer(context);

    // Main render pass
    {
        Pass pass = context.beginPass({m_MsaaFramebuffer.handle, m_ClearColor,
                                       true, false, m_ScreenViewport});
        PipelineState pipeline;

        for (const DrawCommand &cmd : m_DrawCommandList) {
            Model model = m_ModelRegistry.getResource(cmd.modelHandle).value();

            std::vector<Mesh> meshes = model.meshes;

            for (const MaterialOverride &override : cmd.materialOverride) {
                Mesh &mesh = meshes.at(override.meshIndex);
                mesh.material = override.material;
                if (!mesh.material.albedo) {
                    mesh.material.albedo = m_DefaultWhite;
                }
                mesh.shader = m_ShaderCache.getShaderHandle(
                    context,
                    getShaderFeatures(
                        mesh.material.normalMap.has_value(),
                        mesh.material.metallicRoughnessMap.has_value(), false));
            }

            for (const Mesh &mesh : meshes) {
                pipeline.shader = mesh.shader;

                const Material &material = mesh.material;

                pass.usePipeline(pipeline);

                pass.bindUniform("u_tint", material.tint.r, material.tint.g,
                                 material.tint.b);
                pass.bindUniform("u_metallic", material.metallic);
                pass.bindUniform("u_roughness", material.roughness);
                bindDirectionalLight(pass, m_DirectionalLight);

                Handle<Sampler> sampler = material.sampler.has_value()
                                              ? material.sampler.value()
                                              : m_DefaultSampler.value();

                if (material.albedo) {
                    pass.bindTexture(0, material.albedo.value(), sampler);
                    pass.bindUniform("u_albedoTexture", 0);
                }

                if (material.normalMap) {
                    pass.bindTexture(1, material.normalMap.value(), sampler);
                    pass.bindUniform("u_normalMap", 1);
                }

                if (material.metallicRoughnessMap) {
                    pass.bindTexture(2, material.metallicRoughnessMap.value(),
                                     sampler);
                    pass.bindUniform("u_metallicRoughness", 2);
                }

                if (m_Environment.has_value() &&
                    m_Environment.value().irradianceMap.has_value()) {
                    pass.bindTexture(
                        3, m_Environment.value().irradianceMap.value(),
                        m_CubemapSampler.value());
                    pass.bindUniform("u_irradianceMap", 3);
                } else {
                    pass.bindTexture(3, m_DefaultIrradianceMap,
                                     m_CubemapSampler.value());
                    pass.bindUniform("u_irradianceMap", 3);
                }

                if (m_Environment.has_value() &&
                    m_Environment.value().prefilteredMap.has_value()) {
                    pass.bindTexture(
                        4, m_Environment.value().prefilteredMap.value(),
                        m_MipmapCubeSampler.value());
                    pass.bindUniform("u_prefilterMap", 4);
                } else {
                    pass.bindTexture(4, m_DefaultPrefilterMap,
                                     m_MipmapCubeSampler.value());
                    pass.bindUniform("u_prefilterMap", 4);
                }

                pass.bindTexture(5, m_BRDFLut, m_CubemapSampler.value());
                pass.bindUniform("u_brdfLUT", 5);

                pass.bindUniform("u_cameraPos", m_MainCamera.position.x,
                                 m_MainCamera.position.y,
                                 m_MainCamera.position.z);
                pass.bindUniform("u_ViewProjection", projMatrix * viewMatrix);
                pass.bindUniform("u_Model",
                                 cmd.transform * mesh.localTransform);
                pass.bindVertexArray(mesh.vao);
                pass.drawIndexed(mesh.indexCount);
            }
        }

        if (m_Environment.has_value()) {
            glDepthFunc(GL_LEQUAL);
            Environment currentEnvironment = m_Environment.value();
            pipeline.shader = m_SkyboxShader.value();
            pass.usePipeline(pipeline);

            pass.bindTexture(0, currentEnvironment.cubemap,
                             m_CubemapSampler.value());
            pass.bindUniform("u_skybox", 0);
            pass.bindUniform("u_ViewProjection",
                             projMatrix * glm::mat4(glm::mat3(viewMatrix)));
            pass.bindVertexArray(m_SkyboxCube.value());
            pass.draw(36);
        }
        glDepthFunc(GL_LESS);
    }

    context.blitFramebuffer(m_MsaaFramebuffer.handle, m_HdrFramebuffer.handle,
                            m_ScreenViewport, m_ScreenViewport);

    {
        Pass hdrPass = context.beginPass(
            {std::nullopt, m_ClearColor, false, false, m_ScreenViewport});
        PipelineState hdrPipeline;
        hdrPipeline.shader = m_HdrShader.value();

        hdrPass.usePipeline(hdrPipeline);

        hdrPass.bindTexture(0, m_HdrFramebuffer.colorAttachment.value(),
                            m_HdrFramebuffer.colorSampler);
        hdrPass.bindUniform("u_hdrBuffer", 0);
        hdrPass.bindUniform("u_gamma", m_Gamma);
        hdrPass.bindUniform("u_exposure", m_Exposure);

        hdrPass.bindVertexArray(m_ScreenQuad.value());
        hdrPass.drawIndexed(6);
    }

    m_DrawCommandList.clear();
}

void SYN::gfx::gl::Renderer::setGamma(float gamma) {
    m_Gamma = glm::clamp(gamma, MIN_GAMMA, MAX_GAMMA);
}
void SYN::gfx::gl::Renderer::setExposure(float exposure) {
    if (exposure < 0.0f)
        exposure = 0.0f;
    m_Exposure = exposure;
}

void SYN::gfx::gl::Renderer::setBloomEnabled(bool enabled) {}

void SYN::gfx::gl::Renderer::setAntiAliasMode(AntiAliasMode mode) {
    if (m_RenderConfig.aaMode == mode)
        return;

    // MSAA to non-MSAA needs update
    if (m_RenderConfig.aaMode >= AntiAliasMode::MSAA_2x &&
        m_RenderConfig.aaMode <= AntiAliasMode::MSAA_8x) {
        m_MsaaFramebuffer.update = true;
    }

    m_RenderConfig.aaMode = mode;

    // non-MSAA to MSAA needs update
    if (m_RenderConfig.aaMode >= AntiAliasMode::MSAA_2x &&
        m_RenderConfig.aaMode <= AntiAliasMode::MSAA_8x) {
        m_MsaaFramebuffer.update = true;
    }
}

void SYN::gfx::gl::Renderer::resize(int width, int height) {
    if (width == m_ScreenViewport.width && height == m_ScreenViewport.height)
        return;
    m_ScreenViewport.width = width;
    m_ScreenViewport.height = height;
    m_MsaaFramebuffer.update = true;
    m_HdrFramebuffer.update = true;
}

void SYN::gfx::gl::Renderer::setClearColor(const glm::vec4 &clearColor) {
    m_ClearColor = clearColor;
}

void SYN::gfx::gl::Renderer::bindDirectionalLight(
    Pass &pass, const DirectionalLight &light) {
    pass.bindUniform("u_light.direction", light.direction.x, light.direction.y,
                     light.direction.z);
    pass.bindUniform("u_light.color", light.color.r, light.color.g,
                     light.color.b);
    pass.bindUniform("u_light.intensity", light.intensity);
    pass.bindUniform("u_light.castShadow", light.castsShadows);
}

SYN::gfx::gl::ShaderCache::ShaderCache() {
    std::fstream vertexFile("resources/shaders/forward.vert");
    if (!vertexFile) {
        spdlog::error(
            "OpenGL renderer could not open vertex shader. (FORWARD)");
        return;
    }
    std::fstream fragmentFile("resources/shaders/forward.frag");
    if (!fragmentFile) {
        spdlog::error(
            "OpenGL renderer could not open fragment shader. (FORWARD)");
        return;
    }
    m_VertexSource = std::string(std::istreambuf_iterator<char>(vertexFile),
                                 std::istreambuf_iterator<char>());
    m_FragmentSource = std::string(std::istreambuf_iterator<char>(fragmentFile),
                                   std::istreambuf_iterator<char>());
}

SYN::gfx::gl::Handle<SYN::gfx::gl::Shader>
SYN::gfx::gl::ShaderCache::getShaderHandle(Context &context,
                                           uint32_t featureFlags) {
    std::string versionString = "#version 450 core\n";
    if (m_ShaderCache.find(featureFlags) != m_ShaderCache.cend()) {
        return m_ShaderCache.at(featureFlags);
    }
    if (featureFlags & (uint32_t)ShaderFeatures::Normal) {
        versionString += "#define FEATURE_NORMAL\n";
    }
    if (featureFlags & (uint32_t)ShaderFeatures::MetallicRoughness) {
        versionString += "#define FEATURE_METALLIC_ROUGHNESS\n";
    }
    if (featureFlags & (uint32_t)ShaderFeatures::Skinned) {
        versionString += "#define FEATURE_SKINNED\n";
    }

    std::string fullVertexSrc = versionString + m_VertexSource;
    std::string fullFragmentSrc = versionString + m_FragmentSource;
    m_ShaderCache[featureFlags] =
        context.createShader(fullVertexSrc, fullFragmentSrc).value();
    return m_ShaderCache.at(featureFlags);
}
