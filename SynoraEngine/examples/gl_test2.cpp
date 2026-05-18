#include <SynoraEngine/core/Application.h>
#include <SynoraEngine/core/Window.h>
#include <SynoraEngine/gfx/gl/gl.h>
#include <SynoraEngine/scene/Scene.h>
#include <spdlog/spdlog.h>

#include <imgui.h>

#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

std::string vertexSource = R"(
    #version 450 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;
    layout (location = 2) in vec2 aTexCoords;

    out vec4 vertexColor;
    out vec2 fragTexCoords;

    layout (std140, binding = 0) uniform UBO {
        float scale;
    };

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

class GraphicsScene : public SYN::ILayer {
  public:
    void init(SYN::EngineContext *engineContext) {
        m_Context = &engineContext->glContext.value();
        m_WindowHandle = engineContext->window->getHandle();
    }
    void onUIRender() override { ImGui::ShowDemoWindow(); }
    void onRender() override {
        {
            SYN::gfx::gl::Viewport vp;
            int w, h;
            glfwGetWindowSize(m_WindowHandle, &w, &h);
            vp.width = w;
            vp.height = h;
            m_Context->beginPass({std::nullopt, glm::vec4(0.2, 0.2, 0.53, 1.0),
                                  false, false, vp, std::nullopt});
        }
    }
    void onAttach() override {};
    void onDettach() override {};
    void onUpdate(float dt) override {};

  private:
    SYN::Scene *m_Scene;
    SYN::gfx::gl::Context *m_Context;
    GLFWwindow *m_WindowHandle;
};

class GLTest2 : public SYN::Application {
  public:
    GLTest2() { m_Graphics = std::make_unique<GraphicsScene>(); }

    void init() override {
        m_EngineContext.windowConfig =
            SYN::WindowConfig{"GLTest2", 1920, 1080, SYN::OpenGLConfig{4, 5}};

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
