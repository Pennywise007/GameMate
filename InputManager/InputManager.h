#pragma once
#include <array>
#include <atomic>
#include <functional>
#include <map>
#include <optional>
#include <windows.h>

#include <ext/core/singleton.h>

#include <ext/thread/thread.h>

class InputManager
{
    friend ext::Singleton<InputManager>;
 
    // Callback about mouse or keyboard key pressed, also includes VK_MOUSE_WHEEL and VK_MOUSE_HWHEEL
    using OnKeyOrMouseCallback = std::function<bool(WORD vkCode, bool isDown)>;
    using OnMouseMoveCallback = std::function<void(const POINT& position, const POINT& delta)>;
    using OnDirectInputMouseMoveCallback = std::function<void(const POINT& delta)>;

public:
    inline static constexpr WORD VK_MOUSE_WHEEL = 0x0E;
    inline static constexpr WORD VK_MOUSE_HWHEEL = 0x0F;

    enum class InputSimulator : uint8_t {
        Auto,
        SendInput,
        Logitech,
        LogitechGHubNew,
        Razer,
        DD,
        MouClassInputInjection
    };

    using Error = const wchar_t*;
    static std::optional<Error> SetInputSimulator(InputSimulator& inputSimulator);

    [[nodiscard]] static bool IsKeyPressed(DWORD vkCode);
    [[nodiscard]] static POINT GetMousePosition();
    
    // Subscribe/unsubscribe on low level key or mouse press
    static unsigned AddKeyOrMouseHandler(OnKeyOrMouseCallback handler);
    static void RemoveKeyOrMouseHandler(unsigned id);
    // Subscribe/unsubscribe on low level mouse move event
    static unsigned AddMouseMoveHandler(OnMouseMoveCallback handler);
    static void RemoveMouseMoveHandler(unsigned id);
    // Subscribe/unsubscribe on direct input events mouse move events from DirectX
    // Callback will be called from a different thread
    static unsigned AddDirectInputMouseMoveHandler(HINSTANCE hInstance, OnDirectInputMouseMoveCallback handler);
    static void RemoveDirectInputMouseMoveHandler(unsigned id);
    
    static void SendKeyOrMouse(WORD vkCode, bool isDown);

    static void MouseSendDown(DWORD mouseVkCode);
    static void MouseSendUp(DWORD mouseVkCode);

    static void SetCursorPos(POINT position);
    static void MouseMove(POINT delta);

    static void KeyboardSendDown(WORD vkCode);
    static void KeyboardSendUp(WORD vkCode);
private:
    InputManager();
    ~InputManager();

    static LRESULT CALLBACK LowLevelKeyboardProc(const int nCode, const WPARAM wParam, const LPARAM lParam);
    static LRESULT CALLBACK LowLevelMouseProc(const int nCode, const WPARAM wParam, const LPARAM lParam);

    bool OnKeyOrMouseEvent(WORD vkCode, bool isDown);
    void UpdateMousePosition(LONG x, LONG y);

private:
    bool m_extractInjectedEvents = false;
    std::atomic<POINT> m_mousePosition = POINT{ 0, 0 };
    std::array<std::atomic_bool, 256> m_keyStates;

    std::map<unsigned, OnKeyOrMouseCallback> m_onKeyOrMouseEvents;
    std::map<unsigned, OnMouseMoveCallback> m_onMouseMoveEvents;

    ext::thread m_hooksThread;
};
