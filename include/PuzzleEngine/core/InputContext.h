#pragma once

#include "InputTypes.h"

namespace SYN {
class InputContext final {
  public:
    struct ActionBinding {
        ActionID m_Action;
        std::vector<SYN::InputKey> m_RequiredModifiers;
    };

    using ActionCallback = std::function<void(InputState)>;
    using VectorAxisCallback = std::function<void(float, float)>;

    struct VectorAxis {
        float m_X;
        float m_Y;

        std::optional<ActionID> m_Up;
        std::optional<ActionID> m_Down;
        std::optional<ActionID> m_Right;
        std::optional<ActionID> m_Left;
    };

  public:
    InputContext() = default;
    virtual ~InputContext() = default;

    // Bind multiple actions to a key. This will overwrite any previous
    // bindings.
    void bindActions(InputKey key, const std::vector<ActionBinding> &bindings);
    // Bind a single action to a key. This will overwrite any previous bindings.
    void bindAction(InputKey key, ActionBinding binding);

    // Removes any bound actions to specified key (if it exists)
    void unbindKey(InputKey key);

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

    bool isKeyBound(InputKey key);

    // Returns copy of action bindings bound to specified key
    std::optional<std::vector<ActionBinding>> getBindings(SYN::InputKey key);

    uint8_t getPriority() const;

    bool consumesInput() const;
    void setConsumesInput(bool consumeInput);

  private:
    void onInputReceived(RawInput input,
                         const std::array<bool, NUM_INPUT_KEYS> &keyStates);

    void onActionReceive(std::tuple<ActionBinding, InputState> actionBinding);

    void setPriority(uint8_t priority);

    friend class Input;

  private:
    // Will call callbacks attached to a vector axis with (0, 0).
    // Use before removing a vector axis.
    void resetVectorAxis(const std::string &name);

    void removeVectorAxisFromAction(const std::string &name);

    void updateVectorAxes(ActionID action, InputState state);

  private:
    bool m_ShouldConsumeInput;
    uint8_t m_Priority;

    std::unordered_map<ActionID, std::vector<std::string>> m_ActionToVectorAxis;
    std::unordered_map<std::string, VectorAxis> m_VectorAxes;
    std::unordered_map<std::string, std::vector<VectorAxisCallback>>
        m_VectorAxisCallbacks;

    std::unordered_map<ActionID, std::vector<ActionCallback>> m_ActionCallbacks;
    std::unordered_map<SYN::InputKey, std::vector<ActionBinding>>
        m_EnabledActions;
};
} // namespace SYN
