#pragma once

#include <unordered_set>

#include "InputTypes.h"

namespace SYN {
class InputContext {
  public:
    InputContext() = default;
    virtual ~InputContext() = default;
    virtual void onActionReceive(ActionID action) = 0;

    // Adds action if it doesn't exist
    void enableAction(ActionID action);
    void disableAction(ActionID action);

    bool isActionEnabled(ActionID action);

    uint32_t getPriority() const;
    bool isEnabled() const;
    bool consumesInput() const;

  private:
    bool m_ShouldConsumeInput;
    bool m_IsEnabled;
    uint32_t m_Priority;
    std::unordered_set<ActionID> m_EnabledActions;
};
} // namespace SYN
