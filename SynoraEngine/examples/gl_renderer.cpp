#include <SynoraEngine/core/Application.h>
#include <SynoraEngine/core/Window.h>
#include <SynoraEngine/gfx/gl/GL.h>

#include <glm/gtc/matrix_transform.hpp>

#include <GLFW/glfw3.h>

#include "model_loader.h"

#include <imgui.h>
#include <spdlog/spdlog.h>

using namespace SYN::gfx;

constexpr uint32_t WINDOW_WIDTH = 1920;
constexpr uint32_t WINDOW_HEIGHT = 1080;

class GraphicsScene : public SYN::ILayer {
  public:
    GraphicsScene() : m_Renderer({}) {}

    void onAttach() override {};
    void onDettach() override {};
    void init(SYN::EngineContext *engineContext) {
        m_Context = &engineContext->glContext.value();
        m_Window = engineContext->window.get();

        gl::ModelData waltuhModelData =
            gl::loadModelData("resources/assets/waltuh.glb").value();
        m_Waltuh = m_Renderer.createModel(*m_Context, waltuhModelData).value();

        gl::ModelData cabinModelData =
            gl::loadModelData("resources/assets/Cabin/scene.gltf").value();
        m_Cabin = m_Renderer.createModel(*m_Context, cabinModelData).value();

        gl::ModelData sphereModelData =
            gl::loadModelData("resources/assets/Sphere.glb").value();
        m_Sphere = m_Renderer.createModel(*m_Context, sphereModelData).value();

        m_Renderer.setDirectionalLight(m_Light);

        m_Camera.fovYDegrees = 90.0f;
        m_Camera.target = glm::vec3(0.0f);
        m_CameraSpeed = 1.0f;
        m_CameraDistance = 20.0f;
    }
    void onUpdate(float dt) override {}
    void onRender() override {

        double time = glfwGetTime();

        m_Camera.position =
            glm::vec3(cos(time * m_CameraSpeed) * m_CameraDistance, 1.85,
                      sin(time * m_CameraSpeed) * m_CameraDistance);

        auto [screenWidth, screenHeight] = m_Window->getScreenSize();

        m_Renderer.resize(screenWidth, screenHeight);
        m_Renderer.setClearColor({0.48, 0.68, 0.54, 1.0});
        m_Renderer.beginFrame(m_Camera);
        m_Renderer.submit(m_Cabin, glm::mat4(1.0));
        m_Renderer.submit(
            m_Waltuh,
            glm::rotate(
                glm::translate(glm::mat4(1.0f), glm::vec3(12.0f, 2.0f, 12.0)),
                glm::radians(180.0f), glm::vec3(0.0, 1.0, 0.0)));
        m_Renderer.submit(
            m_Sphere, glm::scale(glm::mat4(1.0f), glm::vec3(2.0f)),
            std::array<const gl::MaterialOverride, 1>{gl::MaterialOverride{
                0,
                {std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                 m_Metalness, m_Roughness,
                 glm::vec4(m_Color.r, m_Color.g, m_Color.b, 1.0f)}}});

        m_Renderer.endFrame(*m_Context);
    }
    void onUIRender() override {
        if (ImGui::Begin("Renderer Config")) {
            const char *aa[] = {"None", "FXAA", "MSAA 2x", "MSAA 4x",
                                "MSAA 8x"};
            if (ImGui::Combo("Anti Aliasing", (int *)&m_AntiAliasMode, aa,
                             IM_ARRAYSIZE(aa))) {
                m_Renderer.setAntiAliasMode(m_AntiAliasMode);
            }
        }
        ImGui::End();

        if (ImGui::Begin("Camera")) {
            ImGui::SliderFloat("Speed", &m_CameraSpeed, 0.0f, 10.0f);
            ImGui::SliderFloat("Distance", &m_CameraDistance, 0.0f, 50.0f);
            ImGui::SliderFloat("FOV", &m_Camera.fovYDegrees, 45.0f, 100.0f);
        }
        ImGui::End();

        if (ImGui::Begin("Light control")) {
            if (ImGui::ColorEdit3("Color", &m_Light.color[0])) {
                m_Renderer.setDirectionalLight(m_Light);
            }
            if (ImGui::SliderFloat3("Direction", &m_Light.direction[0], -1.0f,
                                    1.0f)) {
                m_Renderer.setDirectionalLight(m_Light);
            }
            if (ImGui::SliderFloat("Intensity", &m_Light.intensity, 0.0f,
                                   10.0f)) {
                m_Renderer.setDirectionalLight(m_Light);
            }
            if (ImGui::Checkbox("Cast shadow", &m_Light.castsShadows)) {
                m_Renderer.setDirectionalLight(m_Light);
            }
        }
        ImGui::End();

        if (ImGui::Begin("Material")) {
            ImGui::ColorEdit3("Color", &m_Color[0]);
            ImGui::SliderFloat("Roughness", &m_Roughness, 0.0, 1.0);
            ImGui::SliderFloat("Metalness", &m_Metalness, 0.0, 1.0);
        }
        ImGui::End();
    }

  private:
    gl::Context *m_Context;
    SYN::Window *m_Window;
    gl::Renderer m_Renderer;
    gl::Handle<gl::Model> m_Waltuh;
    gl::Handle<gl::Model> m_Cabin;
    gl::Handle<gl::Model> m_Sphere;
    gl::Camera m_Camera;
    float m_CameraDistance;
    float m_CameraSpeed;

    float m_Roughness;
    glm::vec3 m_Color;
    float m_Metalness;

    gl::AntiAliasMode m_AntiAliasMode = gl::AntiAliasMode::MSAA_4x;

    gl::DirectionalLight m_Light;
};

class GLRenderTest : public SYN::Application {
  public:
    GLRenderTest() { m_Graphics = std::make_unique<GraphicsScene>(); }

    void init() override {
        m_EngineContext.windowConfig = SYN::WindowConfig{
            "GLRenderer", WINDOW_WIDTH, WINDOW_HEIGHT, SYN::OpenGLConfig{4, 5}};

        SYN::Application::init();
        m_Graphics->init(&m_EngineContext);
        m_Layers.push_front(m_Graphics.get());
    }
    ~GLRenderTest() override {}

  private:
    std::unique_ptr<GraphicsScene> m_Graphics;
};

int main(void) {
    std::unique_ptr<SYN::Application> app = std::make_unique<GLRenderTest>();

    app->init();
    app->run();
    app->shutdown();
}
