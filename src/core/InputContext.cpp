#include <PuzzleEngine/core/InputContext.h>

#include <spdlog/spdlog.h>

#include <ranges>

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
SYN::InputContext::getBindings(InputKey key) {
    if (m_EnabledActions.find(key) == m_EnabledActions.cend())
        return std::nullopt;
    return m_EnabledActions.at(key);
}

void SYN::InputContext::updateVectorAxes(ActionID action, InputState state) {
    if (auto actionIterator = m_ActionToVectorAxis.find(action);
        actionIterator != m_ActionToVectorAxis.cend()) {
        for (const std::string &vectorAxisName : actionIterator->second) {
            VectorAxis &vectorAxis = m_VectorAxes.at(vectorAxisName);
            float delta = state == InputState::Down ? 1.0f : -1.0f;
            if (vectorAxis.m_Up && vectorAxis.m_Up.value() == action) {
                vectorAxis.m_Y += delta;
            }
            if (vectorAxis.m_Down && vectorAxis.m_Down.value() == action) {
                vectorAxis.m_Y -= delta;
            }
            if (vectorAxis.m_Right && vectorAxis.m_Right.value() == action) {
                vectorAxis.m_X += delta;
            }
            if (vectorAxis.m_Left && vectorAxis.m_Left.value() == action) {
                vectorAxis.m_X -= delta;
            }

            float axisLength2 = vectorAxis.m_X * vectorAxis.m_X +
                                vectorAxis.m_Y * vectorAxis.m_Y;

            const std::vector<VectorAxisCallback> &vectorAxisCallbacks =
                m_VectorAxisCallbacks.at(vectorAxisName);

            if (axisLength2 == 0.0f || axisLength2 == 1.0f) {
                for (const VectorAxisCallback &vectorAxisCallback :
                     vectorAxisCallbacks) {
                    vectorAxisCallback(vectorAxis.m_X, vectorAxis.m_Y);
                }
                continue;
            }

            float axisLength = std::sqrtf(axisLength2);

            float normalizedX = vectorAxis.m_X / axisLength;
            float normalizedY = vectorAxis.m_Y / axisLength;

            for (const VectorAxisCallback &vectorAxisCallback :
                 vectorAxisCallbacks) {
                vectorAxisCallback(normalizedX, normalizedY);
            }
        }
    }
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
            updateVectorAxes(binding.m_Action, input.m_State);
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

void SYN::InputContext::addVectorAxis(
    const std::string &name,
    const std::array<std::optional<ActionID>, 4> &axisActions) {

    std::vector<std::tuple<size_t, ActionID>> validActions;

    for (size_t i = 0; i < axisActions.size(); ++i) {
        if (axisActions[i].has_value()) {
            validActions.emplace_back(
                std::make_tuple(i, axisActions[i].value()));
        }
    }

    if (validActions.empty())
        return;

    VectorAxis newVectorAxis{};

    for (const auto &[i, action] : validActions) {
        // 0: Up, 1: Down, 2: Right, 3: Left
        switch (i) {
        case 0:
            newVectorAxis.m_Up = action;
            break;
        case 1:
            newVectorAxis.m_Down = action;
            break;
        case 2:
            newVectorAxis.m_Right = action;
            break;
        case 3:
            newVectorAxis.m_Left = action;
            break;
        }

        m_ActionToVectorAxis[action].push_back(name);
    }

    m_VectorAxes[name] = newVectorAxis;
    m_VectorAxisCallbacks[name] = std::vector<VectorAxisCallback>();
}

void SYN::InputContext::removeVectorAxisFromAction(const std::string &name) {
    if (m_VectorAxes.find(name) == m_VectorAxes.cend())
        return;
    VectorAxis vectorAxis = m_VectorAxes.at(name);
    if (vectorAxis.m_Up) {
        std::vector<std::string> &axisList =
            m_ActionToVectorAxis.at(vectorAxis.m_Up.value());
        axisList.erase(std::remove(axisList.begin(), axisList.end(), name));
    }
    if (vectorAxis.m_Down) {
        std::vector<std::string> &axisList =
            m_ActionToVectorAxis.at(vectorAxis.m_Down.value());
        axisList.erase(std::remove(axisList.begin(), axisList.end(), name));
    }
    if (vectorAxis.m_Left) {
        std::vector<std::string> &axisList =
            m_ActionToVectorAxis.at(vectorAxis.m_Left.value());
        axisList.erase(std::remove(axisList.begin(), axisList.end(), name));
    }
    if (vectorAxis.m_Right) {
        std::vector<std::string> &axisList =
            m_ActionToVectorAxis.at(vectorAxis.m_Right.value());
        axisList.erase(std::remove(axisList.begin(), axisList.end(), name));
    }
}

void SYN::InputContext::removeVectorAxis(const std::string &name) {
    if (auto vectorAxis = m_VectorAxes.find(name);
        vectorAxis != m_VectorAxes.cend()) {
        resetVectorAxis(name);
        removeVectorAxisFromAction(name);
        m_VectorAxes.erase(vectorAxis);
        m_VectorAxisCallbacks.erase(name);
    }
}

bool SYN::InputContext::addVectorAxisCallback(
    const std::string &name, const VectorAxisCallback &callback) {
    if (m_VectorAxes.find(name) == m_VectorAxes.cend()) {
        spdlog::error("Could not add vector axis callback because vector axis "
                      "isn't registered.");
        return false;
    }

    resetVectorAxis(name);
    m_VectorAxisCallbacks[name] = {callback};
    return true;
}

// Returns true if adding was successful, false if not.
bool SYN::InputContext::addVectorAxisCallbacks(
    const std::string &name, const std::vector<VectorAxisCallback> &callback) {
    if (m_VectorAxes.find(name) == m_VectorAxes.cend()) {
        spdlog::error("Could not add vector axis callback because vector axis "
                      "isn't registered.");
        return false;
    }

    resetVectorAxis(name);
    m_VectorAxisCallbacks[name] = callback;
    return true;
}

void SYN::InputContext::resetVectorAxis(const std::string &name) {
    if (m_VectorAxisCallbacks.find(name) == m_VectorAxisCallbacks.cend())
        return;

    const std::vector<VectorAxisCallback> axisCallbacks =
        m_VectorAxisCallbacks.at(name);

    for (const VectorAxisCallback &axisCallback : axisCallbacks) {
        axisCallback(0, 0);
    }
}
