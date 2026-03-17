#include <PuzzleEngine/core/InputContext.h>

void SYN::InputContext::bindActions(
    InputKey key, const std::vector<ActionBinding> &bindings) {
    m_EnabledActions[key] = bindings;
}

void SYN::InputContext::bindAction(InputKey key, ActionBinding binding) {
    m_EnabledActions[key] = {binding};
}

void SYN::InputContext::unbindKey(InputKey key) {
    if (m_EnabledActions.find(key) != m_EnabledActions.cend())
        m_EnabledActions.at(key).clear();
}

bool SYN::InputContext::isKeyBound(InputKey key) {
    if (m_EnabledActions.find(key) == m_EnabledActions.cend())
        return false;
    return m_EnabledActions.at(key).size() > 0;
}

// Returns copy of action bindings bound to specified key
std::optional<std::vector<SYN::InputContext::ActionBinding>>
SYN::InputContext::GetBindings(InputKey key) {
    if (m_EnabledActions.find(key) == m_EnabledActions.cend())
        return std::nullopt;
    return m_EnabledActions.at(key);
}

void SYN::InputContext::onInputReceived(
    RawInput input, const std::array<bool, NUM_INPUT_KEYS> &keyStates) {
    SYN::InputKey keyCode = input.m_Code;
    if (!isKeyBound(keyCode))
        return;
    for (const ActionBinding &binding : m_EnabledActions.at(keyCode)) {
        if (std::all_of(binding.m_RequiredModifiers.cbegin(),
                        binding.m_RequiredModifiers.cend(),
                        [&](InputKey modifier) {
                            return keyStates[(Keycode)modifier];
                        })) {
            onActionReceive(std::make_tuple(binding, input.m_State));
        }
    }
}

void SYN::InputContext::onActionReceive(
    std::tuple<ActionBinding, InputState> actionBinding) {
    auto &[actionBind, inputState] = actionBinding;
    ActionID currentAction = actionBind.m_Action;
    if (m_ActionCallbacks.find(currentAction) == m_ActionCallbacks.cend())
        return;
    const std::vector<ActionCallback> &actionCallbacks =
        m_ActionCallbacks.at(currentAction);
    for (const ActionCallback &actionCallback : actionCallbacks) {
        actionCallback(inputState);
    }
}

void SYN::InputContext::addActionCallback(ActionID action,
                                          ActionCallback callback) {
    m_ActionCallbacks[action] = {callback};
}

void SYN::InputContext::addActionCallbacks(
    ActionID action, const std::vector<ActionCallback> &callbacks) {
    m_ActionCallbacks[action] = callbacks;
}

void SYN::InputContext::removeActionCallbacks(ActionID action) {
    if (m_ActionCallbacks.find(action) == m_ActionCallbacks.cend()) {
        return;
    }
    m_ActionCallbacks.clear();
}

uint8_t SYN::InputContext::getPriority() const { return m_Priority; }

void SYN::InputContext::setPriority(uint8_t priority) { m_Priority = priority; }

bool SYN::InputContext::consumesInput() const { return m_ShouldConsumeInput; }

void SYN::InputContext::setConsumesInput(bool consumeInput) {
    m_ShouldConsumeInput = consumeInput;
}
