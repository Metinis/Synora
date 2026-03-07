#include "Application.h"
#include "Window.h"

using namespace SYN;
Application* Application::s_Instance = nullptr;

Application::Application() {
    s_Instance = this;
}

void Application::init() {
    //create window etc
    m_IsRunning = true;

    m_Window = std::make_unique<Window>("Synora Engine", 1280, 720);
}

void Application::run() {
    //while running and window open
    while (m_IsRunning && m_Window->isRunning()) {
        m_IsRunning = false;
    }
}

void Application::shutdown() {

}

Application::~Application() = default;
