#include "pch.h"

#include <array>

#include <ext/core/check.h>
#include <ext/core/tracer.h>

#include "InputManager.h"
#include "InputSimulator.hpp"

namespace {

// #define SEND_INPUT_ON_DRIVER_FAIL

#ifdef SEND_INPUT_ON_DRIVER_FAIL

INPUT CreateMouseInput(unsigned short vkCode, bool down)
{
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;

    switch (vkCode)
    {
    case VK_LBUTTON:
        input.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
        break;
    case VK_RBUTTON:
        input.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
        break;
    case VK_MBUTTON:
        input.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
        break;
    case VK_XBUTTON1:
        input.mi.dwFlags = down ? MOUSEEVENTF_XDOWN | XBUTTON1 : MOUSEEVENTF_XUP | XBUTTON1;
        break;
    case VK_XBUTTON2:
        input.mi.dwFlags = down ? MOUSEEVENTF_XDOWN | XBUTTON2 : MOUSEEVENTF_XUP | XBUTTON2;
        break;
    case InputManager::VK_MOUSE_WHEEL:
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = down ? -WHEEL_DELTA : WHEEL_DELTA;
        break;
    case InputManager::VK_MOUSE_HWHEEL:
        input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
        input.mi.mouseData = down ? -WHEEL_DELTA : WHEEL_DELTA;
        break;
    default:
        EXT_ASSERT(false) << "Unknown mouse event " << vkCode;
    }

    return input;
}

INPUT CreateMouseMoveInput(const POINT& position)
{
    static const auto screenWidth = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
    static const auto screenHeight = static_cast<float>(GetSystemMetrics(SM_CYSCREEN));
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));

    input.type = INPUT_MOUSE;
    input.mi.dx = static_cast<int>(float(position.x * 65535) / screenWidth);
    input.mi.dy = static_cast<int>(float(position.y * 65535) / screenHeight);
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    return input;
}

INPUT CreateMouseMoveInput(const POINT& position)
{
    static const auto screenWidth = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
    static const auto screenHeight = static_cast<float>(GetSystemMetrics(SM_CYSCREEN));
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));

    input.type = INPUT_MOUSE;
    input.mi.dx = static_cast<int>(float(position.x * 65535) / screenWidth);
    input.mi.dy = static_cast<int>(float(position.y * 65535) / screenHeight);
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    return input;
}

INPUT CreateKeyboardInput(unsigned short vkCode, bool down)
{
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));

    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.wScan = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    input.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
    return input;
}

#endif // SEND_INPUT_ON_DRIVER_FAIL

} // namespace

InputManager::Error InputManager::InitializeInputMode(InputMode& inputMode)
{
    IbSendDestroy();

    Send::Error error{};

    if (inputMode == InputMode::Auto)
    {
        constexpr std::array driversPriority = {
            Send::SendType::Razer,
            Send::SendType::Logitech,
            Send::SendType::DD,
            Send::SendType::MouClassInputInjection,
            Send::SendType::SendInput
        };

        for (const auto& mode : driversPriority)
        {
            inputMode = InputMode(mode);
            error = IbSendInit(mode, 0, 0);
            if (error == Send::Error::Success)
                break;
        }
    }
    else
        error = IbSendInit(Send::SendType(inputMode), 0, 0);

    if (error == Send::Error::Success)
    {
        EXT_TRACE() << EXT_TRACE_FUNCTION << "Driver successfully initialized " << uint32_t(inputMode);

        // If we use default SendInput method it means that we send event with injected flag which can be detected
        // by some anti-cheat programs, will try to extract this flag(no proves that it works, but a lot of people recommend)
        if (inputMode == InputMode::SendInput)
            ext::get_singleton<InputManager>().m_extractInjectedEvents = true;
    }
    else
        EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "Failed to init driver " << uint32_t(inputMode)
            << ", err " << uint32_t(error);

    return Error(error);
}

InputManager::InputManager()
    : m_keyboardHook(SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0))
    , m_mouseHook(SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, nullptr, 0))
{
    EXT_ASSERT(m_keyboardHook);
    EXT_ASSERT(m_mouseHook);
}

InputManager::~InputManager()
{
    if (m_keyboardHook)
        UnhookWindowsHookEx(m_keyboardHook);
    if (m_mouseHook)
        UnhookWindowsHookEx(m_mouseHook);
    IbSendDestroy();
}

LRESULT CALLBACK InputManager::LowLevelKeyboardProc(const int nCode, const WPARAM wParam, const LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        const auto pKeyboard = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);
        const auto key = WORD(pKeyboard->vkCode);
        const auto isDown = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;

        auto& instance = ext::get_singleton<InputManager>();
        instance.OnKeyboardStateEvent(key, isDown);

        if (instance.m_extractInjectedEvents)
        {
            pKeyboard->flags &= ~LLKHF_INJECTED;
            pKeyboard->flags &= ~LLKHF_LOWER_IL_INJECTED;
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK InputManager::LowLevelMouseProc(const int nCode, const WPARAM wParam, const LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        const auto pMouse = reinterpret_cast<PMSLLHOOKSTRUCT>(lParam);

        auto& instance = ext::get_singleton<InputManager>();
        switch (wParam)
        {
        case WM_LBUTTONDOWN:
            instance.OnMouseStateEvent(VK_LBUTTON, true);
            break;
        case WM_LBUTTONUP:
            instance.OnMouseStateEvent(VK_LBUTTON, false);
            break;
        case WM_RBUTTONDOWN:
            instance.OnMouseStateEvent(VK_RBUTTON, true);
            break;
        case WM_RBUTTONUP:
            instance.OnMouseStateEvent(VK_RBUTTON, false);
            break;
        case WM_MBUTTONDOWN:
            instance.OnMouseStateEvent(VK_MBUTTON, true);
            break;
        case WM_MBUTTONUP:
            instance.OnMouseStateEvent(VK_MBUTTON, false);
            break;
        case WM_XBUTTONDOWN:
            if (HIWORD(pMouse->mouseData) == XBUTTON1)
                instance.OnMouseStateEvent(VK_XBUTTON1, true);
            else if (HIWORD(pMouse->mouseData) == XBUTTON2)
                instance.OnMouseStateEvent(VK_XBUTTON2, true);
            break;
        case WM_XBUTTONUP:
            if (HIWORD(pMouse->mouseData) == XBUTTON1)
                instance.OnMouseStateEvent(VK_XBUTTON1, false);
            else if (HIWORD(pMouse->mouseData) == XBUTTON2)
                instance.OnMouseStateEvent(VK_XBUTTON2, false);
            break;
        case WM_MOUSEWHEEL:
            instance.OnMouseStateEvent(VK_MOUSE_WHEEL, GET_WHEEL_DELTA_WPARAM(wParam) < 0);
            break;
        case WM_MOUSEHWHEEL:
            instance.OnMouseStateEvent(VK_MOUSE_HWHEEL, GET_WHEEL_DELTA_WPARAM(wParam) < 0);
            break;
        case WM_MOUSEMOVE:
            instance.UpdateMousePosition(pMouse->pt.x, pMouse->pt.y);
            break;
        default:
            EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Unrecognized mouse command " << wParam;
            break;
        }

        if (instance.m_extractInjectedEvents)
        {
            pMouse->flags &= ~LLMHF_INJECTED;
            pMouse->flags &= ~LLMHF_LOWER_IL_INJECTED;
        }
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void InputManager::OnMouseStateEvent(WORD mouseVkKey, bool isDown)
{
    for (auto&& [_, callback] : m_onMouseEvents)
    {
        callback(mouseVkKey, isDown);
    }
}

void InputManager::OnKeyboardStateEvent(WORD vkKey, bool isDown)
{
    for (auto&& [_, callback] : m_onKeyStateEvents)
    {
        callback(vkKey, isDown);
    }
}

void InputManager::UpdateMousePosition(LONG x, LONG y)
{
    const auto position = POINT(x, y);
    const auto previousPosition = m_mousePosition;
    const POINT delta = {
        position.x - previousPosition.x,
        position.y - previousPosition.y
    };

    for (auto&& [_, callback] : m_onMouseMoveEvents)
    {
        callback(position, delta);
    }
}

bool InputManager::GetKeyState(DWORD vkCode)
{
    return ::GetKeyState(vkCode) & 0x8000;
}

POINT InputManager::GetMousePosition()
{
    // TODO do we need it?
    return ext::get_singleton<InputManager>().m_mousePosition;
}

unsigned InputManager::AddMouseMoveHandler(OnMouseMoveCallback handler)
{
    unsigned id = 0;
    auto& events = ext::get_singleton<InputManager>().m_onMouseMoveEvents;
    if (!events.empty())
        id = events.rbegin()->first + 1;
    auto res = events.try_emplace(id, std::move(handler));
    EXT_ASSERT(res.second);
    return id;
}

void InputManager::RemoveMouseMoveHandler(unsigned id)
{
    auto res = ext::get_singleton<InputManager>().m_onMouseMoveEvents.erase(id);
    EXT_ASSERT(res == 1);
}

unsigned InputManager::AddMouseEventHandler(OnMouseEventCallback handler)
{
    unsigned id = 0;
    auto& events = ext::get_singleton<InputManager>().m_onMouseEvents;
    if (!events.empty())
        id = events.rbegin()->first + 1;
    auto res = events.try_emplace(id, std::move(handler));
    EXT_ASSERT(res.second);
    return id;
}

void InputManager::RemoveMouseEventHandler(unsigned id)
{
    auto res = ext::get_singleton<InputManager>().m_onMouseEvents.erase(id);
    EXT_ASSERT(res == 1);
}

unsigned InputManager::AddKeyStateEventHandler(OnKeyboardEventCallback handler)
{
    unsigned id = 0;
    auto& events = ext::get_singleton<InputManager>().m_onKeyStateEvents;
    if (!events.empty())
        id = events.rbegin()->first + 1;
    auto res = events.try_emplace(id, std::move(handler));
    EXT_ASSERT(res.second);
    return id;
}

void InputManager::RemoveKeyStateEventHandler(unsigned id)
{
    auto res = ext::get_singleton<InputManager>().m_onKeyStateEvents.erase(id);
    EXT_ASSERT(res == 1);
}

void InputManager::MouseSendDown(DWORD mouseVkKey)
{
    Send::MouseButton button;
    switch (mouseVkKey)
    {
    case VK_LBUTTON:
        button = Send::MouseButton::LeftDown;
        break;
    case VK_RBUTTON:
        button = Send::MouseButton::RightDown;
        break;
    case VK_MBUTTON:
        button = Send::MouseButton::MiddleDown;
        break;
    case VK_XBUTTON1:
        button = Send::MouseButton::XButton1Down;
        break;
    case VK_XBUTTON2:
        button = Send::MouseButton::XButton2Down;
        break;
    case VK_MOUSE_WHEEL:
        {
            if (!IbSendMouseWheel(WHEEL_DELTA))
            {
                EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Failed to send mouse wheel down";

#ifdef SEND_INPUT_ON_DRIVER_FAIL
                INPUT input = CreateMouseInput(mouseVkKey, true);
                SendInput(1, &input, sizeof(INPUT));
                return;
#endif // SEND_INPUT_ON_DRIVER_FAIL
            }
        }
        break;
    case VK_MOUSE_HWHEEL:
        {
            INPUT input{
                .type = INPUT_MOUSE,
                .mi {
                    .dx = 0,
                    .dy = 0,
                    .mouseData = std::bit_cast<DWORD>(-WHEEL_DELTA),
                    .dwFlags = MOUSEEVENTF_HWHEEL,
                    .time = 0,
                    .dwExtraInfo = 0
                }
            };

            if (!IbSendInput(1, &input, sizeof INPUT))
            {
                EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Failed to send mouse wheel down";

#ifdef SEND_INPUT_ON_DRIVER_FAIL
                SendInput(1, &input, sizeof(INPUT));
                return;
#endif // SEND_INPUT_ON_DRIVER_FAIL
            }
        }
        break;
    default:
        EXT_ASSERT(false) << "Unknown mouse vk key " << mouseVkKey;
        return;
    }

    if (!IbSendMouseClick(button))
    {
        EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Failed to send mouse down for " << mouseVkKey;

#ifdef SEND_INPUT_ON_DRIVER_FAIL
        INPUT input = CreateMouseInput(mouseVkKey, true);
        SendInput(1, &input, sizeof(INPUT));
#endif // SEND_INPUT_ON_DRIVER_FAIL
    }
}

void InputManager::MouseSendUp(DWORD mouseVkKey)
{
    Send::MouseButton button;
    switch (mouseVkKey)
    {
    case VK_LBUTTON:
        button = Send::MouseButton::LeftUp;
        break;
    case VK_RBUTTON:
        button = Send::MouseButton::RightUp;
        break;
    case VK_MBUTTON:
        button = Send::MouseButton::MiddleUp;
        break;
    case VK_XBUTTON1:
        button = Send::MouseButton::XButton1Up;
        break;
    case VK_XBUTTON2:
        button = Send::MouseButton::XButton2Up;
        break;
    case VK_MOUSE_WHEEL:
        {
            if (!IbSendMouseWheel(WHEEL_DELTA))
            {
                EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Failed to send mouse wheel down";

#ifdef SEND_INPUT_ON_DRIVER_FAIL
                INPUT input = CreateMouseInput(mouseVkKey, false);
                SendInput(1, &input, sizeof(INPUT));
                return;
#endif // SEND_INPUT_ON_DRIVER_FAIL
            }
        }
        break;
    case VK_MOUSE_HWHEEL:
        {
            INPUT input{
                .type = INPUT_MOUSE,
                .mi {
                    .dx = 0,
                    .dy = 0,
                    .mouseData = std::bit_cast<DWORD>(WHEEL_DELTA),
                    .dwFlags = MOUSEEVENTF_HWHEEL,
                    .time = 0,
                    .dwExtraInfo = 0
                }
            };

            if (!IbSendInput(1, &input, sizeof INPUT))
            {
                EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Failed to send mouse wheel down";

#ifdef SEND_INPUT_ON_DRIVER_FAIL
                SendInput(1, &input, sizeof(INPUT));
                return;
#endif // SEND_INPUT_ON_DRIVER_FAIL
            }
        }
        break;
    default:
        EXT_ASSERT(false) << "Unknown mouse vk key " << mouseVkKey;
        return;
    }

    if (!IbSendMouseClick(button))
    {
        EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Failed to send mouse down for " << mouseVkKey;

#ifdef SEND_INPUT_ON_DRIVER_FAIL
        INPUT input = CreateMouseInput(mouseVkKey, true);
        SendInput(1, &input, sizeof(INPUT));
#endif // SEND_INPUT_ON_DRIVER_FAIL
    }
}

void InputManager::MouseMove(const POINT& position)
{
    if (!IbSendMouseMove(position.x, position.y, Send::MoveMode::Absolute))
    {
        EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Failed to send mouse move";

#ifdef SEND_INPUT_ON_DRIVER_FAIL
        INPUT input = CreateMouseMoveInput(position);
        SendInput(1, &input, sizeof(INPUT));
#endif // SEND_INPUT_ON_DRIVER_FAIL
    }
}

void InputManager::KeyboardSendDown(WORD vkCode)
{
    if (!IbSendKeybdDown(vkCode))
    {
        EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Failed to send keyboard down " << vkCode;

#ifdef SEND_INPUT_ON_DRIVER_FAIL
        INPUT input = CreateKeyboardInput(vkCode, true);
        SendInput(1, &input, sizeof(INPUT));
#endif // SEND_INPUT_ON_DRIVER_FAIL
    }
}

void InputManager::KeyboardSendUp(WORD vkCode)
{
    if (!IbSendKeybdUp(vkCode))
    {
        EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Failed to send keyboard up " << vkCode;

#ifdef SEND_INPUT_ON_DRIVER_FAIL
        INPUT input = CreateKeyboardInput(vkCode, false);
        SendInput(1, &input, sizeof(INPUT));
#endif // SEND_INPUT_ON_DRIVER_FAIL
    }
}
