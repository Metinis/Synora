#include <SynoraEngine/core/Application.h>
#include <SynoraEngine/core/Input.h>
#include <SynoraEngine/core/InputContext.h>
#include <SynoraEngine/core/InputTypes.h>

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
    PrintStatus,
    Aim,
    CursorPos,
    LookDelta,
    ScrollZoom
};

// Creating your own input context no longer requires inheritance.
// Instead, you bind actions and then attach callbacks to those actions.
// This lets you keep game/UI state in your own structs, not inside contexts.
struct GameplayState {
    std::string m_PlayerName;
    float m_X = 0;
    float m_Y = 0;
    int m_Stamina = 100;
};

struct UIState {
    bool m_MenuOpen = false;
};

// This is handled in the application update loop for you.
// You should not be calling the onKeyEvent and processInputQueue manually.
static void simulateKey(SYN::Input &input, int key, int action) {
    input.onKeyEvent(key, action);
    input.processInputQueue();
}

static void simulateMouseButton(SYN::Input &input, int button, int action) {
    input.onMouseButtonEvent(button, action);
    input.processInputQueue();
}

static void simulateMouseMove(SYN::Input &input, double x, double y) {
    input.onMouseMoveEvent(x, y);
    input.processInputQueue();
}

static void simulateMouseScroll(SYN::Input &input, double x, double y) {
    input.onMouseScrollEvent(x, y);
    input.processInputQueue();
}

int main() {
    std::unique_ptr<SYN::Application> app =
        std::make_unique<SYN::Application>();
    app->init();

    std::unique_ptr<SYN::Input> &inputRef = app->GetInput();

    // Don't actually do this. Just for testing purposes.
    SYN::Input &input = *inputRef;

    // Create input contexts by priority. Lower values = higher priority.
    //
    // You will receive an integer handle if a context could be made.
    // Do not store pointers to contexts, just the handles to them!
    std::optional<SYN::InputContextHandle> gameplayHandle =
        input.addInputContext(0);

    // In this case, uiHandle will process actions after gameplayHandle.
    // But we usually want the ui to process actions before gameplay, we'll
    // change the priority of gameplayHandle below.
    std::optional<SYN::InputContextHandle> uiHandle = input.addInputContext(1);

    if (!gameplayHandle || !uiHandle) {
        spdlog::critical("Failed to create input contexts");
        app->shutdown();
        return 1;
    }

    // Now, gameplayHandle will run after uiHandle!
    input.setPriority(gameplayHandle.value(), 4);

    GameplayState gameplayState{"Avery"};
    UIState uiState{};

    // You can retrieve the input context from its handle, assuming the handle
    // isn't outdated. getInputContext returns an std::optional<InputContext*>
    // with a pointer to an input context if the handle is valid.
    // Only use pointers in short blocks of code! Never store them across
    // function calls!
    if (auto gameplayCtx = input.getInputContext(gameplayHandle.value())) {
        (*gameplayCtx)->setConsumesInput(false);

        // You may specify an ActionBinding and bind it to a key. An
        // ActionBinding is of the form { ActionID, vector<InputKey> modifiers
        // }. As the user, you are free to define your own meanings for actions,
        // as actions are simply processed as integers. I recommend a large enum
        // UserAction {} so you don't accidentally duplicate values for
        // different actions.
        (*gameplayCtx)->bindActions(SYN::InputKey::W, {{Action::MoveUp, {}}});
        (*gameplayCtx)->bindActions(SYN::InputKey::S, {{Action::MoveDown, {}}});
        (*gameplayCtx)->bindActions(SYN::InputKey::A, {{Action::MoveLeft, {}}});
        (*gameplayCtx)
            ->bindActions(SYN::InputKey::D, {{Action::MoveRight, {}}});

        // Alternatively, you can bind multiple actions to a key. Just pass in a
        // vector of ActionBindings. Order does matter in the action vector, as
        // here, Jump will be executed before Interact.
        (*gameplayCtx)
            ->bindActions(SYN::InputKey::Space,
                          {{Action::Jump, {}}, {Action::Interact, {}}});

        (*gameplayCtx)
            ->bindActions(SYN::InputKey::F, {{Action::PrintStatus, {}}});

        // Mouse button example (right click to aim).
        (*gameplayCtx)
            ->bindActions(SYN::MouseButton::Right, {{Action::Aim, {}}});

        // Mouse move, delta, and scroll triggers.
        (*gameplayCtx)
            ->bindActions(SYN::RawInputType::MouseMove,
                          {{Action::CursorPos, {}}});
        (*gameplayCtx)
            ->bindActions(SYN::RawInputType::MouseDelta,
                          {{Action::LookDelta, {}}});
        (*gameplayCtx)
            ->bindActions(SYN::RawInputType::MouseScroll,
                          {{Action::ScrollZoom, {}}});

        // Let's make use of a vector axis for directional inputs. Useful
        // for movement related code!
        (*gameplayCtx)
            ->addVectorAxis("Movement", {Action::MoveUp, Action::MoveDown,
                                         Action::MoveRight, Action::MoveLeft});

        // Now update player state based on input vector:
        (*gameplayCtx)
            ->addVectorAxisCallback("Movement", [&](float x, float y) {
                gameplayState.m_X = x;
                gameplayState.m_Y = y;
                spdlog::info("{} moves in direction: ({}, {})",
                             gameplayState.m_PlayerName, gameplayState.m_X,
                             gameplayState.m_Y);
            });

        // Action callbacks are associated with action IDs, not keys.
        // You can register lambdas that capture your game state.
        (*gameplayCtx)
            ->addActionCallback(Action::Jump, [&](SYN::InputState state) {
                if (state == SYN::InputState::Up) {
                    return;
                }
                spdlog::info("{} jumps!", gameplayState.m_PlayerName);
            });
        (*gameplayCtx)
            ->addActionCallback(Action::Interact, [&](SYN::InputState state) {
                if (state == SYN::InputState::Up) {
                    return;
                }
                if (gameplayState.m_Stamina >= 10) {
                    gameplayState.m_Stamina -= 10;
                    spdlog::info("{} interacts. Stamina now {}",
                                 gameplayState.m_PlayerName,
                                 gameplayState.m_Stamina);
                } else {
                    spdlog::warn("{} is too tired to interact.",
                                 gameplayState.m_PlayerName);
                }
            });
        (*gameplayCtx)
            ->addActionCallback(
                Action::PrintStatus, [&](SYN::InputState state) {
                    if (state == SYN::InputState::Up) {
                        return;
                    }
                    spdlog::info("{} status -> pos=({}, {}), stamina={}",
                                 gameplayState.m_PlayerName, gameplayState.m_X,
                                 gameplayState.m_Y, gameplayState.m_Stamina);
                });

        (*gameplayCtx)
            ->addActionCallback(Action::Aim, [&](SYN::InputState state) {
                if (state == SYN::InputState::Up) {
                    return;
                }
                spdlog::info("{} aims (right click).",
                             gameplayState.m_PlayerName);
            });

        (*gameplayCtx)
            ->addActionCallback(Action::CursorPos, [&](double x, double y) {
                spdlog::info("{} cursor pos -> ({}, {})",
                             gameplayState.m_PlayerName, x, y);
            });

        (*gameplayCtx)
            ->addActionCallback(Action::LookDelta, [&](double dx, double dy) {
                spdlog::info("{} mouse delta -> ({}, {})",
                             gameplayState.m_PlayerName, dx, dy);
            });

        (*gameplayCtx)
            ->addActionCallback(Action::ScrollZoom, [&](double dx, double dy) {
                spdlog::info("Scroll input -> ({}, {})", dx, dy);
            });
    }

    if (auto uiCtx = input.getInputContext(uiHandle.value())) {
        (*uiCtx)->setConsumesInput(false);
        (*uiCtx)->bindActions(SYN::InputKey::Escape,
                              {{Action::ToggleMenu, {}}});

        // Here is an example of modifier keys being used. They allow you to
        // only execute an action if the modifier keys are held in addition to
        // the main key press. Here: LeftCtrl + LeftShift + O. Keep in mind that
        // LeftShift + LeftCtrl + O will also open settings. The order of
        // modifier keys does not matter.
        (*uiCtx)->bindActions(SYN::InputKey::O, {{Action::OpenSettings,
                                                  {SYN::InputKey::LeftCtrl,
                                                   SYN::InputKey::LeftShift}}});
        (*uiCtx)->bindActions(SYN::InputKey::P, {{Action::CloseSettings, {}}});

        (*uiCtx)->addActionCallback(
            Action::ToggleMenu, [&](SYN::InputState state) {
                if (state == SYN::InputState::Up) {
                    return;
                }
                uiState.m_MenuOpen = !uiState.m_MenuOpen;
                if (auto liveUiCtx = input.getInputContext(uiHandle.value())) {
                    (*liveUiCtx)->setConsumesInput(uiState.m_MenuOpen);
                }
                spdlog::info("Menu {}",
                             uiState.m_MenuOpen ? "opened" : "closed");
            });
        (*uiCtx)->addActionCallback(Action::OpenSettings,
                                    [&](SYN::InputState state) {
                                        if (state == SYN::InputState::Up) {
                                            return;
                                        }
                                        spdlog::info("Settings opened");
                                    });
        (*uiCtx)->addActionCallback(Action::CloseSettings,
                                    [&](SYN::InputState state) {
                                        if (state == SYN::InputState::Up) {
                                            return;
                                        }
                                        spdlog::info("Settings closed");
                                    });
    }

    spdlog::info("-- Simulated gameplay input --");
    simulateKey(input, GLFW_KEY_W, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_W, GLFW_RELEASE);
    simulateKey(input, GLFW_KEY_SPACE, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_SPACE, GLFW_RELEASE);
    simulateKey(input, GLFW_KEY_F, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_F, GLFW_RELEASE);
    simulateMouseButton(input, GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
    simulateMouseButton(input, GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE);
    simulateMouseMove(input, 100.0, 200.0);
    simulateMouseMove(input, 140.0, 210.0);
    simulateMouseScroll(input, 0.0, 1.0);

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
    simulateKey(input, GLFW_KEY_W, GLFW_PRESS);

    simulateKey(input, GLFW_KEY_W, GLFW_RELEASE);
    simulateKey(input, GLFW_KEY_D, GLFW_RELEASE);

    spdlog::info("-- Disable gameplay context manually --");
    input.disableContext(gameplayHandle.value());
    simulateKey(input, GLFW_KEY_A, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_A, GLFW_RELEASE);

    spdlog::info("-- Re-enable gameplay context --");
    input.enableContext(gameplayHandle.value());
    simulateKey(input, GLFW_KEY_A, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_A, GLFW_RELEASE);

    simulateKey(input, GLFW_KEY_A, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_S, GLFW_PRESS);
    simulateKey(input, GLFW_KEY_A, GLFW_RELEASE);
    simulateKey(input, GLFW_KEY_S, GLFW_RELEASE);

    spdlog::info("-- Unbind/remove trigger/action/vector tests --");
    if (auto gameplayCtx = input.getInputContext(gameplayHandle.value())) {
        spdlog::info(
            "Unbinding D (MoveRight). Expect no movement output on D press.");
        (*gameplayCtx)->unbindTrigger(SYN::InputKey::D);
        simulateKey(input, GLFW_KEY_D, GLFW_PRESS);
        simulateKey(input, GLFW_KEY_D, GLFW_RELEASE);

        spdlog::info(
            "Unbinding right mouse button. Expect no aim output on click.");
        (*gameplayCtx)->unbindTrigger(SYN::MouseButton::Right);
        simulateMouseButton(input, GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
        simulateMouseButton(input, GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE);

        spdlog::info("Unbinding mouse move. Expect no cursor output (should "
                     "still see mouse delta though).");
        (*gameplayCtx)->unbindTrigger(SYN::RawInputType::MouseMove);
        simulateMouseMove(input, 200.0, 300.0);

        spdlog::info(
            "Removing PrintStatus callbacks. Expect no status output.");
        (*gameplayCtx)->removeActionCallbacks(Action::PrintStatus);
        simulateKey(input, GLFW_KEY_F, GLFW_PRESS);
        simulateKey(input, GLFW_KEY_F, GLFW_RELEASE);

        spdlog::info("Removing Movement vector axis. Expect no movement output "
                     "on W. Note: When unbinding a vector axis, corresponding "
                     "callbacks are called with (0, 0) to reset any states "
                     "relying on the vector axis.");
        (*gameplayCtx)->removeVectorAxis("Movement");
        simulateKey(input, GLFW_KEY_W, GLFW_PRESS);
        simulateKey(input, GLFW_KEY_W, GLFW_RELEASE);

        spdlog::info("Expect error: bindActions with RawInputType::Key.");
        (*gameplayCtx)
            ->bindActions(SYN::RawInputType::Key, {{Action::PrintStatus, {}}});

        SYN::InputContext::VectorAxisCallback noopAxis = [](float, float) {};

        spdlog::info("Expect error: addVectorAxisCallback on missing axis.");
        (*gameplayCtx)->addVectorAxisCallback("MissingAxis", noopAxis);

        spdlog::info("Expect error: addVectorAxisCallbacks on missing axis.");
        (*gameplayCtx)->addVectorAxisCallbacks("MissingAxis", {noopAxis});
    }

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
        auto temp = input.addInputContext(10);
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
        auto h = input.addInputContext(20);
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
