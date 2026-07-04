#include <SynoraEngine/core/Application.h>
#include <SynoraEngine/core/Window.h>
#include <SynoraEngine/gfx/gl/GL.h>
#include <SynoraEngine/scene/Scene.h>
#include <spdlog/spdlog.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/glm.hpp>

#include <imgui.h>

#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

std::string vertexSource = R"(
    #version 450 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoord;

    out vec3 fragPos;
    out vec3 fragNormal;
    out vec2 fragTexCoord;

    uniform mat4 u_Model;
    uniform mat4 u_View;
    uniform mat4 u_Proj;

    void main() {
        fragPos = vec3(u_Model * vec4(aPos, 1.0));
        fragNormal = normalize(mat3(transpose(inverse(u_Model))) * aNormal);
        fragTexCoord = aTexCoord;
        gl_Position = u_Proj * u_View * u_Model * vec4(aPos, 1.0);
    }
)";

std::string fragmentSource = R"(
    #version 450 core
    out vec4 fragColor;

    in vec3 fragPos;
    in vec3 fragNormal;
    in vec2 fragTexCoord;

    uniform vec4 u_baseColor;
    uniform sampler2D u_texture0;

    uniform vec3 u_cameraPos;

    void main() {
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0)); 

        float ambientStrength = 0.3f;
        vec3 ambient = vec3(ambientStrength);

        float diff = max(dot(lightDir, fragNormal), 0.0);
        vec3 diffuse = vec3(diff);

        vec3 cameraDir = normalize(u_cameraPos - fragPos);
        vec3 reflDir = reflect(-lightDir, fragNormal);

        float spec = pow(max(dot(cameraDir, reflDir), 0.0), 64);
        vec3 specular = vec3(spec) * 0.5;

        vec4 texColor = texture(u_texture0, fragTexCoord);
        vec3 objectColorLit = u_baseColor.rgb * (ambient + diffuse + specular);
        fragColor = texColor * vec4(objectColorLit, 1.0);
    }
)";

std::string framebufferVertexSource = R"(
   #version 450 core
   layout (location = 0) in vec3 aPos;
   layout (location = 1) in vec2 aTexCoords;
   out vec2 fragTexCoords;
   void main() {
    fragTexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
   }
)";

std::string framebufferFragmentSource = R"(
   #version 450 core
   out vec4 fragColor;
   in vec2 fragTexCoords;
   uniform sampler2D texture0;
   #define INVERT 0
   #define GRAYSCALE 1
   #define SHARPEN 2
   uniform int mode;
   void main() {
    vec3 textureColor = texture(texture0, fragTexCoords).rgb;
    vec3 finalColor = textureColor;
    if (mode == GRAYSCALE) {
        finalColor = vec3(0.2126 * finalColor.r + 0.7152 * finalColor.g + 0.0722 * finalColor.b);
    } else if (mode == SHARPEN) {
        const float offset = 1.0 / 300.0;
        vec2 offsets[9] = vec2[](
            vec2(-offset,  offset), // top-left
            vec2( 0.0f,    offset), // top-center
            vec2( offset,  offset), // top-right
            vec2(-offset,  0.0f),   // center-left
            vec2( 0.0f,    0.0f),   // center-center
            vec2( offset,  0.0f),   // center-right
            vec2(-offset, -offset), // bottom-left
            vec2( 0.0f,   -offset), // bottom-center
            vec2( offset, -offset)  // bottom-right    
        );

        float kernel[9] = float[](
            -1, -1, -1,
            -1,  9, -1,
            -1, -1, -1
        );
        vec3 sampleTex[9];
        for(int i = 0; i < 9; i++) {
            sampleTex[i] = vec3(texture(texture0, fragTexCoords + offsets[i]));
        }
        finalColor = vec3(0.0);
        for(int i = 0; i < 9; i++)
            finalColor += sampleTex[i] * kernel[i];
    } else {
        finalColor = 1.0 - finalColor;
    }
    fragColor = vec4(finalColor, 1.0);
   }
)";

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Texture {
    std::vector<char> data;
    size_t width;
    size_t height;
    size_t channelCount;
    std::string texturePath;
};

struct Material {
    glm::vec4 albedoColor = glm::vec4(1.0);
    std::optional<Texture> albedoTexture = std::nullopt;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::mat4 localTransform = glm::mat4(1.0);
    uint32_t materialIndex;
};

struct Model {
    std::string dir;
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
};

Mesh processMesh(aiMesh *mesh, const aiScene *scene,
                 const glm::mat4 &transform) {
    Mesh newMesh;
    newMesh.localTransform = transform;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y,
                           mesh->mVertices[i].z};

        vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y,
                         mesh->mNormals[i].z};

        if (mesh->HasTextureCoords(0)) {
            vertex.texCoord = {mesh->mTextureCoords[0][i].x,
                               mesh->mTextureCoords[0][i].y};
        } else {
            vertex.texCoord = {0.0f, 0.0f};
        }

        newMesh.vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            newMesh.indices.push_back(face.mIndices[j]);
        }
    }
    newMesh.materialIndex = mesh->mMaterialIndex;
    return newMesh;
}

void loadMaterial(aiMaterial *mat, const aiScene *scene, Model &model) {
    Material newMaterial;
    aiColor4D color;
    if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
        newMaterial.albedoColor = {color.r, color.g, color.b, color.a};
    } else {
        newMaterial.albedoColor = {1.0f, 1.0f, 1.0f, 1.0f};
    }

    if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &str);
        if (str.length > 0) {
            const aiTexture *texture = scene->GetEmbeddedTexture(str.C_Str());
            Texture albedoTexture;
            albedoTexture.texturePath = str.C_Str();
            if (texture) {
                int width, height, channels;
                // Load with original channel count first to detect actual
                // format
                unsigned char *data = stbi_load_from_memory(
                    (unsigned char *)texture->pcData, texture->mWidth, &width,
                    &height, &channels, 0); // 0 = keep original channels
                if (data) {
                    albedoTexture.data.assign(data,
                                              data + width * height * channels);
                    stbi_image_free(data);
                    albedoTexture.width = width;
                    albedoTexture.height = height;
                    albedoTexture.channelCount = channels;
                    newMaterial.albedoTexture = albedoTexture;
                } else {
                    spdlog::error("Unable to load {}!",
                                  texture->mFilename.C_Str());
                }
            } else {
                int w, h, nrChannels;
                std::string fullpath = model.dir + std::string(str.C_Str());
                // Load with original channel count first
                unsigned char *data =
                    stbi_load(fullpath.c_str(), &w, &h, &nrChannels,
                              0); // 0 = keep original channels
                if (data) {
                    albedoTexture.data.assign(data, data + w * h * nrChannels);
                    albedoTexture.width = w;
                    albedoTexture.height = h;
                    albedoTexture.channelCount = nrChannels;
                    newMaterial.albedoTexture = albedoTexture;
                } else {
                    spdlog::error("Unable to load {}!", fullpath);
                }
            }
        }
    }
    model.materials.push_back(newMaterial);
}

void processNode(aiNode *currentNode, const aiScene *scene, Model &model,
                 const glm::mat4 &parentTransform) {
    glm::mat4 nodeTransform =
        parentTransform *
        glm::transpose(*(glm::mat4 *)&currentNode->mTransformation);
    for (unsigned int i = 0; i < currentNode->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[currentNode->mMeshes[i]];
        model.meshes.push_back(processMesh(mesh, scene, nodeTransform));
    }

    for (unsigned int i = 0; i < currentNode->mNumChildren; i++) {
        processNode(currentNode->mChildren[i], scene, model, nodeTransform);
    }
}

std::optional<Model> loadModel(const std::string &path) {
    Assimp::Importer importer;
    const aiScene *scene{importer.ReadFile(path, aiProcess_Triangulate |
                                                     aiProcess_FlipUVs |
                                                     aiProcess_GenNormals)};

    if (scene == nullptr) {
        spdlog::warn("Could not load {}: {}", path.c_str(),
                     importer.GetErrorString());
        return std::nullopt;
    }

    Model model;
    model.dir = path.substr(0, path.find_last_of("/") + 1);
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        loadMaterial(scene->mMaterials[i], scene, model);
    }
    processNode(scene->mRootNode, scene, model, glm::mat4(1.0f));
    return model;
}

class GraphicsScene : public SYN::ILayer {
  public:
    void init(SYN::EngineContext *engineContext) {
        m_Context = &engineContext->glContext.value();
        m_WindowHandle = engineContext->window->getHandle();
        m_Model = loadModel("resources/assets/waltuh.glb").value();
        for (const Mesh &mesh : m_Model.meshes) {
            SYN::gfx::gl::Handle<SYN::gfx::gl::Buffer> modelVertexBuf =
                m_Context
                    ->createBuffer(
                        {SYN::gfx::gl::BufferType::Vertex,
                         SYN::gfx::gl::MemoryUsage::CpuToGPU,
                         (uint32_t)(mesh.vertices.size() * sizeof(Vertex))},
                        &mesh.vertices[0])
                    .value();

            SYN::gfx::gl::Handle<SYN::gfx::gl::Buffer> modelIndexBuf =
                m_Context
                    ->createBuffer(
                        {SYN::gfx::gl::BufferType::Index,
                         SYN::gfx::gl::MemoryUsage::CpuToGPU,
                         (uint32_t)(mesh.indices.size() * sizeof(uint32_t))},
                        &mesh.indices[0])
                    .value();

            m_VertexArrays.push_back(
                m_Context
                    ->createVertexArray(
                        {modelVertexBuf,
                         sizeof(Vertex),
                         modelIndexBuf,
                         {
                             {{0, SYN::gfx::gl::VertexFormat::Float3,
                               offsetof(Vertex, position)},
                              {1, SYN::gfx::gl::VertexFormat::Float3,
                               offsetof(Vertex, normal)},
                              {2, SYN::gfx::gl::VertexFormat::Float2,
                               offsetof(Vertex, texCoord)}},
                         },
                         3})
                    .value());
        }

        for (const Material &material : m_Model.materials) {
            if (!material.albedoTexture.has_value())
                continue;
            const Texture &texture = material.albedoTexture.value();
            const std::string &pathKey = texture.texturePath;
            if (m_Textures.find(pathKey) != m_Textures.cend()) {
                continue;
            }

            // Use appropriate format based on actual channel count
            SYN::gfx::gl::TextureFormat format;
            switch (texture.channelCount) {
            case 1:
                format = SYN::gfx::gl::TextureFormat::R8;
                break;
            case 2:
                format = SYN::gfx::gl::TextureFormat::RG8;
                break;
            case 3:
                format = SYN::gfx::gl::TextureFormat::RGB8;
                break;
            case 4:
            default:
                format = SYN::gfx::gl::TextureFormat::RGBA8;
                break;
            }

            m_Textures[pathKey] =
                m_Context
                    ->createTexture({(uint32_t)texture.width,
                                     (uint32_t)texture.height, format},
                                    &texture.data[0])
                    .value();
        }
        m_Sampler = m_Context->createSampler({}).value();

        m_Renderbuffer = m_Context->createRenderbuffer({
            SYN::gfx::gl::TextureFormat::Depth24Stencil8,
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
        }).value();

        m_PostProcessTexture = m_Context->createTexture({
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SYN::gfx::gl::TextureFormat::RGB8
        }).value();

        m_Framebuffer = m_Context->createFramebuffer({
                std::array{SYN::gfx::gl::AttachmentDesc{m_PostProcessTexture}},
                SYN::gfx::gl::AttachmentDesc{m_Renderbuffer}
        }).value();

        struct Vertex {
            glm::vec3 position;
            glm::vec2 texCoord;
        };

        Vertex vertices[] = {
            {{1.0f, 1.0f, 0.0f},  {1.0f, 1.0f}},
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
            {{1.0f, -1.0f, 0.0f},  {1.0f, 0.0f}},
            {{-1.0f, 1.0f, 0.0f},  {0.0f, 1.0f}}};

        unsigned int indices[] = {3, 0, 2, 3, 1, 2};

        SYN::gfx::gl::Handle<SYN::gfx::gl::Buffer> vertexBuffer =
            m_Context
                ->createBuffer({SYN::gfx::gl::BufferType::Vertex, SYN::gfx::gl::MemoryUsage::CpuToGPU,
                            sizeof(vertices)},
                            vertices)
                .value();

        SYN::gfx::gl::Handle<SYN::gfx::gl::Buffer> indexBuffer =
            m_Context
                ->createBuffer({SYN::gfx::gl::BufferType::Index, SYN::gfx::gl::MemoryUsage::GpuOnly,
                            sizeof(indices)},
                            indices)
                .value();

        SYN::gfx::gl::VertexArrayDesc basicVertexDesc;
        basicVertexDesc.indexBufferHandle = indexBuffer;
        basicVertexDesc.vertexBufferHandle = vertexBuffer;
        basicVertexDesc.vertexBufferStride = sizeof(Vertex);
        basicVertexDesc.attributes[0] = {0, SYN::gfx::gl::VertexFormat::Float3,
                                        offsetof(Vertex, position), false};
        basicVertexDesc.attributes[1] = {1, SYN::gfx::gl::VertexFormat::Float2,
                                        offsetof(Vertex, texCoord), false};
        basicVertexDesc.attributeCount = 2;

        m_ScreenQuad =
            m_Context->createVertexArray(basicVertexDesc).value();
        m_Shader =
            m_Context->createShader(vertexSource, fragmentSource).value();
        m_PostProcessShader = 
            m_Context->createShader(framebufferVertexSource, framebufferFragmentSource).value();
    }
    void onUIRender() override { 
        if (ImGui::Begin("Post Process")) {
            const char *effects[] = { "Invert", "Grayscale", "Sharpen" };
            ImGui::Combo("Current Effect", &m_PostProcessMode, effects, IM_ARRAYSIZE(effects));
            ImGui::End();
        }
    }
    void onRender() override {
        SYN::gfx::gl::Viewport vp;
        int w, h;
        glfwGetWindowSize(m_WindowHandle, &w, &h);
        {
            SYN::gfx::gl::PipelineState pipeline;
            pipeline.shader = m_Shader;

            vp.width = WINDOW_WIDTH;
            vp.height = WINDOW_HEIGHT;

            SYN::gfx::gl::Pass pass = m_Context->beginPass(
                {m_Framebuffer, glm::vec4(0.2, 0.2, 0.53, 1.0), true, false, vp,
                 std::nullopt});

            glm::mat4 model(1.0);
            glm::mat4 view(1.0);
            glm::mat4 projection(1.0);

            model = glm::scale(model, glm::vec3(0.3));

            double time = glfwGetTime();
            glm::vec3 cameraPos =
                glm::vec3(cosf(time) * 0.5, 0.45, sinf(time) * 0.5f);

            view = glm::lookAt(cameraPos, glm::vec3(0.0),
                               glm::vec3(0.0, 1.0, 0.0));

            projection =
                glm::perspective(90.0f, (float)w / (float)h, 0.01f, 100.0f);

            pass.usePipeline(pipeline);
            for (int i = 0; i < m_VertexArrays.size(); ++i) {
                const Mesh &mesh = m_Model.meshes[i];
                const Material &material =
                    m_Model.materials[mesh.materialIndex];
                if (material.albedoTexture.has_value()) {
                    const Texture &texture = material.albedoTexture.value();
                    pass.bindTexture(0, m_Textures.at(texture.texturePath),
                                     m_Sampler);
                    pass.bindUniform("u_texture0", 0);
                }
                pass.bindUniform("u_baseColor", material.albedoColor.r,
                                 material.albedoColor.g, material.albedoColor.b,
                                 material.albedoColor.a);

                pass.bindUniform("u_Model", model * mesh.localTransform);
                pass.bindUniform("u_View", view);
                pass.bindUniform("u_Proj", projection);
                pass.bindUniform("u_cameraPos", cameraPos.x, cameraPos.y,
                                 cameraPos.z);

                pass.bindVertexArray(m_VertexArrays[i]);
                pass.drawIndexed(mesh.indices.size());
            }
        }
        vp.width = w;
        vp.height = h;
        {
            SYN::gfx::gl::PipelineState pipeline;
            pipeline.shader = m_PostProcessShader;
            SYN::gfx::gl::Pass pass = m_Context->beginPass(
                {std::nullopt, glm::vec4(0.0, 0.0, 0.0, 1.0), false, false, vp,
                 std::nullopt});
            pass.usePipeline(pipeline);
            pass.bindTexture(0, m_PostProcessTexture, m_Sampler);
            pass.bindUniform("texture0", 0);
            pass.bindUniform("mode", m_PostProcessMode);
            pass.bindVertexArray(m_ScreenQuad);
            pass.drawIndexed(6);
        }
    }
    void onAttach() override {};
    void onDettach() override {};
    void onUpdate(float dt) override {};

  private:
    SYN::Scene *m_Scene;
    SYN::gfx::gl::Context *m_Context;
    GLFWwindow *m_WindowHandle;
    Model m_Model;
    int m_PostProcessMode = 0;
    SYN::gfx::gl::Handle<SYN::gfx::gl::Shader> m_Shader;
    SYN::gfx::gl::Handle<SYN::gfx::gl::Shader> m_PostProcessShader;
    SYN::gfx::gl::Handle<SYN::gfx::gl::Renderbuffer> m_Renderbuffer;
    SYN::gfx::gl::Handle<SYN::gfx::gl::Texture> m_PostProcessTexture;
    SYN::gfx::gl::Handle<SYN::gfx::gl::Framebuffer> m_Framebuffer;
    SYN::gfx::gl::Handle<SYN::gfx::gl::VertexArray> m_ScreenQuad;
    std::vector<SYN::gfx::gl::Handle<SYN::gfx::gl::VertexArray>> m_VertexArrays;
    SYN::gfx::gl::Handle<SYN::gfx::gl::Sampler> m_Sampler;
    std::unordered_map<std::string, SYN::gfx::gl::Handle<SYN::gfx::gl::Texture>>
        m_Textures;
};

class GLTest2 : public SYN::Application {
  public:
    GLTest2() { m_Graphics = std::make_unique<GraphicsScene>(); }

    void init() override {
        m_EngineContext.windowConfig = SYN::WindowConfig{
            "GLTest2", WINDOW_WIDTH, WINDOW_HEIGHT, SYN::OpenGLConfig{4, 5}};

        SYN::Application::init();
        m_Graphics->init(&m_EngineContext);
        m_Layers.push_front(m_Graphics.get());
    }
    ~GLTest2() override {}

  private:
    std::unique_ptr<GraphicsScene> m_Graphics;
};

int main(void) {
    std::unique_ptr<SYN::Application> app = std::make_unique<GLTest2>();

    app->init();

    app->run();
    app->shutdown();
}
