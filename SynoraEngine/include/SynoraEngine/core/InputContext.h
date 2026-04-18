#pragma once

#include "InputTypes.h"

namespace SYN {
class InputContext final {
  public:
    struct Trigger {
        RawInputType triggerType;
        std::optional<InputKey> key;
        std::optional<MouseButton> mouseButton;
    };

    struct ActionBinding {
        ActionID action;
        std::vector<SYN::InputKey> requiredModifiers;
    };

    using StateCallback = std::function<void(InputState)>;
    using Vec2Callback = std::function<void(double, double)>;

    using ActionCallback = std::variant<StateCallback, Vec2Callback>;

    using VectorAxisCallback = std::function<void(float, float)>;

    struct VectorAxis {
        float x;
        float y;

        std::optional<ActionID> up;
        std::optional<ActionID> down;
        std::optional<ActionID> right;
        std::optional<ActionID> left;
    };

  public:
    InputContext() = default;
    virtual ~InputContext() = default;

    // Bind multiple actions to either mouse move, scroll, or mouse delta. This
    // will overwrite any previous bindings. If trigger type is not of the
    // specified types above, the action will not be bound. For keys, or mouse
    // buttons, just pass in the key/button code directly.
    void bindActions(RawInputType type,
                     const std::vector<ActionBinding> &bindings);

    // Bind multiple actions to a key. This will overwrite any previous
    // bindings.
    void bindActions(InputKey key, const std::vector<ActionBinding> &bindings);

    // Bind multiple actions to a mouse button. This will overwrite any previous
    // bindings.
    void bindActions(MouseButton mouseButton,
                     const std::vector<ActionBinding> &bindings);

    // Removes any bound actions to specified key (if it exists)
    void unbindTrigger(InputKey key);

    // Removes any bound actions to specified mouse (if it exists)
    void unbindTrigger(MouseButton mouseButton);

    // Removes any bound actions to specified type (mouse cursor pos, mouse
    // delta, scroll wheel) if it exists
    void unbindTrigger(RawInputType type);

    // Will overwrite previous action callbacks bound to action if they
    // exist
    void addActionCallback(ActionID action, ActionCallback callback);

    // Action callbacks will be called in the order they are sent
    void addActionCallbacks(ActionID action,
                            const std::vector<ActionCallback> &callbacks);

    void removeActionCallbacks(ActionID action);

    // Attach action ids to vector axis. axisActions expects the
    // order { Up, Down, Right, Left }. As action IDs are optional,
    // you can choose to omit some of the directions. If you omit every
    // direction, the vector axis will not be added.
    void
    addVectorAxis(const std::string &name,
                  const std::array<std::optional<ActionID>, 4> &axisActions);

    void removeVectorAxis(const std::string &name);

    // Returns true if successful, false if not.
    // Overwrites vector axis callback(s) attached to name if it already
    // exists.
    bool addVectorAxisCallback(const std::string &name,
                               const VectorAxisCallback &callback);

    // Returns true if adding was successful, false if not.
    bool
    addVectorAxisCallbacks(const std::string &name,
                           const std::vector<VectorAxisCallback> &callback);

    bool isTriggerBound(InputKey key);
    bool isTriggerBound(MouseButton mouseButton);
    bool isTriggerBound(RawInputType type);

    std::optional<std::vector<ActionBinding>> getBindings(InputKey key);
    std::optional<std::vector<ActionBinding>> getBindings(MouseButton button);
    std::optional<std::vector<ActionBinding>> getBindings(RawInputType type);

    uint8_t getPriority() const;

    bool consumesInput() const;
    void setConsumesInput(bool consumeInput);

  private:
    void onInputReceived(RawInput input,
                         const std::array<bool, NUM_INPUT_KEYS> &keyStates);

    void onActionReceive(ActionBinding binding, RawInput input);

    void setPriority(uint8_t priority);

    friend class Input;

  private:
    struct TriggerHash {
        std::size_t operator()(const Trigger &trigger) const {
            uint32_t code = 0;
            if (trigger.triggerType == RawInputType::Key) {
                code = (uint32_t)trigger.key.value();
            } else if (trigger.triggerType == RawInputType::MouseButton) {
                code = (uint32_t)trigger.mouseButton.value();
            }
            return ((uint32_t)trigger.triggerType << 16) ^ code;
        }
    };

  private:
    // Will call callbacks attached to a vector axis with (0, 0).
    // Use before removing a vector axis.
    void resetVectorAxis(const std::string &name);

    void removeVectorAxisFromAction(const std::string &name);

    void updateVectorAxes(ActionID action, RawInput input);

    void updateAxisWithDiscreteDelta(const std::string &vectorAxisName,
                                     ActionID action,
                                     InputContext::VectorAxis &vectorAxis,
                                     float delta);

    void updateAxisWithMouseDelta(const std::string &vectorAxisName,
                                  ActionID action,
                                  InputContext::VectorAxis &vectorAxis,
                                  float dx, float dy);

  private:
    bool m_ShouldConsumeInput;
    uint8_t m_Priority;

    std::unordered_map<ActionID, std::vector<std::string>> m_ActionToVectorAxis;
    std::unordered_map<std::string, VectorAxis> m_VectorAxes;
    std::unordered_map<std::string, std::vector<VectorAxisCallback>>
        m_VectorAxisCallbacks;

    std::unordered_map<ActionID, std::vector<ActionCallback>> m_ActionCallbacks;
    std::unordered_map<Trigger, std::vector<ActionBinding>, TriggerHash>
        m_EnabledActions;
};

inline bool operator==(const InputContext::Trigger &triggerA,
                       const InputContext::Trigger &triggerB) {
    if (triggerA.triggerType != triggerB.triggerType)
        return false;

    if (triggerA.triggerType == RawInputType::Key) {
        if (triggerA.key.value() != triggerB.key.value())
            return false;
    } else if (triggerA.triggerType == RawInputType::MouseButton) {
        if (triggerA.mouseButton.value() != triggerB.mouseButton.value())
            return false;
    }

    return true;
}
} // namespace SYN
