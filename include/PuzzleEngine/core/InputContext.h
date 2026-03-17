#pragma once

#include "InputTypes.h"

namespace SYN {
class InputContext {
  public:
    struct ActionBinding {
        ActionID m_Action;
        std::vector<SYN::InputKey> m_RequiredModifiers;
    };

  public:
    InputContext() = default;
    virtual ~InputContext() = default;

    virtual void
    onActionReceive(std::tuple<ActionBinding, InputState> actionBinding) = 0;

    // Bind multiple actions to a key. This will overwrite any previous
    // bindings.
    void bindActions(InputKey key, const std::vector<ActionBinding> &bindings);
    // Bind a single action to a key. This will overwrite any previous bindings.
    void bindAction(InputKey key, ActionBinding binding);

    // Removes any bound actions to specified key (if it exists)
    void unbindKey(InputKey key);

    bool isKeyBound(InputKey key);

    // Returns copy of action bindings bound to specified key
    std::optional<std::vector<ActionBinding>> GetBindings(SYN::InputKey key);

    uint8_t getPriority() const;

    bool consumesInput() const;
    void setConsumesInput(bool consumeInput);

  private:
    void onInputReceived(RawInput input,
                         const std::array<bool, NUM_INPUT_KEYS> &keyStates);

    void setPriority(uint8_t priority);

    friend class Input;

  private:
    bool m_ShouldConsumeInput;
    uint8_t m_Priority;
    std::unordered_map<SYN::InputKey, std::vector<ActionBinding>>
        m_EnabledActions;
};
} // namespace SYN
