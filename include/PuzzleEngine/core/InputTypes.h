#pragma once

namespace SYN {
using ActionID = uint32_t;
using Keycode = uint32_t;

// https://www.glfw.org/docs/latest/group__keys.html
// GLFW_KEY_LAST
constexpr uint32_t kNumKeys = 348;

enum class RawInputState : uint8_t { Down, Up };

// Map OS events into RawInput.
// Add more state as needed.
struct RawInput {
    RawInputState m_State;
    int32_t m_Code;
};

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
} // namespace SYN
