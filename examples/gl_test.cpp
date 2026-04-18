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

        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    std::string fragmentSource = R"(
        #version 450 core
        out vec4 fragColor;

        void main() {
            fragColor = vec4(0.3, 0.8, 0.4, 1.0);
        }
    )";

    gl::Handle<gl::Shader> mainShader =
        glContext.createShader(vertexSource, fragmentSource).value();

    float vertices[] = {0.5, 0.5, 0.0, -0.5, -0.5, 0.0, 0.5, -0.5, 0.0};

    gl::Handle<gl::Buffer> vertexBuffer =
        glContext
            .createBuffer({gl::BufferType::Vertex, gl::BufferUsage::Static,
                           sizeof(vertices)},
                          vertices)
            .value();

    gl::VertexArrayDesc basicVertexDesc;
    basicVertexDesc.vertexBufferHandle = vertexBuffer;
    basicVertexDesc.vertexBufferStride = sizeof(float) * 3;
    basicVertexDesc.attributes[0] = {0, gl::VertexFormat::Float3, 0, false};
    basicVertexDesc.attributeCount = 1;

    gl::Handle<gl::VertexArray> vertexArray =
        glContext.createVertexArray(basicVertexDesc).value();

    gl::PipelineState defaultPipeline;
    defaultPipeline.shader = mainShader;
    defaultPipeline.polygonMode = gl::PolygonMode::Line;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        {
            gl::Viewport vp;
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            vp.width = w;
            vp.height = h;
            gl::Pass pass = glContext.beginPass(
                {std::nullopt, glm::vec4(1.0, 0.0, 0.0, 1.0), false, false, vp,
                 std::nullopt});
            pass.usePipeline(defaultPipeline);
            pass.bindVertexArray(vertexArray);
            pass.draw(3);
        }

        glContext.present();
        glContext.flushDeferredDeletes();
    }
}
