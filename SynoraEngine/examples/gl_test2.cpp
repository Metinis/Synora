#include <SynoraEngine/core/Application.h>
#include <SynoraEngine/core/Window.h>
#include <SynoraEngine/gfx/gl/gl.h>
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

    out vec3 fragNormal;
    out vec2 fragTexCoord;

    uniform mat4 u_MVP;

    void main() {
        fragNormal = aNormal;
        fragTexCoord = aTexCoord;
        gl_Position = u_MVP * vec4(aPos, 1.0);
    }
)";

std::string fragmentSource = R"(
    #version 450 core
    out vec4 fragColor;

    in vec3 fragNormal;
    in vec2 fragTexCoord;

    void main() {
        fragColor = vec4(abs(fragNormal), 1.0);
    }
)";

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::mat4 localTransform;
};

struct Model {
    std::vector<Mesh> meshes;
};

Mesh processMesh(aiMesh *mesh, const aiScene *scene,
                 const glm::mat4 &transform) {
    Mesh newMesh;
    newMesh.localTransform = transform;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y,
                           mesh->mVertices[i].z};

        if (mesh->HasNormals()) {
            vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y,
                             mesh->mNormals[i].z};
        }

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
    return newMesh;
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
    const aiScene *scene{importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices)};

    if (scene == nullptr) {
        spdlog::warn("Could not load {}: {}", path.c_str(),
                     importer.GetErrorString());
        return std::nullopt;
    }

    Model model;
    processNode(scene->mRootNode, scene, model, glm::mat4(1.0f));
    return model;
}

class GraphicsScene : public SYN::ILayer {
  public:
    void init(SYN::EngineContext *engineContext) {
        m_Context = &engineContext->glContext.value();
        m_WindowHandle = engineContext->window->getHandle();
        m_Model = loadModel("resources/assets/Cabin/scene.gltf").value();
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
                             {{0, SYN::gfx::gl::VertexFormat::Float3, 0},
                              {1, SYN::gfx::gl::VertexFormat::Float3,
                               offsetof(Vertex, normal)},
                              {2, SYN::gfx::gl::VertexFormat::Float2,
                               offsetof(Vertex, texCoord)}},
                         },
                         2})
                    .value());
        }
        m_Shader =
            m_Context->createShader(vertexSource, fragmentSource).value();
    }
    void onUIRender() override { ImGui::ShowDemoWindow(); }
    void onRender() override {
        {
            SYN::gfx::gl::Viewport vp;
            int w, h;
            glfwGetWindowSize(m_WindowHandle, &w, &h);
            vp.width = w;
            vp.height = h;

            SYN::gfx::gl::PipelineState pipeline;
            pipeline.shader = m_Shader;

            SYN::gfx::gl::Pass pass = m_Context->beginPass(
                {std::nullopt, glm::vec4(0.2, 0.2, 0.53, 1.0), true, false, vp,
                 std::nullopt});

            glm::mat4 model(1.0);
            glm::mat4 view(1.0);
            glm::mat4 projection(1.0);

            model = glm::scale(model, glm::vec3(0.1));

            double time = glfwGetTime();
            view =
                glm::lookAt(glm::vec3(cosf(time) * 2.0, 0.0, sinf(time) * 2.0f),
                            glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0));

            projection =
                glm::perspective(90.0f, (float)w / (float)h, 0.01f, 100.0f);

            pass.usePipeline(pipeline);
            for (int i = 0; i < m_VertexArrays.size(); ++i) {
                const Mesh &mesh = m_Model.meshes[i];

                pass.bindUniform("u_MVP", projection * view * model *
                                              mesh.localTransform);
                pass.bindVertexArray(m_VertexArrays[i]);
                pass.drawIndexed(mesh.indices.size());
            }
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
    SYN::gfx::gl::Handle<SYN::gfx::gl::Shader> m_Shader;
    std::vector<SYN::gfx::gl::Handle<SYN::gfx::gl::VertexArray>> m_VertexArrays;
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
