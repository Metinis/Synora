#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <PuzzleEngine/core/Input.h>
#include <PuzzleEngine/core/InputContext.h>
#include <PuzzleEngine/core/Window.h>

#include <optional>

static std::optional<SYN::InputKey> TranslateGLFWKey(int32_t keyCode) {
    switch (keyCode) {
    case GLFW_KEY_UP:
        return SYN::InputKey::Up;
    case GLFW_KEY_DOWN:
        return SYN::InputKey::Down;
    case GLFW_KEY_LEFT:
        return SYN::InputKey::Left;
    case GLFW_KEY_RIGHT:
        return SYN::InputKey::Right;
    case GLFW_KEY_SPACE:
        return SYN::InputKey::Space;
    case GLFW_KEY_ENTER:
        return SYN::InputKey::Enter;
    case GLFW_KEY_ESCAPE:
        return SYN::InputKey::Escape;
    case GLFW_KEY_LEFT_SHIFT:
        return SYN::InputKey::LeftShift;
    case GLFW_KEY_RIGHT_SHIFT:
        return SYN::InputKey::RightShift;
    case GLFW_KEY_LEFT_CONTROL:
        return SYN::InputKey::LeftCtrl;
    case GLFW_KEY_RIGHT_CONTROL:
        return SYN::InputKey::RightCtrl;
    case GLFW_KEY_LEFT_ALT:
        return SYN::InputKey::LeftAlt;
    case GLFW_KEY_RIGHT_ALT:
        return SYN::InputKey::RightAlt;
    case GLFW_KEY_TAB:
        return SYN::InputKey::Tab;
    case GLFW_KEY_A:
        return SYN::InputKey::A;
    case GLFW_KEY_B:
        return SYN::InputKey::B;
    case GLFW_KEY_C:
        return SYN::InputKey::C;
    case GLFW_KEY_D:
        return SYN::InputKey::D;
    case GLFW_KEY_E:
        return SYN::InputKey::E;
    case GLFW_KEY_F:
        return SYN::InputKey::F;
    case GLFW_KEY_G:
        return SYN::InputKey::G;
    case GLFW_KEY_H:
        return SYN::InputKey::H;
    case GLFW_KEY_I:
        return SYN::InputKey::I;
    case GLFW_KEY_J:
        return SYN::InputKey::J;
    case GLFW_KEY_K:
        return SYN::InputKey::K;
    case GLFW_KEY_L:
        return SYN::InputKey::L;
    case GLFW_KEY_M:
        return SYN::InputKey::M;
    case GLFW_KEY_N:
        return SYN::InputKey::N;
    case GLFW_KEY_O:
        return SYN::InputKey::O;
    case GLFW_KEY_P:
        return SYN::InputKey::P;
    case GLFW_KEY_Q:
        return SYN::InputKey::Q;
    case GLFW_KEY_R:
        return SYN::InputKey::R;
    case GLFW_KEY_S:
        return SYN::InputKey::S;
    case GLFW_KEY_T:
        return SYN::InputKey::T;
    case GLFW_KEY_U:
        return SYN::InputKey::U;
    case GLFW_KEY_V:
        return SYN::InputKey::V;
    case GLFW_KEY_W:
        return SYN::InputKey::W;
    case GLFW_KEY_X:
        return SYN::InputKey::X;
    case GLFW_KEY_Y:
        return SYN::InputKey::Y;
    case GLFW_KEY_Z:
        return SYN::InputKey::Z;
    case GLFW_KEY_0:
        return SYN::InputKey::Num0;
    case GLFW_KEY_1:
        return SYN::InputKey::Num1;
    case GLFW_KEY_2:
        return SYN::InputKey::Num2;
    case GLFW_KEY_3:
        return SYN::InputKey::Num3;
    case GLFW_KEY_4:
        return SYN::InputKey::Num4;
    case GLFW_KEY_5:
        return SYN::InputKey::Num5;
    case GLFW_KEY_6:
        return SYN::InputKey::Num6;
    case GLFW_KEY_7:
        return SYN::InputKey::Num7;
    case GLFW_KEY_8:
        return SYN::InputKey::Num8;
    case GLFW_KEY_9:
        return SYN::InputKey::Num9;
    case GLFW_KEY_F1:
        return SYN::InputKey::F1;
    case GLFW_KEY_F2:
        return SYN::InputKey::F2;
    case GLFW_KEY_F3:
        return SYN::InputKey::F3;
    case GLFW_KEY_F4:
        return SYN::InputKey::F4;
    case GLFW_KEY_F5:
        return SYN::InputKey::F5;
    case GLFW_KEY_F6:
        return SYN::InputKey::F6;
    case GLFW_KEY_F7:
        return SYN::InputKey::F7;
    case GLFW_KEY_F8:
        return SYN::InputKey::F8;
    case GLFW_KEY_F9:
        return SYN::InputKey::F9;
    case GLFW_KEY_F10:
        return SYN::InputKey::F10;
    case GLFW_KEY_F11:
        return SYN::InputKey::F11;
    case GLFW_KEY_F12:
        return SYN::InputKey::F12;
    default:
        return std::nullopt;
    }
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action,
                 int mods) {
    struct WindowPointers {
        SYN::Input *input;
    };

    WindowPointers *user_data =
        static_cast<WindowPointers *>(glfwGetWindowUserPointer(window));

    if (user_data == nullptr || user_data->input == nullptr) {
        spdlog::error("Input not passed to window context...");
        return;
    }

    user_data->input->onKeyEvent(key, action);
}

void SYN::Input::onKeyEvent(int32_t keyCode, int32_t action) {
    std::optional<SYN::InputKey> currentKey = TranslateGLFWKey(keyCode);
    if (!currentKey.has_value())
        return;

    uint32_t inputKeyIndex = (uint32_t)currentKey.value();

    if (action == GLFW_PRESS && !m_RawKeyStates[inputKeyIndex]) {
        m_RawKeyStates[inputKeyIndex] = true;
        m_RawInputQueue.emplace(InputState::Down, currentKey.value());
    } else if (action == GLFW_RELEASE && m_RawKeyStates[inputKeyIndex]) {
        m_RawKeyStates[inputKeyIndex] = false;
        m_RawInputQueue.emplace(InputState::Up, currentKey.value());
    }
}

void SYN::Input::init(const std::unique_ptr<Window> &window) {
    glfwSetKeyCallback(window->getHandle(), keyCallback);
}

uint8_t getHandleGeneration(SYN::InputContextHandle handle) {
    return (handle.m_Data & 0x00FE) >> 1;
}

bool sameGeneration(SYN::InputContextHandle handleA,
                    SYN::InputContextHandle handleB) {
    return getHandleGeneration(handleA) == getHandleGeneration(handleB);
}

void incrementGeneration(SYN::InputContextHandle &handle) {
    uint8_t currentGeneration = getHandleGeneration(handle);
    ++currentGeneration;
    handle.m_Data =
        ((currentGeneration << 1) | (handle.m_Data & ~(uint16_t)0x00FE));

    if (((handle.m_Data & 0x00FE) >> 1) == SYN::MAX_INPUT_CONTEXT_GENERATIONS) {
        spdlog::warn("Handle generation exceeds limit. Will overflow.");
    }
}

uint8_t getIndex(SYN::InputContextHandle handle) { return handle.m_Data >> 8; }

bool isHandleActive(SYN::InputContextHandle handle) {
    return handle.m_Data & 1;
}

std::optional<SYN::InputContextHandle>
SYN::Input::addInputContext(uint8_t priority,
                            std::unique_ptr<SYN::InputContext> context) {
    if (m_FreeSlots.empty()) {
        if (m_InputContexts.size() == MAX_INPUT_CONTEXTS) {
            spdlog::critical("Cannot add any more input contexts. Cannot "
                             "exceed maximum.");
            return std::nullopt;
        }
        InputContextHandle newHandle;
        newHandle.m_Data = m_InputContextHandles.size() << 8;

        m_InputContexts.push_back(std::move(context));
        m_InputContexts.back()->setPriority(priority);

        m_InputContextToHandle.push_back(m_InputContextHandles.size());
        m_InputContextHandles.push_back(newHandle);

        newHandle.m_Data |= 1;
        enableContext(newHandle);

        return newHandle;
    }

    uint8_t freeIndex = m_FreeSlots.back();
    m_FreeSlots.pop_back();

    uint8_t currentHandleGeneration =
        getHandleGeneration(m_InputContextHandles[freeIndex]);

    if (currentHandleGeneration == MAX_INPUT_CONTEXT_GENERATIONS) {
        spdlog::error(
            "Unable to generate new handle. Exhausted all generations.");
        return std::nullopt;
    }

    uint8_t currentObjectIndex = getIndex(m_InputContextHandles[freeIndex]);

    m_InputContexts[currentObjectIndex] = std::move(context);
    m_InputContexts[currentObjectIndex]->setPriority(priority);

    InputContextHandle newHandle;
    newHandle.m_Data =
        (freeIndex << 8) | (currentHandleGeneration << 1) | (uint16_t)1;

    enableContext(newHandle);

    return newHandle;
}

void SYN::Input::removeContext(InputContextHandle userHandle) {
    if (!validateHandle(userHandle)) {
        return;
    }

    uint8_t userIndex = getIndex(userHandle);
    if (m_InputContextHandles[userIndex].m_Data & 1) {
        disableContext(userHandle);
    }

    incrementGeneration(m_InputContextHandles[userIndex]);
    m_FreeSlots.push_back(userIndex);
}

bool SYN::Input::validateHandle(InputContextHandle userHandle) {
    uint8_t userIndex = getIndex(userHandle);
    if (userIndex >= m_InputContextHandles.size()) {
        spdlog::error("Input context handle is beyond valid range. Invalid.");
        return false;
    }

    InputContextHandle currentContextHandle = m_InputContextHandles[userIndex];

    if (!sameGeneration(currentContextHandle, userHandle)) {
        spdlog::error("Input context handle is outdated. Invalid.");
        return false;
    }

    return true;
}

bool SYN::Input::validateHandle(InputContextHandle userHandle,
                                bool expectedState) {
    if (!validateHandle(userHandle))
        return false;
    uint8_t userIndex = getIndex(userHandle);
    InputContextHandle currentContextHandle = m_InputContextHandles[userIndex];
    if ((currentContextHandle.m_Data & 1) != expectedState) {
        spdlog::error("Handle does not match expected state ({}). Invalid.",
                      expectedState ? "true" : "false");
        return false;
    }
    return true;
}

void SYN::Input::rebuildDispatchList() {
    m_DispatchIndices.clear();
    for (uint8_t i = 0; i < m_ActiveContextsCount; ++i) {
        m_DispatchIndices.push_back({i, m_InputContexts[i]->getPriority()});
    }
    std::sort(m_DispatchIndices.begin(), m_DispatchIndices.end(),
              [](ContextDispatch a, ContextDispatch b) {
                  return a.m_Priority < b.m_Priority;
              });
}

void SYN::Input::swapHandlesAndContexts(uint8_t handleIndexA,
                                        uint8_t handleIndexB) {
    uint8_t indexA = getIndex(m_InputContextHandles[handleIndexA]);
    uint8_t indexB = getIndex(m_InputContextHandles[handleIndexB]);

    if (indexA == indexB) {
        return;
    }

    std::iter_swap(m_InputContexts.begin() + indexA,
                   m_InputContexts.begin() + indexB);
    std::iter_swap(m_InputContextToHandle.begin() + indexA,
                   m_InputContextToHandle.begin() + indexB);

    m_InputContextHandles[handleIndexA].m_Data &= 0x00FF;
    m_InputContextHandles[handleIndexA].m_Data |= (indexB << 8);

    m_InputContextHandles[handleIndexB].m_Data &= 0x00FF;
    m_InputContextHandles[handleIndexB].m_Data |= (indexA << 8);
}

void SYN::Input::enableContext(InputContextHandle userHandle) {
    if (!validateHandle(userHandle, false) ||
        m_ActiveContextsCount == m_InputContextHandles.size()) {
        return;
    }

    uint8_t userIndex = getIndex(userHandle);

    if (m_ActiveContextsCount < m_InputContexts.size()) {
        swapHandlesAndContexts(userIndex,
                               m_InputContextToHandle[m_ActiveContextsCount]);
    }

    InputContextHandle &handleToEnable = m_InputContextHandles[userIndex];
    handleToEnable.m_Data |= (uint16_t)1;
    m_ActiveContextsCount += 1;

    rebuildDispatchList();
}

void SYN::Input::disableContext(InputContextHandle userHandle) {
    if (!validateHandle(userHandle, true) || m_ActiveContextsCount == 0) {
        return;
    }

    uint8_t userIndex = getIndex(userHandle);

    InputContextHandle &currentContextHandle = m_InputContextHandles[userIndex];
    uint8_t currentIndex = getIndex(currentContextHandle);
    currentContextHandle.m_Data &= ~(uint16_t)1;

    --m_ActiveContextsCount;
    if (m_ActiveContextsCount == 0) {
        rebuildDispatchList();
        return;
    }

    swapHandlesAndContexts(userIndex,
                           m_InputContextToHandle[m_ActiveContextsCount]);

    rebuildDispatchList();
}

void SYN::Input::processInput(RawInput input) {
    for (auto &[dispatchIndex, _] : m_DispatchIndices) {
        std::unique_ptr<InputContext> &inputContext =
            m_InputContexts[dispatchIndex];

        inputContext->onInputReceived(input, m_RawKeyStates);
        if (inputContext->consumesInput())
            break;
    }
}

bool SYN::Input::isKeyDown(InputKey keyCode) {
    return m_RawKeyStates[(Keycode)keyCode];
}

void SYN::Input::processInputQueue() {
    while (!m_RawInputQueue.empty()) {
        RawInput currentInput = m_RawInputQueue.front();
        m_RawInputQueue.pop();
        processInput(currentInput);
    }
}

std::optional<SYN::InputContext *>
SYN::Input::getInputContext(SYN::InputContextHandle userHandle) {
    if (!validateHandle(userHandle, 1)) {
        spdlog::error("Cannot retrieve input context from invalid handle.");
        return std::nullopt;
    }
    uint8_t userIndex = getIndex(userHandle);
    uint8_t contextIndex = getIndex(m_InputContextHandles[userIndex]);
    return m_InputContexts[contextIndex].get();
}
