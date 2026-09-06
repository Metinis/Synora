#include <SynoraEngine/core/InputContext.h>

#include <spdlog/spdlog.h>

static bool isInputButtonOrKey(SYN::RawInputType type) {
    return type != SYN::RawInputType::MouseMove &&
           type != SYN::RawInputType::MouseDelta &&
           type != SYN::RawInputType::MouseScroll;
}

static SYN::InputContext::Trigger getTriggerFromKey(SYN::InputKey key) {
    return SYN::InputContext::Trigger{SYN::RawInputType::Key, key,
                                      std::nullopt};
}

static SYN::InputContext::Trigger
getTriggerFromMouseButton(SYN::MouseButton button) {
    return SYN::InputContext::Trigger{SYN::RawInputType::MouseButton,
                                      std::nullopt, button};
}

// Do not pass in button/key type.
static SYN::InputContext::Trigger getTriggerFromType(SYN::RawInputType type) {
    return SYN::InputContext::Trigger{type, std::nullopt, std::nullopt};
}

void SYN::InputContext::bindActions(
    RawInputType type, const std::vector<ActionBinding> &bindings) {
    if (isInputButtonOrKey(type)) {
        spdlog::error(
            "Expected either scroll, mouse move, or mouse delta type. If "
            "wanting buttons, or keys, pass those directly instead!");
        return;
    }
    Trigger trigger = getTriggerFromType(type);
    m_EnabledActions[trigger] = bindings;
}

void SYN::InputContext::bindActions(
    MouseButton mouseButton, const std::vector<ActionBinding> &bindings) {
    Trigger trigger = getTriggerFromMouseButton(mouseButton);
    m_EnabledActions[trigger] = bindings;
}

void SYN::InputContext::bindActions(
    InputKey key, const std::vector<ActionBinding> &bindings) {
    Trigger trigger = getTriggerFromKey(key);
    m_EnabledActions[trigger] = bindings;
}

void SYN::InputContext::unbindTrigger(InputKey key) {
    Trigger trigger = getTriggerFromKey(key);
    if (m_EnabledActions.find(trigger) != m_EnabledActions.cend())
        m_EnabledActions.at(trigger).clear();
}

void SYN::InputContext::unbindTrigger(MouseButton mouseButton) {
    Trigger trigger = getTriggerFromMouseButton(mouseButton);
    if (m_EnabledActions.find(trigger) != m_EnabledActions.cend())
        m_EnabledActions.at(trigger).clear();
}

void SYN::InputContext::unbindTrigger(RawInputType type) {
    if (isInputButtonOrKey(type))
        return;
    Trigger trigger = getTriggerFromType(type);
    if (m_EnabledActions.find(trigger) != m_EnabledActions.cend())
        m_EnabledActions.at(trigger).clear();
}

bool SYN::InputContext::isTriggerBound(InputKey key) {
    Trigger trigger = getTriggerFromKey(key);
    if (m_EnabledActions.find(trigger) == m_EnabledActions.cend())
        return false;
    return m_EnabledActions.at(trigger).size() > 0;
}
bool SYN::InputContext::isTriggerBound(MouseButton mouseButton) {
    Trigger trigger = getTriggerFromMouseButton(mouseButton);
    if (m_EnabledActions.find(trigger) == m_EnabledActions.cend())
        return false;
    return m_EnabledActions.at(trigger).size() > 0;
}

bool SYN::InputContext::isTriggerBound(RawInputType type) {
    if (isInputButtonOrKey(type))
        return false;
    Trigger trigger = getTriggerFromType(type);
    if (m_EnabledActions.find(trigger) == m_EnabledActions.cend())
        return false;

    return m_EnabledActions.at(trigger).size() > 0;
}

// Returns copy of action bindings bound to specified key
std::optional<std::vector<SYN::InputContext::ActionBinding>>
SYN::InputContext::getBindings(InputKey key) {
    Trigger trigger = getTriggerFromKey(key);
    if (m_EnabledActions.find(trigger) == m_EnabledActions.cend())
        return std::nullopt;
    return m_EnabledActions.at(trigger);
}

std::optional<std::vector<SYN::InputContext::ActionBinding>>
SYN::InputContext::getBindings(MouseButton button) {
    Trigger trigger = getTriggerFromMouseButton(button);
    if (m_EnabledActions.find(trigger) == m_EnabledActions.cend())
        return std::nullopt;
    return m_EnabledActions.at(trigger);
}
std::optional<std::vector<SYN::InputContext::ActionBinding>>
SYN::InputContext::getBindings(RawInputType type) {
    if (isInputButtonOrKey(type))
        return std::nullopt;

    Trigger trigger = getTriggerFromType(type);
    if (m_EnabledActions.find(trigger) == m_EnabledActions.cend())
        return std::nullopt;
    return m_EnabledActions.at(trigger);
}

std::variant<float, std::tuple<float, float>>
getVectorDelta(SYN::RawInput input) {
    switch (input.inputType) {
    case SYN::RawInputType::Key:
        return input.state == SYN::InputState::Down ? 1.0f : -1.0f;
    case SYN::RawInputType::MouseButton:
        return input.state == SYN::InputState::Down ? 1.0f : -1.0f;
    case SYN::RawInputType::MouseDelta: {
        SYN::MouseDelta mouseDelta = std::get<SYN::MouseDelta>(input.input);
        return std::make_tuple((float)mouseDelta.dx, (float)mouseDelta.dy);
    }
    default: {
        spdlog::warn("Mapped invalid trigger to vector delta. Only supports: "
                     "Key, mouse button, or mouse delta.");
        return 0.0f;
    }
    }
}

void SYN::InputContext::updateInputVectorWithDiscreteDelta(
    const std::string &inputVectorName, ActionID action,
    InputContext::InputVector &inputVector, float delta) {
    if (inputVector.up && inputVector.up.value() == action) {
        inputVector.y += delta;
    }
    if (inputVector.down && inputVector.down.value() == action) {
        inputVector.y -= delta;
    }
    if (inputVector.right && inputVector.right.value() == action) {
        inputVector.x += delta;
    }
    if (inputVector.left && inputVector.left.value() == action) {
        inputVector.x -= delta;
    }

    float vectorLength2 =
        inputVector.x * inputVector.x + inputVector.y * inputVector.y;

    const std::vector<InputVectorCallback> &inputVectorCallbacks =
        m_InputVectorCallbacks.at(inputVectorName);

    if (vectorLength2 == 0.0f || vectorLength2 == 1.0f) {
        for (const InputVectorCallback &inputVectorCallback :
             inputVectorCallbacks) {
            inputVectorCallback(inputVector.x, inputVector.y);
        }
        return;
    }

    float vectorLength = std::sqrt(vectorLength2);

    float normalizedX = inputVector.x / vectorLength;
    float normalizedY = inputVector.y / vectorLength;

    for (const InputVectorCallback &inputVectorCallback :
         inputVectorCallbacks) {
        inputVectorCallback(normalizedX, normalizedY);
    }
}

void SYN::InputContext::updateInputVectorWithMouseDelta(
    const std::string &inputVectorName, SYN::ActionID action,
    SYN::InputContext::InputVector &inputVector, float dx, float dy) {

    inputVector.x += dx;
    inputVector.y += dy;

    const std::vector<InputVectorCallback> &inputVectorCallbacks =
        m_InputVectorCallbacks.at(inputVectorName);

    for (const InputVectorCallback &inputVectorCallback :
         inputVectorCallbacks) {
        inputVectorCallback(inputVector.x, inputVector.y);
    }
}

void SYN::InputContext::updateInputVectors(ActionID action, RawInput input) {
    if (auto actionIterator = m_ActionToInputVector.find(action);
        actionIterator != m_ActionToInputVector.cend()) {
        for (const std::string &inputVectorName : actionIterator->second) {
            InputVector &inputVector = m_InputVectors.at(inputVectorName);

            std::variant<float, std::tuple<float, float>> delta =
                getVectorDelta(input);

            std::visit(
                [&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same<T, float>()) {
                        updateInputVectorWithDiscreteDelta(
                            inputVectorName, action, inputVector, arg);
                    } else if constexpr (std::is_same<
                                             T, std::tuple<float, float>>()) {
                        auto &[dx, dy] = arg;
                        updateInputVectorWithMouseDelta(inputVectorName, action,
                                                        inputVector, dx, dy);
                    }
                },
                delta);
        }
    }
}

void SYN::InputContext::onInputReceived(
    RawInput input, const std::array<bool, NUM_INPUT_KEYS> &keyStates) {
    std::optional<Trigger> triggerOpt;
    std::visit(
        [&](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same<T, InputKey>()) {
                if (!isTriggerBound(arg))
                    return;
                triggerOpt = getTriggerFromKey(arg);
            } else if constexpr (std::is_same<T, MouseButton>()) {
                if (!isTriggerBound(arg))
                    return;
                triggerOpt = getTriggerFromMouseButton(arg);
            } else {
                if (!isTriggerBound(input.inputType))
                    return;
                triggerOpt = getTriggerFromType(input.inputType);
            }
        },
        input.input);
    if (!triggerOpt.has_value())
        return;
    Trigger trigger = triggerOpt.value();

    for (const ActionBinding &binding : m_EnabledActions.at(trigger)) {
        if (std::all_of(binding.requiredModifiers.cbegin(),
                        binding.requiredModifiers.cend(),
                        [&](InputKey modifier) {
                            return keyStates[(Keycode)modifier];
                        })) {
            updateInputVectors(binding.action, input);
            onActionReceive(binding, input);
        }
    }
}

void SYN::InputContext::onActionReceive(ActionBinding binding, RawInput input) {
    ActionID currentAction = binding.action;
    if (m_ActionCallbacks.find(currentAction) == m_ActionCallbacks.cend())
        return;
    const std::vector<ActionCallback> &actionCallbacks =
        m_ActionCallbacks.at(currentAction);
    for (const ActionCallback &actionCallback : actionCallbacks) {
        switch (input.inputType) {
        case SYN::RawInputType::Key:
            std::get<StateCallback>(actionCallback)(input.state.value());
            break;
        case SYN::RawInputType::MouseButton:
            std::get<StateCallback>(actionCallback)(input.state.value());
            break;
        case SYN::RawInputType::MouseMove: {
            MousePos mousePos = std::get<MousePos>(input.input);
            std::get<Vec2Callback>(actionCallback)(mousePos.x, mousePos.y);
            break;
        }
        case SYN::RawInputType::MouseDelta: {
            MouseDelta mouseDelta = std::get<MouseDelta>(input.input);
            std::get<Vec2Callback>(actionCallback)(mouseDelta.dx,
                                                   mouseDelta.dy);
            break;
        }
        case SYN::RawInputType::MouseScroll: {
            MouseScroll mouseScroll = std::get<MouseScroll>(input.input);
            std::get<Vec2Callback>(actionCallback)(mouseScroll.scrollX,
                                                   mouseScroll.scrollY);
            break;
        }
        }
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
    m_ActionCallbacks.at(action).clear();
}

uint8_t SYN::InputContext::getPriority() const { return m_Priority; }

void SYN::InputContext::setPriority(uint8_t priority) { m_Priority = priority; }

bool SYN::InputContext::consumesInput() const { return m_ShouldConsumeInput; }

void SYN::InputContext::setConsumesInput(bool consumeInput) {
    m_ShouldConsumeInput = consumeInput;
}

void SYN::InputContext::addInputVector(
    const std::string &name,
    const std::array<std::optional<ActionID>, 4> &inputVectorActions) {

    std::vector<std::tuple<size_t, ActionID>> validActions;

    for (size_t i = 0; i < inputVectorActions.size(); ++i) {
        if (inputVectorActions[i].has_value()) {
            validActions.emplace_back(
                std::make_tuple(i, inputVectorActions[i].value()));
        }
    }

    if (validActions.empty())
        return;

    InputVector newInputVector{};

    for (const auto &[i, action] : validActions) {
        // 0: Up, 1: Down, 2: Right, 3: Left
        switch (i) {
        case 0:
            newInputVector.up = action;
            break;
        case 1:
            newInputVector.down = action;
            break;
        case 2:
            newInputVector.right = action;
            break;
        case 3:
            newInputVector.left = action;
            break;
        }

        m_ActionToInputVector[action].push_back(name);
    }

    m_InputVectors[name] = newInputVector;
    m_InputVectorCallbacks[name] = std::vector<InputVectorCallback>();
}

void SYN::InputContext::removeInputVectorFromAction(const std::string &name) {
    if (m_InputVectors.find(name) == m_InputVectors.cend())
        return;
    InputVector inputVector = m_InputVectors.at(name);
    if (inputVector.up) {
        std::vector<std::string> &inputVectorList =
            m_ActionToInputVector.at(inputVector.up.value());
        inputVectorList.erase(
            std::remove(inputVectorList.begin(), inputVectorList.end(), name));
    }
    if (inputVector.down) {
        std::vector<std::string> &inputVectorList =
            m_ActionToInputVector.at(inputVector.down.value());
        inputVectorList.erase(
            std::remove(inputVectorList.begin(), inputVectorList.end(), name));
    }
    if (inputVector.left) {
        std::vector<std::string> &inputVectorList =
            m_ActionToInputVector.at(inputVector.left.value());
        inputVectorList.erase(
            std::remove(inputVectorList.begin(), inputVectorList.end(), name));
    }
    if (inputVector.right) {
        std::vector<std::string> &inputVectorList =
            m_ActionToInputVector.at(inputVector.right.value());
        inputVectorList.erase(
            std::remove(inputVectorList.begin(), inputVectorList.end(), name));
    }
}

void SYN::InputContext::removeInputVector(const std::string &name) {
    if (auto inputVector = m_InputVectors.find(name);
        inputVector != m_InputVectors.cend()) {
        resetInputVector(name);
        removeInputVectorFromAction(name);
        m_InputVectors.erase(inputVector);
        m_InputVectorCallbacks.erase(name);
    }
}

bool SYN::InputContext::addInputVectorCallback(
    const std::string &name, const InputVectorCallback &callback) {
    if (m_InputVectors.find(name) == m_InputVectors.cend()) {
        spdlog::error(
            "Could not add input vector callback because input vector name"
            "isn't registered.");
        return false;
    }

    resetInputVector(name);
    m_InputVectorCallbacks[name] = {callback};
    return true;
}

// Returns true if adding was successful, false if not.
bool SYN::InputContext::addInputVectorCallbacks(
    const std::string &name, const std::vector<InputVectorCallback> &callback) {
    if (m_InputVectors.find(name) == m_InputVectors.cend()) {
        spdlog::error(
            "Could not add input vector callback because input vector "
            "isn't registered.");
        return false;
    }

    resetInputVector(name);
    m_InputVectorCallbacks[name] = callback;
    return true;
}

void SYN::InputContext::resetInputVector(const std::string &name) {
    if (m_InputVectorCallbacks.find(name) == m_InputVectorCallbacks.cend())
        return;

    const std::vector<InputVectorCallback> inputVectorCallbacks =
        m_InputVectorCallbacks.at(name);

    for (const InputVectorCallback &inputVectorCallback :
         inputVectorCallbacks) {
        inputVectorCallback(0, 0);
    }
}
