#include "pch.h"

#include <array>

#include "InputManager.h"
#include "InputSimulator.hpp"

#include <ext/core/check.h>
#include <ext/core/tracer.h>
#include <ext/constexpr/map.h>

#define DONT_USE_HOOK

namespace {

//#define SEND_INPUT_ON_DRIVER_FAIL

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
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));

    input.type = INPUT_MOUSE;
    input.mi.dx = position.x;
    input.mi.dy = position.y;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
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

std::optional<InputManager::Error> InputManager::SetInputSimulator(InputSimulator& inputSimulator)
{
    IbSendDestroy();

    Send::Error error{};

    if (inputSimulator == InputSimulator::Auto)
    {
        std::array driversPriority = {
            Send::SendType::Razer,
            Send::SendType::LogitechGHubNew,
            Send::SendType::DD,
            Send::SendType::MouClassInputInjection,
            Send::SendType::SendInput
        };

        for (const auto& mode : driversPriority)
        {
            inputSimulator = InputSimulator(mode);
            error = IbSendInit(mode, 0, 0);
            if (error == Send::Error::Success)
                break;
        }
    }
    else
        error = IbSendInit(Send::SendType(inputSimulator), 0, 0);

    if (error == Send::Error::Success)
    {
        EXT_TRACE() << EXT_TRACE_FUNCTION << "Input simulator successfully set " << uint32_t(inputSimulator);

        // If we use default SendInput method it means that we send event with injected flag which can be detected
        // by some anti-cheat programs, will try to extract this flag(no proves that it works, but a lot of people recommend)
        if (inputSimulator == InputSimulator::SendInput)
            ext::get_singleton<InputManager>().m_extractInjectedEvents = true;
        return std::nullopt;
    }
    else
    {
        std::map<Send::Error, const wchar_t*> kErrorCodes = {
            { Send::Error::InvalidArgument,    L"invalid argument"},
            { Send::Error::LibraryNotFound,    L"library not found"},
            { Send::Error::LibraryLoadFailed,  L"library load failed"},
            { Send::Error::LibraryError,       L"library error"},
            { Send::Error::DeviceCreateFailed, L"device creation failed"},
            { Send::Error::DeviceNotFound,     L"device not found"},
            { Send::Error::DeviceOpenFailed,   L"device open failed"},
        };

        const auto* errorText = kErrorCodes[error];
        EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "Failed to set input simulator " << uint32_t(inputSimulator)
            << ", err " << uint32_t(error) << "(" << errorText << ")";
        return errorText;
    }
}

InputManager::InputManager()
#ifdef DONT_USE_HOOK
    : m_keyboardHook(nullptr)
    , m_mouseHook(nullptr)
#else
    : m_keyboardHook(SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0))
    , m_mouseHook(SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, nullptr, 0))
#endif
{
#ifndef DONT_USE_HOOK
    for (size_t i = 0; i < m_keyStates.size(); ++i)
    {
        m_keyStates[i] = ::GetKeyState(int(i)) & 0x8000;
    }

    ::GetCursorPos(&m_mousePosition);

    try
    {
        EXT_EXPECT(m_keyboardHook) << EXT_TRACE_FUNCTION << "Failed to set keyboard hook, err " << GetLastError();
        EXT_EXPECT(m_mouseHook) << EXT_TRACE_FUNCTION << "Failed to set mouse hook, err " << GetLastError();
    }
    catch (...)
    {
        if (m_keyboardHook)
            UnhookWindowsHookEx(m_keyboardHook);
        if (m_mouseHook)
            UnhookWindowsHookEx(m_mouseHook);
        throw;
    }
#endif
}

InputManager::~InputManager()
{
#ifndef DONT_USE_HOOK
    UnhookWindowsHookEx(m_keyboardHook);
    UnhookWindowsHookEx(m_mouseHook);
#endif
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
        if (instance.OnKeyOrMouseEvent(key, isDown))
            return 1;

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

        bool absorbEvent = false;
        auto& instance = ext::get_singleton<InputManager>();
        switch (wParam)
        {
        case WM_LBUTTONDOWN:
            absorbEvent = instance.OnKeyOrMouseEvent(VK_LBUTTON, true);
            break;
        case WM_LBUTTONUP:
            absorbEvent = instance.OnKeyOrMouseEvent(VK_LBUTTON, false);
            break;
        case WM_RBUTTONDOWN:
            absorbEvent = instance.OnKeyOrMouseEvent(VK_RBUTTON, true);
            break;
        case WM_RBUTTONUP:
            absorbEvent = instance.OnKeyOrMouseEvent(VK_RBUTTON, false);
            break;
        case WM_MBUTTONDOWN:
            absorbEvent = instance.OnKeyOrMouseEvent(VK_MBUTTON, true);
            break;
        case WM_MBUTTONUP:
            absorbEvent = instance.OnKeyOrMouseEvent(VK_MBUTTON, false);
            break;
        case WM_XBUTTONDOWN:
            if (HIWORD(pMouse->mouseData) == XBUTTON1)
                absorbEvent = instance.OnKeyOrMouseEvent(VK_XBUTTON1, true);
            else if (HIWORD(pMouse->mouseData) == XBUTTON2)
                absorbEvent = instance.OnKeyOrMouseEvent(VK_XBUTTON2, true);
            break;
        case WM_XBUTTONUP:
            if (HIWORD(pMouse->mouseData) == XBUTTON1)
                absorbEvent = instance.OnKeyOrMouseEvent(VK_XBUTTON1, false);
            else if (HIWORD(pMouse->mouseData) == XBUTTON2)
                absorbEvent = instance.OnKeyOrMouseEvent(VK_XBUTTON2, false);
            break;
        case WM_MOUSEWHEEL:
            absorbEvent = instance.OnKeyOrMouseEvent(VK_MOUSE_WHEEL, GET_WHEEL_DELTA_WPARAM(wParam) < 0);
            break;
        case WM_MOUSEHWHEEL:
            absorbEvent = instance.OnKeyOrMouseEvent(VK_MOUSE_HWHEEL, GET_WHEEL_DELTA_WPARAM(wParam) < 0);
            break;
        case WM_MOUSEMOVE:
            instance.UpdateMousePosition(pMouse->pt.x, pMouse->pt.y);
            break;
        default:
            EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Unrecognized mouse command " << wParam;
            break;
        }

        if (absorbEvent)
            return 1;

        if (instance.m_extractInjectedEvents)
        {
            pMouse->flags &= ~LLMHF_INJECTED;
            pMouse->flags &= ~LLMHF_LOWER_IL_INJECTED;
        }
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool InputManager::OnKeyOrMouseEvent(WORD vkCode, bool isPressed)
{
    m_keyStates[vkCode] = isPressed;

    for (auto&& [_, callback] : m_onKeyOrMouseEvents)
    {
        if (callback(vkCode, isPressed))
            return true;
    }

    return false;
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
    auto& keyStates = ext::get_singleton<InputManager>().m_keyStates;
    if (vkCode < keyStates.size()) {
        return keyStates[vkCode];
    }
    return false;
}

POINT InputManager::GetMousePosition()
{
    return ext::get_singleton<InputManager>().m_mousePosition;
}

unsigned InputManager::AddKeyOrMouseHandler(OnKeyOrMouseCallback handler)
{
    auto& manager = ext::get_singleton<InputManager>();
    unsigned id = 0;
    auto& events = manager.m_onKeyOrMouseEvents;
    if (!events.empty())
        id = events.rbegin()->first + 1;
    auto res = events.try_emplace(id, std::move(handler));
    EXT_ASSERT(res.second);
    return id;
}

void InputManager::RemoveKeyOrMouseHandler(unsigned id)
{
    auto& manager = ext::get_singleton<InputManager>();
    auto res = manager.m_onKeyOrMouseEvents.erase(id);
    EXT_ASSERT(res == 1);
}

unsigned InputManager::AddMouseMoveHandler(OnMouseMoveCallback handler)
{
    auto& manager = ext::get_singleton<InputManager>();
    unsigned id = 0;
    auto& events = manager.m_onMouseMoveEvents;
    if (!events.empty())
        id = events.rbegin()->first + 1;
    auto res = events.try_emplace(id, std::move(handler));
    EXT_ASSERT(res.second);
    return id;
}

void InputManager::RemoveMouseMoveHandler(unsigned id)
{
    auto& manager = ext::get_singleton<InputManager>();
    auto res = manager.m_onMouseMoveEvents.erase(id);
    EXT_ASSERT(res == 1);
}

void InputManager::SendKeyOrMouse(WORD vkCode, bool isDown)
{
    if ((vkCode >= VK_LBUTTON && vkCode <= VK_XBUTTON2) || vkCode == VK_MOUSE_WHEEL || vkCode == VK_MOUSE_HWHEEL)
    {
        if (isDown)
            return MouseSendDown(vkCode);
        else
            return MouseSendUp(vkCode);
    }

    if (isDown)
        return KeyboardSendDown(vkCode);
    else
        return KeyboardSendUp(vkCode);
}

void InputManager::MouseSendDown(DWORD mouseVkCode)
{
    Send::MouseButton button;
    switch (mouseVkCode)
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
                INPUT input = CreateMouseInput(mouseVkCode, true);
                SendInput(1, &input, sizeof(INPUT));
#endif // SEND_INPUT_ON_DRIVER_FAIL
            }
            return;
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
#endif // SEND_INPUT_ON_DRIVER_FAIL
            }
            return;
        }
        break;
    default:
        EXT_ASSERT(false) << "Unknown mouse vk key " << mouseVkCode;
        return;
    }

    if (!IbSendMouseClick(button))
    {
        EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Failed to send mouse down for " << mouseVkCode;

#ifdef SEND_INPUT_ON_DRIVER_FAIL
        INPUT input = CreateMouseInput(mouseVkCode, true);
        SendInput(1, &input, sizeof(INPUT));
#endif // SEND_INPUT_ON_DRIVER_FAIL
    }
}

void InputManager::MouseSendUp(DWORD mouseVkCode)
{
    Send::MouseButton button;
    switch (mouseVkCode)
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
                INPUT input = CreateMouseInput(mouseVkCode, false);
                SendInput(1, &input, sizeof(INPUT));
#endif // SEND_INPUT_ON_DRIVER_FAIL
            }
            return;
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
#endif // SEND_INPUT_ON_DRIVER_FAIL
            }
            return;
        }
        break;
    default:
        EXT_ASSERT(false) << "Unknown mouse vk key " << mouseVkCode;
        return;
    }

    if (!IbSendMouseClick(button))
    {
        EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "Failed to send mouse down for " << mouseVkCode;

#ifdef SEND_INPUT_ON_DRIVER_FAIL
        INPUT input = CreateMouseInput(mouseVkCode, true);
        SendInput(1, &input, sizeof(INPUT));
#endif // SEND_INPUT_ON_DRIVER_FAIL
    }
}

void InputManager::MouseMove(POINT position)
{
    static const auto screenWidth = static_cast<float>(GetSystemMetrics(SM_CXVIRTUALSCREEN));
    static const auto screenHeight = static_cast<float>(GetSystemMetrics(SM_CYVIRTUALSCREEN));

    position.x = (int)roundf(float(position.x * 65536) / screenWidth);
    position.y = (int)roundf(float(position.y * 65536) / screenHeight);

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
