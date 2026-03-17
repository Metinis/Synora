#include <PuzzleEngine/core/Application.h>
#include <PuzzleEngine/core/Input.h>
#include <PuzzleEngine/core/InputContext.h>
#include <PuzzleEngine/core/InputTypes.h>

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <string>
#include <vector>

// Define your own actions. ActionID is just an integer.
enum Action : SYN::ActionID {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Jump,
    Interact,
    ToggleMenu,
    OpenSettings,
    CloseSettings,
    PrintStatus
};

// Creating your own input context to pass into the engine
// requires deriving from InputContext and overriding onActionReceive.
// More on how to pass the input context into the engine, and on binding actions
// below.
class GameplayContext : public SYN::InputContext {
  public:
    explicit GameplayContext(std::string playerName)
        : m_PlayerName(std::move(playerName)) {
        // If an input context consumes the current input, input will not
        // propagate down to contexts with lower priority. This is useful when
        // say, opening an inventory and wanting to prevent players from moving
        // their character while the inventory is opened up. You don't have to
        // explicitly disable a context, but instead make the higher context
        // consume input.
        setConsumesInput(false);
        m_X = 0;
        m_Y = 0;
        m_Stamina = 100;
    }

    void
    onActionReceive(std::tuple<ActionBinding, SYN::InputState> input) override {
        auto &[binding, state] = input;
        if (state == SYN::InputState::Up) {
            return;
        }

        switch (binding.m_Action) {
        case Action::MoveUp:
            m_Y += 1;
            spdlog::info("{} moves up to ({}, {})", m_PlayerName, m_X, m_Y);
            break;
        case Action::MoveDown:
            m_Y -= 1;
            spdlog::info("{} moves down to ({}, {})", m_PlayerName, m_X, m_Y);
            break;
        case Action::MoveLeft:
            m_X -= 1;
            spdlog::info("{} moves left to ({}, {})", m_PlayerName, m_X, m_Y);
            break;
        case Action::MoveRight:
            m_X += 1;
            spdlog::info("{} moves right to ({}, {})", m_PlayerName, m_X, m_Y);
            break;
        case Action::Jump:
            spdlog::info("{} jumps!", m_PlayerName);
            break;
        case Action::Interact:
            if (m_Stamina >= 10) {
                m_Stamina -= 10;
                spdlog::info("{} interacts. Stamina now {}", m_PlayerName,
                             m_Stamina);
            } else {
                spdlog::warn("{} is too tired to interact.", m_PlayerName);
            }
            break;
        case Action::PrintStatus:
            spdlog::info("{} status -> pos=({}, {}), stamina={}", m_PlayerName,
                         m_X, m_Y, m_Stamina);
            break;
        default:
            break;
        }
    }

  private:
    std::string m_PlayerName;
    int m_X;
    int m_Y;
    int m_Stamina;
};

class UIContext : public SYN::InputContext {
  public:
    UIContext() {
        setConsumesInput(false);
        m_MenuOpen = false;
    }

    void
    onActionReceive(std::tuple<ActionBinding, SYN::InputState> input) override {
        auto &[binding, state] = input;
        if (state == SYN::InputState::Up) {
            return;
        }

        switch (binding.m_Action) {
        case Action::ToggleMenu:
            m_MenuOpen = !m_MenuOpen;
            setConsumesInput(m_MenuOpen);
            spdlog::info("Menu {}", m_MenuOpen ? "opened" : "closed");
            break;
        case Action::OpenSettings:
            spdlog::info("Settings opened");
            break;
        case Action::CloseSettings:
            spdlog::info("Settings closed");
            break;
        default:
            break;
        }
    }

  private:
    bool m_MenuOpen;
};

class DummyContext : public SYN::InputContext {
  public:
    DummyContext() { setConsumesInput(false); }

    void onActionReceive(std::tuple<ActionBinding, SYN::InputState>) override {}
};

// This is handled in the application update loop for you.
// You should not be calling the onKeyEvent and processInputQueue manually.
static void simulateKey(SYN::Input &input, int key, int action) {
    input.onKeyEvent(key, action);
    input.processInputQueue();
}

int main() {
    std::unique_ptr<SYN::Application> app =
        std::make_unique<SYN::Application>();
    app->init();

    std::unique_ptr<SYN::Input> &inputRef = app->GetInput();

    // Don't actually do this. Just for testing purposes.
    SYN::Input &input = *inputRef;

    // If your derived class has a constructor with arguments, you can pass
    // it in addInputContexts after passing in the priority number
    //
    // You will receive an integer handle if a context could be made
    // Do not store pointers to contexts, just the handles to them!
    std::optional<SYN::InputContextHandle> gameplayHandle =
        input.addInputContext<GameplayContext>(5, "Avery");

    // Default constructor works too! Lower values = higher priority.
    // In this case, uiHandle will process actions before gameplayHandle.
    std::optional<SYN::InputContextHandle> uiHandle =
        input.addInputContext<UIContext>(1);

    if (!gameplayHandle || !uiHandle) {
        spdlog::critical("Failed to create input contexts");
        app->shutdown();
        return 1;
    }

    // You can retrieve the input context from its handle, assuming the handle
    // isn't outdated. getInputContext returns an std::optional<InputContext*>
    // with a pointer to an input context if the handle is valid.
    // Only use pointers in short blocks of code! Never store them across
    // function calls!
    if (auto gameplayCtx = input.getInputContext(gameplayHandle.value())) {

        // You may specify an ActionBinding and bind it to a key. An
        // ActionBinding is of the form { ActionID, vector<InputKey> modifiers
        // }. As the user, you are free to define your own meanings for actions,
        // as actions are simply processed as integers. I recommend a large enum
        // UserAction {} so you don't accidentally duplicate values for
        // different actions.
        (*gameplayCtx)->bindAction(SYN::InputKey::W, {Action::MoveUp, {}});
        (*gameplayCtx)->bindAction(SYN::InputKey::S, {Action::MoveDown, {}});
        (*gameplayCtx)->bindAction(SYN::InputKey::A, {Action::MoveLeft, {}});
        (*gameplayCtx)->bindAction(SYN::InputKey::D, {Action::MoveRight, {}});

        // Alternatively, you can bind multiple actions to a key. Just pass in a
        // vector of ActionBindings. Order does matter in the action vector, as
        // here, Jump will be executed before Interact.
        (*gameplayCtx)
            ->bindActions(SYN::InputKey::Space,
                          {{Action::Jump, {}}, {Action::Interact, {}}});

        (*gameplayCtx)->bindAction(SYN::InputKey::F, {Action::PrintStatus, {}});
    }

    if (auto uiCtx = input.getInputContext(uiHandle.value())) {
        (*uiCtx)->bindAction(SYN::InputKey::Escape, {Action::ToggleMenu, {}});

        // Here is an example of modifier keys being used. They allow you to
        // only execute an action if the modifier keys are held in addition to
        // the main key press. Here: LeftCtrl + LeftShift + O. Keep in mind that
        // LeftShift + LeftCtrl + O will also open settings. The order of
        // modifier keys does not matter.
        (*uiCtx)->bindAction(SYN::InputKey::O, {Action::OpenSettings,
                                                {SYN::InputKey::LeftCtrl,
                                                 SYN::InputKey::LeftShift}});
        (*uiCtx)->bindAction(SYN::InputKey::P, {Action::CloseSettings, {}});
    }

    spdlog::info("-- Simulated gameplay input --");
    simulateKey(input, GLFW_KEY_W, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_W, GLFW_RELEASE);
    simulateKey(input, GLFW_KEY_SPACE, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_SPACE, GLFW_RELEASE);
    simulateKey(input, GLFW_KEY_F, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_F, GLFW_RELEASE);

    spdlog::info("-- Open menu (UI consumes input) --");
    simulateKey(input, GLFW_KEY_ESCAPE, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_ESCAPE, GLFW_RELEASE);

    spdlog::info("-- Gameplay input should be blocked while menu is open --");
    simulateKey(input, GLFW_KEY_D, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_D, GLFW_RELEASE);

    spdlog::info("-- Open settings with modifiers --");
    simulateKey(input, GLFW_KEY_LEFT_CONTROL, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_LEFT_SHIFT, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_O, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_O, GLFW_RELEASE);
    simulateKey(input, GLFW_KEY_LEFT_SHIFT, GLFW_RELEASE);
    simulateKey(input, GLFW_KEY_LEFT_CONTROL, GLFW_RELEASE);

    spdlog::info("-- Close settings --");
    simulateKey(input, GLFW_KEY_P, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_P, GLFW_RELEASE);

    spdlog::info("-- Close menu --");
    simulateKey(input, GLFW_KEY_ESCAPE, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_ESCAPE, GLFW_RELEASE);

    spdlog::info("-- Gameplay input restored --");
    simulateKey(input, GLFW_KEY_D, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_D, GLFW_RELEASE);

    spdlog::info("-- Disable gameplay context manually --");
    input.disableContext(gameplayHandle.value());
    simulateKey(input, GLFW_KEY_A, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_A, GLFW_RELEASE);

    spdlog::info("-- Re-enable gameplay context --");
    input.enableContext(gameplayHandle.value());
    simulateKey(input, GLFW_KEY_A, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_A, GLFW_RELEASE);

    spdlog::info("-- Intentional invalid handle usage (use-after-free) --");
    input.disableContext(uiHandle.value());
    spdlog::info("Invalid disable should produce error:");
    input.disableContext(uiHandle.value());

    spdlog::info("Remove disabled context:");
    input.removeContext(uiHandle.value());

    spdlog::info("Enable context on invalid handle should produce error:");
    input.enableContext(uiHandle.value());

    spdlog::info("Remove context on invalid handle should produce error:");
    input.removeContext(uiHandle.value());

    spdlog::info("Attempting to get input context with invalid handle should "
                 "produce error:");
    // It should also return nothing in the optional.
    if (!input.getInputContext(uiHandle.value())) {
        spdlog::info("UI context is gone as expected");
    }

    spdlog::info("-- Exhaust handle generations --");
    for (uint32_t i = 0; i < SYN::MAX_INPUT_CONTEXT_GENERATIONS + 2; ++i) {
        auto temp = input.addInputContext<DummyContext>(10);
        if (!temp) {
            spdlog::warn("Generation exhausted at iteration {}", i);
            break;
        }
        input.removeContext(temp.value());
    }

    spdlog::info("-- Exhaust max context count --");
    std::vector<SYN::InputContextHandle> handles;
    handles.reserve(SYN::MAX_INPUT_CONTEXTS);
    while (true) {
        auto h = input.addInputContext<DummyContext>(20);
        if (!h) {
            spdlog::warn("Max context count reached at {} contexts",
                         handles.size());
            break;
        }
        handles.push_back(h.value());
    }

    // Since all these handles should be valid, removing them shouldn't cause
    // any issue (no error outputted).
    for (const auto &h : handles) {
        input.removeContext(h);
    }

    app->shutdown();
    return 0;
}
