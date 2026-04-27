#include <PuzzleEngine/gfx/gl/gl.h>

#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

int main(void) {
    using namespace SYN::gfx;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Window",
                                          nullptr, nullptr);
    gl::Context glContext =
        gl::Context::createContext({window, WINDOW_WIDTH, WINDOW_HEIGHT})
            .value();

    std::string vertexSource = R"(
        #version 450 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        layout (location = 2) in vec2 aTexCoords;

        out vec4 vertexColor;
        out vec2 fragTexCoords;

        uniform float scale;

        void main() {
            fragTexCoords = aTexCoords;
            vertexColor = vec4(aColor, 1.0);
            gl_Position = vec4(aPos * scale, 1.0);
        }
    )";

    std::string fragmentSource = R"(
        #version 450 core
        out vec4 fragColor;

        in vec4 vertexColor;
        in vec2 fragTexCoords;

        uniform sampler2D texture0;

        void main() {
            vec4 imageTex = texture(texture0, fragTexCoords);
            fragColor = mix(imageTex, vertexColor, 0.3);
        }
    )";

    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 texCoord;
    };

    Vertex vertices[] = {
        // position           // color           // texCoord
        {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}};

    gl::Handle<gl::Shader> mainShader =
        glContext.createShader(vertexSource, fragmentSource).value();

    unsigned int indices[] = {3, 0, 2, 3, 1, 2};

    gl::Handle<gl::Buffer> vertexBuffer =
        glContext
            .createBuffer({gl::BufferType::Vertex, gl::BufferUsage::Static,
                           sizeof(vertices)},
                          vertices)
            .value();

    gl::Handle<gl::Buffer> indexBuffer =
        glContext
            .createBuffer({gl::BufferType::Index, gl::BufferUsage::Dynamic,
                           sizeof(indices)},
                          indices)
            .value();

    gl::VertexArrayDesc basicVertexDesc;
    basicVertexDesc.indexBufferHandle = indexBuffer;
    basicVertexDesc.vertexBufferHandle = vertexBuffer;
    basicVertexDesc.vertexBufferStride = sizeof(Vertex);
    basicVertexDesc.attributes[0] = {0, gl::VertexFormat::Float3,
                                     offsetof(Vertex, position), false};
    basicVertexDesc.attributes[1] = {1, gl::VertexFormat::Float3,
                                     offsetof(Vertex, color), false};
    basicVertexDesc.attributes[2] = {2, gl::VertexFormat::Float2,
                                     offsetof(Vertex, texCoord), false};
    basicVertexDesc.attributeCount = 3;

    gl::Handle<gl::VertexArray> vertexArray =
        glContext.createVertexArray(basicVertexDesc).value();

    gl::PipelineState defaultPipeline;
    defaultPipeline.shader = mainShader;

    stbi_set_flip_vertically_on_load(true);

    int x, y, nrC;
    uint8_t *image_data =
        stbi_load("resources/textures/talkischeap.jpg", &x, &y, &nrC, 0);

    gl::TextureDesc textureDesc;
    textureDesc.width = x;
    textureDesc.height = y;
    textureDesc.format = gl::TextureFormat::RGB8;

    gl::Handle<gl::Texture> texture =
        glContext.createTexture(textureDesc, image_data).value();
    gl::Handle<gl::Sampler> sampler = glContext.createSampler({}).value();

    stbi_image_free(image_data);

    image_data =
        stbi_load("resources/textures/missing_texture.png", &x, &y, &nrC, 0);

    glContext.updateTexture(texture, 0, 100, 100, x, y,
                            gl::TextureFormat::RGBA8, image_data);
    glContext.updateTexture(texture, 0, 500, 500, x, y,
                            gl::TextureFormat::RGBA8, image_data);

    stbi_image_free(image_data);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        {
            gl::Viewport vp;
            int w, h;
            glfwGetWindowSize(window, &w, &h);
            vp.width = w;
            vp.height = h;
            gl::Pass pass = glContext.beginPass(
                {std::nullopt, glm::vec4(0.74, 0.32, 0.24, 1.0), false, false,
                 vp, std::nullopt});
            pass.usePipeline(defaultPipeline);

            float t = glfwGetTime();
            for (int i = 0; i < 4; ++i) {
                vertices[i].color.x += sinf(t * 2.0f);
                vertices[i].color.y += cosf(t * 2.0f);
                vertices[i].color.x =
                    glm::clamp(vertices[i].color.x, -1.0f, 1.0f);
                vertices[i].color.y =
                    glm::clamp(vertices[i].color.y, -1.0f, 1.0f);
            }
            glContext.updateBuffer(vertexBuffer, 0, sizeof(vertices),
                                   &vertices[0]);

            pass.bindTexture(0, texture, sampler);

            pass.bindUniform("scale", 1.0f);
            pass.bindUniform("texture0", 0);

            pass.bindVertexArray(vertexArray);
            pass.drawIndexed(6);
        }

        glContext.present();
        glContext.flushDeferredDeletes();
    }
}
