#pragma once

#include <variant>

namespace SYN {
using ActionID = uint32_t;
using Keycode = uint32_t;
using MouseButtonCode = uint32_t;

// Least significant bit: is active?
// Next 7 bits: generation
// Last 8 bits: index
// We really shouldn't have anywhere near 256 active input contexts +
// we also should't be constantly removing input contexts to warrant
// over 128 generations.
struct InputContextHandle {
    uint16_t data;
};

constexpr uint8_t MAX_INPUT_CONTEXTS = 255;
constexpr uint8_t MAX_INPUT_CONTEXT_GENERATIONS = 127;

enum class InputState : uint8_t { Down, Up };

enum class InputKey : Keycode {
    Up,
    Down,
    Left,
    Right,
    Space,
    Enter,
    Escape,
    LeftShift,
    RightShift,
    LeftCtrl,
    RightCtrl,
    LeftAlt,
    RightAlt,
    Tab,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12
};

struct MousePos {
    double x;
    double y;
};

struct MouseDelta {
    double dx;
    double dy;
};

struct MouseScroll {
    double scrollX;
    double scrollY;
};

enum class MouseButton : MouseButtonCode { Left, Right, Middle };

enum class RawInputType : uint32_t {
    Key,
    MouseButton,
    MouseMove,
    MouseDelta,
    MouseScroll
};

using InputData =
    std::variant<InputKey, MouseButton, MousePos, MouseDelta, MouseScroll>;

// Map OS events into RawInput.
// Add more state as needed.
struct RawInput {
    std::optional<InputState> state;
    RawInputType inputType;
    InputData input;
};

// Needs to be updated everytime a new input key is added.
constexpr uint32_t NUM_INPUT_KEYS = (uint32_t)InputKey::F12 + 1;
constexpr uint32_t NUM_MOUSE_BUTTONS = (uint32_t)MouseButton::Middle + 1;

} // namespace SYN
