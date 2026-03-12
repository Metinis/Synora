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

    // Adds and enables context if it doesn't already exist.
    void enableContext(const std::unique_ptr<InputContext> &context);

    void disableContext(const std::unique_ptr<InputContext> &context);

  private:
    std::unordered_map<InputKey, std::vector<ActionID>> m_ActionMap;
    std::array<bool, kNumKeys> m_RawKeyStates;
    std::vector<RawInput> m_RawInputQueue;

    std::vector<std::unique_ptr<InputContext>> m_EnabledContexts;
    std::vector<std::unique_ptr<InputContext>> m_DisabledContexts;
};

}; // namespace SYN
