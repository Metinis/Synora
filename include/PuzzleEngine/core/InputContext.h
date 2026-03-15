#pragma once

#include <unordered_set>

#include "InputTypes.h"

namespace SYN {
class InputContext {
  public:
    InputContext() = default;
    virtual ~InputContext() = default;
    virtual void onActionReceive(ActionBinding actionBinding) = 0;

    void enableAction(ActionID action);
    void disableAction(ActionID action);

    bool isActionEnabled(ActionID action);

    uint8_t getPriority() const;
    void setPriority(uint8_t priority);

    bool consumesInput() const;
    void setConsumesInput(bool consumeInput);

  private:
    bool m_ShouldConsumeInput;
    uint8_t m_Priority;
    std::unordered_set<ActionID> m_EnabledActions;
};
} // namespace SYN
