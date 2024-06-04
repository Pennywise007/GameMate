#pragma once

#include <functional>
#include <map>

#include <ext/core/singleton.h>
#include <ext/thread/thread.h>

// Direct input support
#include <d3d11.h>
#include <dxgi.h>
#include <dinput.h>

class DirectInputManager
{
    friend ext::Singleton<DirectInputManager>;

    ~DirectInputManager();
    using OnMouseMovedCallback = std::function<void(const POINT& delta)>;
    using OnKeyboardCallback = std::function<void(WORD vkCode, bool isDown)>;;

public:
    static unsigned AddMouseMovedHandler(HINSTANCE hInstance, OnMouseMovedCallback&& handler);
    static void RemoveMouseMovedHandler(unsigned id);

    static unsigned AddKeyboardHandler(HINSTANCE hInstance, OnKeyboardCallback&& handler);
    static void RemoveKeyboardHandler(unsigned id);

private:
    bool InitHandler(HINSTANCE hInstance, bool mouse);
    void DeinitHandler(bool mouse);

private:
    LPDIRECTINPUT8 m_directInput = nullptr;
    LPDIRECTINPUTDEVICE8 m_directInputMouseDevice = nullptr;
    LPDIRECTINPUTDEVICE8 m_directInputKeyboardDevice = nullptr;

private:
    std::mutex m_callbackMutex;
    std::map<unsigned, OnMouseMovedCallback> m_mouseMoveCallbacks;
    std::map<unsigned, OnKeyboardCallback> m_keyboardCallbacks;

    ext::thread m_mouseHandlingThread;
    ext::thread m_keyboardHandlingThread;
};

