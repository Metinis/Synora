#include <PuzzleEngine/core/InputContext.h>

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
getAxisDelta(SYN::RawInput input) {
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
        spdlog::warn("Mapped invalid trigger to axis delta. Only supports: "
                     "Key, mouse button, or mouse delta.");
        return 0.0f;
    }
    }
}

void SYN::InputContext::updateAxisWithDiscreteDelta(
    const std::string &vectorAxisName, ActionID action,
    InputContext::VectorAxis &vectorAxis, float delta) {
    if (vectorAxis.up && vectorAxis.up.value() == action) {
        vectorAxis.y += delta;
    }
    if (vectorAxis.down && vectorAxis.down.value() == action) {
        vectorAxis.y -= delta;
    }
    if (vectorAxis.right && vectorAxis.right.value() == action) {
        vectorAxis.x += delta;
    }
    if (vectorAxis.left && vectorAxis.left.value() == action) {
        vectorAxis.x -= delta;
    }

    float axisLength2 =
        vectorAxis.x * vectorAxis.x + vectorAxis.y * vectorAxis.y;

    const std::vector<VectorAxisCallback> &vectorAxisCallbacks =
        m_VectorAxisCallbacks.at(vectorAxisName);

    if (axisLength2 == 0.0f || axisLength2 == 1.0f) {
        for (const VectorAxisCallback &vectorAxisCallback :
             vectorAxisCallbacks) {
            vectorAxisCallback(vectorAxis.x, vectorAxis.y);
        }
        return;
    }

    float axisLength = std::sqrt(axisLength2);

    float normalizedX = vectorAxis.x / axisLength;
    float normalizedY = vectorAxis.y / axisLength;

    for (const VectorAxisCallback &vectorAxisCallback : vectorAxisCallbacks) {
        vectorAxisCallback(normalizedX, normalizedY);
    }
}

void SYN::InputContext::updateAxisWithMouseDelta(
    const std::string &vectorAxisName, SYN::ActionID action,
    SYN::InputContext::VectorAxis &vectorAxis, float dx, float dy) {

    vectorAxis.x += dx;
    vectorAxis.y += dy;

    const std::vector<VectorAxisCallback> &vectorAxisCallbacks =
        m_VectorAxisCallbacks.at(vectorAxisName);

    for (const VectorAxisCallback &vectorAxisCallback : vectorAxisCallbacks) {
        vectorAxisCallback(vectorAxis.x, vectorAxis.y);
    }
}

void SYN::InputContext::updateVectorAxes(ActionID action, RawInput input) {
    if (auto actionIterator = m_ActionToVectorAxis.find(action);
        actionIterator != m_ActionToVectorAxis.cend()) {
        for (const std::string &vectorAxisName : actionIterator->second) {
            VectorAxis &vectorAxis = m_VectorAxes.at(vectorAxisName);

            std::variant<float, std::tuple<float, float>> delta =
                getAxisDelta(input);

            std::visit(
                [&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same<T, float>()) {
                        updateAxisWithDiscreteDelta(vectorAxisName, action,
                                                    vectorAxis, arg);
                    } else if constexpr (std::is_same<
                                             T, std::tuple<float, float>>()) {
                        auto &[dx, dy] = arg;
                        updateAxisWithMouseDelta(vectorAxisName, action,
                                                 vectorAxis, dx, dy);
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
            updateVectorAxes(binding.action, input);
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
            newVectorAxis.up = action;
            break;
        case 1:
            newVectorAxis.down = action;
            break;
        case 2:
            newVectorAxis.right = action;
            break;
        case 3:
            newVectorAxis.left = action;
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
    if (vectorAxis.up) {
        std::vector<std::string> &axisList =
            m_ActionToVectorAxis.at(vectorAxis.up.value());
        axisList.erase(std::remove(axisList.begin(), axisList.end(), name));
    }
    if (vectorAxis.down) {
        std::vector<std::string> &axisList =
            m_ActionToVectorAxis.at(vectorAxis.down.value());
        axisList.erase(std::remove(axisList.begin(), axisList.end(), name));
    }
    if (vectorAxis.left) {
        std::vector<std::string> &axisList =
            m_ActionToVectorAxis.at(vectorAxis.left.value());
        axisList.erase(std::remove(axisList.begin(), axisList.end(), name));
    }
    if (vectorAxis.right) {
        std::vector<std::string> &axisList =
            m_ActionToVectorAxis.at(vectorAxis.right.value());
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
