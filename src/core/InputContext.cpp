#include <PuzzleEngine/core/InputContext.h>

void SYN::InputContext::enableAction(ActionID action) {
    if (isActionEnabled(action))
        return;
    m_EnabledActions.insert(action);
}

void SYN::InputContext::disableAction(ActionID action) {
    if (!isActionEnabled(action))
        return;
    m_EnabledActions.erase(action);
}

bool SYN::InputContext::isActionEnabled(ActionID action) {
    return m_EnabledActions.contains(action);
}

uint8_t SYN::InputContext::getPriority() const { return m_Priority; }

void SYN::InputContext::setPriority(uint8_t priority) { m_Priority = priority; }

bool SYN::InputContext::consumesInput() const { return m_ShouldConsumeInput; }

void SYN::InputContext::setConsumesInput(bool consumeInput) {
    m_ShouldConsumeInput = consumeInput;
}
