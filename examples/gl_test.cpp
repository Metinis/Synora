#include <PuzzleEngine/gfx/gl/gl.h>

#include <GLFW/glfw3.h>

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

        out vec4 vertexColor;

        uniform float scale;

        void main() {
            vertexColor = vec4(aColor, 1.0);
            gl_Position = vec4(aPos * scale, 1.0);
        }
    )";

    std::string fragmentSource = R"(
        #version 450 core
        out vec4 fragColor;

        in vec4 vertexColor;

        void main() {
            fragColor = vertexColor;
        }
    )";

    float vertices[] = {0.5, 0.5, 0.0,  1.0, 0.0, 0.0,  -0.5, -0.5,
                        0.0, 0.0, 1.0,  0.0, 0.5, -0.5, 0.0,  0.0,
                        0.0, 1.0, -0.5, 0.5, 0.0, 1.0,  1.0,  0.0};

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
            .createBuffer({gl::BufferType::Index, gl::BufferUsage::Static,
                           sizeof(indices)},
                          indices)
            .value();

    gl::VertexArrayDesc basicVertexDesc;
    basicVertexDesc.indexBufferHandle = indexBuffer;
    basicVertexDesc.vertexBufferHandle = vertexBuffer;
    basicVertexDesc.vertexBufferStride = sizeof(float) * 6;
    basicVertexDesc.attributes[0] = {0, gl::VertexFormat::Float3, 0, false};
    basicVertexDesc.attributes[1] = {1, gl::VertexFormat::Float3,
                                     sizeof(float) * 3, false};
    basicVertexDesc.attributeCount = 2;

    gl::Handle<gl::VertexArray> vertexArray =
        glContext.createVertexArray(basicVertexDesc).value();

    gl::PipelineState defaultPipeline;
    defaultPipeline.shader = mainShader;

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

            float scale = std::sin(glfwGetTime());

            pass.bindUniform("scale", scale);

            pass.bindVertexArray(vertexArray);
            pass.drawIndexed(6);
        }

        glContext.present();
        glContext.flushDeferredDeletes();
    }
}
