#pragma once

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include <unordered_map>

// Serves as a key for the Keys map
// The name of the keycode identifies what they were originally for, but can be changed
// e.g: Keys[W].Keycode = S
enum KeyCodes
{
    A,
    D,
    W,
    S,
    Q,
    E,
    X,
    Z,
    SPACE,
    ARROW_LEFT,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    LSHIFT,
    LCTRL,
    RSHIFT,
    RCTRL,
    ENTER,
    PAGE_UP,
    PAGE_DOWN,
    ESCAPE,
    F1,
    F2,
    F3
};

class Input
{
public:
    struct Key
    {
        int Keycode{ GLFW_KEY_UNKNOWN };
        int State{ GLFW_RELEASE };
        bool PressedOrRepeated = false;
        bool JustReleased = false;

        Key(int keycode = GLFW_KEY_UNKNOWN) : Keycode(keycode) {}
    };

    std::unordered_map<int, Key> Keys;

    // Mouse
    double LastX = 0.0, LastY = 0.0;
    bool FirstMouse = true;
    float Yaw = 0.0f, Pitch = 0.0f;
    float XChange = 0.0f, YChange = 0.0f;

    GLFWwindow* Window;

    // Will enable/disable mouse acceleration if supported by the mouse.
    virtual void SetMouseAcceleration(bool Enabled);

    // Adds default entries (Like WASD) to Keys map.
    virtual void SetStandardKeys();

    virtual void SetCallbacks();

    virtual void ResetJustReleased();

    virtual glm::vec2 GetMouseWorldPosition();

    Input(GLFWwindow* InWindow = nullptr) : Window(InWindow)
    {
        if (!Window) return; // must be a default constructor, hopefully it will be set up later
        SetStandardKeys();
        SetCallbacks();
    }
private:
    virtual void KeyCallback(GLFWwindow* Window, int Key, int ScanCode, int Action, int Mods);
    virtual void MouseButtonCallback(GLFWwindow* Window, int Button, int Action, int Mods);
    virtual void ScrollCallback(GLFWwindow* Window, double X, double Y);
    virtual void CursorEnterCallback(GLFWwindow* Window, int Entered);
    virtual void CursorPositionCallback(GLFWwindow* Window, double X, double Y);
};