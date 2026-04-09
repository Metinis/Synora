#include <PuzzleEngine/gfx/gl/gl.h>

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <spdlog/spdlog.h>

SYN::gfx::gl::Context::Context(const ContextInitDesc &desc) {
    m_Window = desc.windowHandle;
}

SYN::gfx::gl::Context::~Context() {}

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

    return Context(desc);
}

SYN::gfx::gl::Pass SYN::gfx::gl::Context::beginPass(const PassDesc &desc) {
    return Pass(this, desc);
}

SYN::gfx::gl::Pass::Pass(Context *context, const PassDesc &desc) {
    assert(context != nullptr && "OpenGL pass context cannot be NULL!");

    m_ContextPtr = context;

    if (desc.framebuffer.has_value()) {
        glBindFramebuffer(GL_FRAMEBUFFER, desc.framebuffer.value().id);
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
        glClear(clearMask);
        if (desc.clearColor.has_value()) {
            glm::vec4 clearColor = desc.clearColor.value();
            glClearColor(clearColor.r, clearColor.g, clearColor.b,
                         clearColor.a);
        }
    }
}

std::optional<SYN::gfx::gl::Handle<SYN::gfx::gl::Buffer>>
SYN::gfx::gl::Context::createBuffer(const BufferDesc &desc,
                                    const void *initialData) {
    GLenum usage = GL_STATIC_DRAW;
    if (desc.usage == BufferUsage::Dynamic) {
        usage = GL_DYNAMIC_DRAW;
    }

    Buffer buffer;
    buffer.type = desc.bufferType;
    glCreateBuffers(1, &buffer.id);
    glNamedBufferData(buffer.id, desc.size, initialData, usage);

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
        };

        glVertexArrayAttribFormat(vaoId, attribute.location, size, GL_FLOAT,
                                  attribute.normalized, attribute.offset);
    }

    glVertexArrayVertexBuffer(vaoId, 0, vertexBuffer.value().id, 0,
                              desc.vertexBufferStride);
    std::optional<Buffer> indexBuffer =
        m_BufferRegistry.getResource(desc.indexBufferHandle);
    if (indexBuffer.has_value()) {
        glVertexArrayElementBuffer(vaoId, indexBuffer.value().id);
    }

    return m_VertexArrayRegistry.createHandle(VertexArray{vaoId});
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

    m_PendingDeleteShaders.clear();
    m_PendingDeletePrograms.clear();
    m_PendingDeleteVertexArrays.clear();
    m_PendingDeleteBuffers.clear();
}

SYN::gfx::gl::Pass::~Pass() {
    glUseProgram(0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
