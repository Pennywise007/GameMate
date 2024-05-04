#pragma once
#include <functional>
#include <map>
#include <windows.h>

#include <ext/core/singleton.h>
#include <ext/constexpr/map.h>

class InputManager
{
    friend ext::Singleton<InputManager>;
 
    using OnMouseEventCallback = std::function<void(WORD mouseVkKey, bool isDown)>;
    using OnMouseMoveCallback = std::function<void(const POINT& position, const POINT& delta)>;
    using OnKeyboardEventCallback = std::function<void(WORD vkKey, bool isDown)>;

public:
    inline static constexpr WORD VK_MOUSE_WHEEL = 0x07;
    inline static constexpr WORD VK_MOUSE_HWHEEL = 0x08;

    enum class Error : uint8_t {
        Success,
        InvalidArgument,
        LibraryNotFound,
        LibraryLoadFailed,
        LibraryError,
        DeviceCreateFailed,
        DeviceNotFound,
        DeviceOpenFailed
    };

    inline static constexpr ext::constexpr_map kErrorCodes = {
        std::pair{ InputManager::Error::InvalidArgument,    L"invalid argument"},
        std::pair{ InputManager::Error::LibraryNotFound,    L"library not found"},
        std::pair{ InputManager::Error::LibraryLoadFailed,  L"library load failed"},
        std::pair{ InputManager::Error::LibraryError,       L"library error"},
        std::pair{ InputManager::Error::DeviceCreateFailed, L"device creation failed"},
        std::pair{ InputManager::Error::DeviceNotFound,     L"device not found"},
        std::pair{ InputManager::Error::DeviceOpenFailed,   L"device open failed"},
    };

    enum class InputMode : uint8_t {
        Auto,
        SendInput,
        Logitech,
        Razer,
        DD,
        MouClassInputInjection
    };

    static Error InitializeInputMode(InputMode& inputMode);

    [[nodiscard]] static POINT GetMousePosition();
    static unsigned AddMouseMoveHandler(OnMouseMoveCallback handler);
    static void RemoveMouseMoveHandler(unsigned id);
    static unsigned AddMouseEventHandler(OnMouseEventCallback handler);
    static void RemoveMouseEventHandler(unsigned id);
    static void MouseSendDown(DWORD mouseVkKey);
    static void MouseSendUp(DWORD mouseVkKey);
    static void MouseMove(const POINT& position);
    
    [[nodiscard]] static bool GetKeyState(DWORD vkCode);
    static unsigned AddKeyStateEventHandler(OnKeyboardEventCallback handler);
    static void RemoveKeyStateEventHandler(unsigned id);
    static void KeyboardSendDown(WORD vkCode);
    static void KeyboardSendUp(WORD vkCode);

private:
    InputManager();
    ~InputManager();

    static LRESULT CALLBACK LowLevelKeyboardProc(const int nCode, const WPARAM wParam, const LPARAM lParam);
    static LRESULT CALLBACK LowLevelMouseProc(const int nCode, const WPARAM wParam, const LPARAM lParam);

    void OnMouseStateEvent(WORD mouseVkKey, bool isDown);
    void OnKeyboardStateEvent(WORD vkKey, bool isDown);
    void UpdateMousePosition(LONG x, LONG y);

private:
    const HHOOK m_keyboardHook;
    const HHOOK m_mouseHook;

    bool m_extractInjectedEvents = false;
    POINT m_mousePosition = { 0, 0 };

    std::map<unsigned, OnMouseEventCallback> m_onMouseEvents;
    std::map<unsigned, OnMouseMoveCallback> m_onMouseMoveEvents;
    std::map<unsigned, OnKeyboardEventCallback> m_onKeyStateEvents;
};
