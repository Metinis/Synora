#pragma once

#include <queue>

#include "Application.h"
#include "InputTypes.h"

namespace SYN {
class Window;
class InputContext;

class Input {
  public:
    Input() = default;
    ~Input() = default;

    void init(EngineContext* ctx);

    void onKeyEvent(int32_t keyCode, int32_t action);
    void onMouseButtonEvent(int32_t buttonCode, int32_t action);
    void onMouseMoveEvent(double xPos, double yPos);
    void onMouseScrollEvent(double xOffset, double yOffset);

    bool isKeyDown(InputKey keyCode);

    void removeContext(InputContextHandle userHandle);

    void enableContext(InputContextHandle userHandle);
    void disableContext(InputContextHandle userHandle);

    // Returns raw pointer to input context. Never store this as the user!
    // Cannot guarantee that pointer will still point to the same input context.
    std::optional<InputContext *>
    getInputContext(InputContextHandle userHandle);

    // Call every frame to dispatch update to user.
    void processInputQueue();

    std::optional<InputContextHandle> addInputContext(uint8_t priority);

    void setPriority(InputContextHandle userHandle, uint8_t priority);

  private:
    // Validate user handle's generation and active (lowest bit) state against
    // an expected state.
    bool validateHandle(InputContextHandle userHandle, bool expectedState);

    bool validateHandle(InputContextHandle userHandle);

    // Swaps corresponding input contexts and updates internal handle indices to
    // updated context positions.
    // Expects index into internal handle.
    void swapHandlesAndContexts(uint8_t handleIndexA, uint8_t handleIndexB);

    // Rebuilds internal dispatch list (sorted by priority) after
    // adding/enabling or removing/disabling an
    // input context.
    void rebuildDispatchList();

    void processInput(RawInput input);

    std::optional<SYN::InputContextHandle>
    addInputContext(uint8_t priority, std::unique_ptr<InputContext> context);

  private:
    struct ContextDispatch {
        uint8_t contextIndex;
        uint8_t priority;
    };

  private:
    double m_LastMousePosX = 0.0;
    double m_LastMousePosY = 0.0;

  private:
    std::array<bool, NUM_INPUT_KEYS> m_RawKeyStates;
    std::array<bool, NUM_MOUSE_BUTTONS> m_RawMouseButtonStates;

    std::queue<RawInput> m_RawInputQueue;

    std::vector<InputContextHandle> m_InputContextHandles;
    std::vector<ContextDispatch> m_DispatchIndices;

    std::vector<std::unique_ptr<InputContext>> m_InputContexts;
    uint8_t m_ActiveContextsCount;
    std::vector<uint8_t> m_FreeSlots;

    // Maps InputContext to InputContextHandle
    std::vector<uint8_t> m_InputContextToHandle;
};

}; // namespace SYN
