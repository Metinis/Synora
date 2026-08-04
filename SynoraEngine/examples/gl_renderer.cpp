#include <glm/gtc/matrix_transform.hpp>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <stb_image.h>

#include <SynoraEngine/core/Application.h>
#include <SynoraEngine/core/Window.h>
#include <SynoraEngine/gfx/gl/GL.h>

#include "model_loader.h"

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

        m_Context->enableVSync(false);

        m_Renderer.init(*m_Context);

        m_CSMLayers = m_Renderer.getCSMTextures(*m_Context);

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

        int width, height, nrChannels;
        uint8_t *nx = stbi_load("resources/assets/GhibliSkybox/nx.png", &width,
                                &height, &nrChannels, 4);
        uint8_t *ny = stbi_load("resources/assets/GhibliSkybox/ny.png", &width,
                                &height, &nrChannels, 4);
        uint8_t *nz = stbi_load("resources/assets/GhibliSkybox/nz.png", &width,
                                &height, &nrChannels, 4);

        uint8_t *px = stbi_load("resources/assets/GhibliSkybox/px.png", &width,
                                &height, &nrChannels, 4);
        uint8_t *py = stbi_load("resources/assets/GhibliSkybox/py.png", &width,
                                &height, &nrChannels, 4);
        uint8_t *pz = stbi_load("resources/assets/GhibliSkybox/pz.png", &width,
                                &height, &nrChannels, 4);
        if (nx == nullptr || ny == nullptr || nz == nullptr || px == nullptr ||
            py == nullptr || pz == nullptr) {
            spdlog::error("Unable to load skybox");
            return;
        }

        m_Environments[0] = {m_Renderer.loadCubemap(
            *m_Context, {px, nx, py, ny, pz, nz}, width, height)};

        stbi_set_flip_vertically_on_load(true);
        float *hdrData =
            stbi_loadf("resources/assets/cowboy_town_saloon_4k.hdr", &width,
                       &height, &nrChannels, 4);
        assert(hdrData != nullptr);

        m_Environments[1].cubemap =
            m_Renderer.loadCubemapFromEquirectangularTexture(
                *m_Context, hdrData, width, height);

        m_Environments[1].irradianceMap = m_Renderer.createIrradianceMap(
            *m_Context, m_Environments[1].cubemap);

        m_Environments[1].prefilteredMap =
            m_Renderer.createPrefilteredEnvironmentMap(
                *m_Context, m_Environments[1].cubemap);

        stbi_image_free(hdrData);

        hdrData = stbi_loadf("resources/assets/suburban_garden_4k.hdr", &width,
                             &height, &nrChannels, 4);
        assert(hdrData != nullptr);

        m_Environments[2].cubemap =
            m_Renderer.loadCubemapFromEquirectangularTexture(
                *m_Context, hdrData, width, height);

        m_Environments[2].irradianceMap = m_Renderer.createIrradianceMap(
            *m_Context, m_Environments[2].cubemap);

        m_Environments[2].prefilteredMap =
            m_Renderer.createPrefilteredEnvironmentMap(
                *m_Context, m_Environments[2].cubemap);

        stbi_image_free(hdrData);

        hdrData = stbi_loadf("resources/assets/lobby.hdr", &width, &height,
                             &nrChannels, 4);
        assert(hdrData != nullptr);

        m_Environments[3].cubemap =
            m_Renderer.loadCubemapFromEquirectangularTexture(
                *m_Context, hdrData, width, height);

        m_Environments[3].irradianceMap = m_Renderer.createIrradianceMap(
            *m_Context, m_Environments[3].cubemap);

        m_Environments[3].prefilteredMap =
            m_Renderer.createPrefilteredEnvironmentMap(
                *m_Context, m_Environments[3].cubemap);

        stbi_image_free(hdrData);

        stbi_set_flip_vertically_on_load(false);

        stbi_image_free(px);
        stbi_image_free(nx);
        stbi_image_free(py);
        stbi_image_free(ny);
        stbi_image_free(pz);
        stbi_image_free(nz);
    }

    void onUpdate(float dt) override {
        m_FrameHistory[m_FrameIdx] = dt * 1000.0f; // seconds -> milliseconds
        (++m_FrameIdx) %= 120;

        m_FrameAvg = 0.0f;
        for (float history : m_FrameHistory)
            m_FrameAvg += history;
        m_FrameAvg /= 120.0f;
    }

    void setCameraRotation() {
        glm::mat3 rotMatrix = glm::mat3(glm::rotate(
            glm::mat4(1.0f), glm::radians(-m_Yaw), glm::vec3(0.0f, 1.0f, 0.0)));

        rotMatrix =
            glm::mat3(glm::rotate(glm::mat4(rotMatrix), glm::radians(-m_Pitch),
                                  glm::vec3(1.0f, 0.0f, 0.0)));

        m_Camera.up = rotMatrix * glm::vec3(0.0, 1.0, 0.0);
        m_Camera.target =
            m_Camera.position + rotMatrix * glm::vec3(0.0, 0.0, -1.0f);
    }

    void renderCabin() {

        double time = glfwGetTime();

        if (m_CameraSpeed != 0.0f) {
            m_Camera.position =
                glm::vec3(cos(time * m_CameraSpeed) * m_CameraDistance, 1.85,
                          sin(time * m_CameraSpeed) * m_CameraDistance);
        } else {
            m_Camera.position =
                glm::normalize(m_Camera.position) * m_CameraDistance;
        }

        setCameraRotation();

        auto [screenWidth, screenHeight] = m_Window->getScreenSize();

        m_Renderer.resize(screenWidth, screenHeight);
        m_Renderer.setClearColor({0.48, 0.68, 0.54, 1.0});
        m_Renderer.beginFrame(m_Camera);
        m_Renderer.submit(m_Cabin, glm::mat4(1.0));
        m_Renderer.submit(
            m_Waltuh,
            glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 0.0f, 0.0f)));

        m_Renderer.endFrame(*m_Context);
    }

    void renderSpheres() {
        m_Camera.position = glm::vec3(0.0f, 0.0, m_CameraDistance);

        setCameraRotation();

        auto [screenWidth, screenHeight] = m_Window->getScreenSize();

        m_Renderer.resize(screenWidth, screenHeight);
        m_Renderer.setClearColor({0.48, 0.68, 0.54, 1.0});
        m_Renderer.beginFrame(m_Camera);

        for (int row = 0; row < 7; ++row) {
            float metal = (float)row / 6.0f;
            for (int column = 0; column < 7; ++column) {
                float rough = (float)column / 6.0f;
                glm::vec3 position(column * 2.5 - 7.5f, row * 2.5 - 7.5f, 0.0);
                m_Renderer.submit(
                    m_Sphere, glm::translate(glm::mat4(1.0f), position),
                    std::array<gl::MaterialOverride, 1>{gl::MaterialOverride{
                        0,
                        {std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                         std::nullopt, metal, rough,
                         glm::vec4(m_Color.r, m_Color.g, m_Color.b, 1.0f)}}});
            }
        }

        m_Renderer.endFrame(*m_Context);
    }

    void onRender() override {
        m_Renderer.setEnvironment(m_Environments[m_EnvironmentIdx]);

        if (m_RenderMode == "Cabin") {
            renderCabin();
        } else if (m_RenderMode == "Sphere") {
            renderSpheres();
        }
    }
    void onUIRender() override {
        if (ImGui::Begin("Renderer Config")) {
            const char *aa[] = {"None", "FXAA", "MSAA 2x", "MSAA 4x",
                                "MSAA 8x"};
            if (ImGui::Combo("Anti Aliasing", (int *)&m_AntiAliasMode, aa,
                             IM_ARRAYSIZE(aa))) {
                m_Renderer.setAntiAliasMode(m_AntiAliasMode);
            }

            const char *anisotropicFiltering[] = {"None", "2x", "4x", "8x",
                                                  "16x"};
            if (ImGui::Combo("Anisotropic Filtering", &m_AnisotropicMode,
                             anisotropicFiltering,
                             IM_ARRAYSIZE(anisotropicFiltering))) {
                switch (m_AnisotropicMode) {
                case 1:
                    m_Renderer.setAnisotropicFiltering(2.0f);
                    break;
                case 2:
                    m_Renderer.setAnisotropicFiltering(4.0f);
                    break;
                case 3:
                    m_Renderer.setAnisotropicFiltering(8.0f);
                    break;
                case 4:
                    m_Renderer.setAnisotropicFiltering(16.0f);
                    break;
                default:
                    m_Renderer.setAnisotropicFiltering(1.0f);
                }
            }

            if (ImGui::SliderFloat("Gamma", &m_Gamma, gl::MIN_GAMMA,
                                   gl::MAX_GAMMA)) {
                m_Renderer.setGamma(m_Gamma);
            }
            if (ImGui::SliderFloat("Exposure", &m_Exposure, 0.0f, 10.0f)) {
                m_Renderer.setExposure(m_Exposure);
            }

            const char *environments[] = {"Ghibli (no irradiance)",
                                          "Cowboy Saloon", "Suburbs", "Lobby"};
            ImGui::Combo("Environment", &m_EnvironmentIdx, environments,
                         IM_ARRAYSIZE(environments));

            const char *renderMode[] = {"Cabin", "Sphere"};
            if (ImGui::Combo("Render mode", &m_RenderModeIdx, renderMode,
                             IM_ARRAYSIZE(renderMode))) {
                m_RenderMode = renderMode[m_RenderModeIdx];
            }
        }
        ImGui::End();

        if (ImGui::Begin("Camera")) {
            ImGui::SliderFloat("Speed", &m_CameraSpeed, 0.0f, 10.0f);
            ImGui::SliderFloat("Distance", &m_CameraDistance, 0.01f, 50.0f);
            ImGui::SliderFloat("FOV", &m_Camera.fovYDegrees, 45.0f, 100.0f);
            ImGui::SliderFloat("Yaw", &m_Yaw, -360.0f, 360.0f);
            ImGui::SliderFloat("Pitch", &m_Pitch, -90.0f, 90.0f);
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
                                   10000.0f)) {
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

        if (ImGui::Begin("Performance")) {
            ImGui::Text("%.3f ms (%.1f FPS)", m_FrameAvg, 1000.0f / m_FrameAvg);
            ImGui::PlotLines("Frame ms", m_FrameHistory, 120, m_FrameIdx,
                             nullptr, 0.0f, 33.3f, ImVec2(0, 80));
        }
        ImGui::End();

        if (ImGui::Begin("CSM Debug")) {
            const float tileSize = 256.0f;
            const float spacing = ImGui::GetStyle().ItemSpacing.x;
            const float availWidth = ImGui::GetContentRegionAvail().x;
            int perRow = (int)((availWidth + spacing) / (tileSize + spacing));
            if (perRow < 1)
                perRow = 1;

            for (size_t i = 0; i < m_CSMLayers.size(); ++i) {
                if (i % perRow != 0)
                    ImGui::SameLine();

                ImGui::BeginGroup();
                ImGui::Text("Cascade %zu", i);
                ImGui::Image((ImTextureID)(intptr_t)m_CSMLayers[i],
                             ImVec2(tileSize, tileSize), ImVec2(0, 1),
                             ImVec2(1, 0));
                ImGui::EndGroup();
            }

            if (ImGui::SliderFloat("CSM Distance", &m_CSMDistance, 0.001f,
                                   500.0f)) {
                m_Renderer.setCSMDistance(m_CSMDistance);
            }
        }
        ImGui::End();
    }

  private:
    gl::Context *m_Context;
    SYN::Window *m_Window;
    gl::Renderer m_Renderer;

    float m_FrameHistory[120] = {};
    int m_FrameIdx = 0;

    gl::Handle<gl::Model> m_Waltuh;
    gl::Handle<gl::Model> m_Cabin;
    gl::Handle<gl::Model> m_Sphere;
    std::array<gl::Environment, 4> m_Environments;

    float m_FrameAvg = 0.0f;

    float m_CSMDistance = 20.0f;
    std::vector<uint32_t> m_CSMLayers;

    int m_EnvironmentIdx = 0;
    gl::Camera m_Camera;
    float m_CameraDistance;
    float m_CameraSpeed;
    float m_Gamma = 2.2f;
    float m_Exposure = 1.0f;
    float m_Yaw = 0.0f, m_Pitch = 0.0f;

    float m_Roughness;
    glm::vec3 m_Color;
    float m_Metalness;

    std::string m_RenderMode = "Sphere";
    int m_RenderModeIdx = 1;

    gl::AntiAliasMode m_AntiAliasMode = gl::AntiAliasMode::MSAA_4x;
    int m_AnisotropicMode = 3;

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
