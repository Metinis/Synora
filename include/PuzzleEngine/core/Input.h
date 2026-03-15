#pragma once

#include "InputTypes.h"

namespace SYN {
class Window;
class InputContext;

class Input {
  public:
    Input() = default;
    ~Input() = default;

    void init(const std::unique_ptr<Window> &window);

    void onKeyEvent(int32_t keyCode, int32_t action);

    void assignAction(InputKey key, ActionID action);
    void removeAction(InputKey key, ActionID action);

    void removeContext(InputContextHandle userHandle);

    void enableContext(InputContextHandle userHandle);
    void disableContext(InputContextHandle userHandle);

    // Returns raw pointer to input context. Never store this as the user!
    // Cannot guarantee that pointer will still point to the same input context.
    std::optional<InputContext *>
    getInputContext(InputContextHandle userHandle);

    // Call every frame to dispatch update to user.
    void processInputQueue();

    template <class ContextDerived, typename... Args>
        requires std::derived_from<ContextDerived, InputContext>
    std::optional<InputContextHandle> addInputContext(uint8_t priority,
                                                      Args &&...args) {
        return addInputContext(priority,
                               std::move(std::make_unique<ContextDerived>(
                                   std::forward<Args>(args)...)));
    }

  private:
    // Validate user handle's generation and active (lowest bit) state against
    // an expected state.
    bool validateHandle(InputContextHandle userHandle, bool expectedState);

    // Swaps corresponding input contexts and updates internal handle indices to
    // updated context positions.
    // Expects index into internal handle.
    void swapHandlesAndContexts(uint8_t handleIndexA, uint8_t handleIndexB);

    // Rebuilds internal dispatch list (sorted by priority) after
    // adding/enabling or removing/disabling an
    // input context.
    void rebuildDispatchList();

    void processAction(ActionID action, InputState state);

    std::optional<SYN::InputContextHandle>
    addInputContext(uint8_t priority, std::unique_ptr<InputContext> context);

  private:
    std::unordered_map<InputKey, std::vector<ActionID>> m_ActionMap;
    std::array<bool, NUM_INPUT_KEYS> m_RawKeyStates;
    std::vector<RawInput> m_RawInputQueue;

    std::vector<InputContextHandle> m_InputContextHandles;
    std::vector<uint8_t> m_DispatchIndices;

    std::vector<std::unique_ptr<InputContext>> m_InputContexts;
    uint8_t m_ActiveContextsCount;
    std::vector<uint8_t> m_FreeSlots;

    // Maps InputContext to InputContextHandle
    std::vector<uint8_t> m_InputContextToHandle;
};

}; // namespace SYN
