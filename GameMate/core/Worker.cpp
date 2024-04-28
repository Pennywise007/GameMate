#include "pch.h"
#include "psapi.h"
#include "resource.h"

#include "Crosshairs.h"
#include "Worker.h"

#include <ext/thread/invoker.h>

namespace {
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    return ext::get_singleton<Worker>().OnMouseProc(nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    return ext::get_singleton<Worker>().OnKeyboardProc(nCode, wParam, lParam);
}

void CALLBACK WindowForegroundChangedProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    TCHAR className[MAX_PATH];
    GetClassName(hWnd, className, MAX_PATH);
    // Ignores specific windows
    if (_tcscmp(className, _T("Shell_InputSwitchTopLevelWindow")) == 0) { // Win + Space window, it takes focus but don't return it back
        return;
    }
    EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << className;

    ext::get_singleton<Worker>().OnFocusChanged(hWnd);
}
} // namespace

Worker::Worker()
{
    m_activeWindowHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, &WindowForegroundChangedProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    if (m_activeWindowHook == nullptr) {
        MessageBox(0, L"Failed to set system foreground changed hook", L"Failed to start program", MB_OK | MB_ICONERROR);
        ExitProcess(400);
    }

    m_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
    if (m_mouseHook == nullptr) {
        UnhookWinEvent(m_activeWindowHook);
        MessageBox(0, L"Failed to set mouse hook", L"Failed to start program", MB_OK | MB_ICONERROR);
        ExitProcess(401);
    }

    m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (m_keyboardHook == nullptr) {
        UnhookWinEvent(m_activeWindowHook);
        UnhookWindowsHookEx(m_mouseHook);
        MessageBox(0, L"Failed to set keyboard hook", L"Failed to start program", MB_OK | MB_ICONERROR);
        ExitProcess(402);
    }

    OnFocusChanged(GetForegroundWindow());
}

Worker::~Worker()
{
    ASSERT(m_activeWindowHook);
    UnhookWinEvent(m_activeWindowHook);
    ASSERT(m_mouseHook);
    UnhookWindowsHookEx(m_mouseHook);
    ASSERT(m_keyboardHook);
    UnhookWindowsHookEx(m_keyboardHook);
}

void Worker::OnFocusChanged(HWND hWnd)
{
    auto& settings = ext::get_singleton<Settings>();
    if (!settings.programWorking)
        return;

    std::wstring activeWindowProcessname;

    DWORD pid;
    GetWindowThreadProcessId(hWnd, &pid);

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == nullptr)
        return;

    TCHAR processName[MAX_PATH * 10];
    GetModuleFileNameEx(hProcess, nullptr, processName, MAX_PATH * 10);
    CloseHandle(hProcess);

    activeWindowProcessname = PathFindFileName(processName);

    EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << L"Active process: " << activeWindowProcessname;

    if (m_activeExeTabConfig)
    {
        m_crosshairWindow.RemoveCrosshairWindow();
        m_activeExeTabConfig = nullptr;
    }

    for (auto& tab : settings.tabs)
    {
        if (tab->exeName == activeWindowProcessname)
        {
            m_activeExeTabConfig = tab;
            break;
        }
    }

    // add game mode detection
    if (!m_activeExeTabConfig || !m_activeExeTabConfig->enabled)
        return;

    m_lastWindowsIgnoreTimePoint.reset();

    auto& crossahair = m_activeExeTabConfig->crosshairSettings;
    if (crossahair.show)
        m_crosshairWindow.AttachCrosshairToWindow(hWnd, crossahair);
}

LRESULT Worker::OnMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && !!m_activeExeTabConfig)
    {
        PMSLLHOOKSTRUCT hookStruct = (PMSLLHOOKSTRUCT)lParam;
        for (auto&& [bind, macro] : m_activeExeTabConfig->macrosByBind)
        {
            if (bind.IsBind(wParam, wParam))
            {
                // Execute macros on key down and ignore key up
                switch (wParam)
                {
                case WM_LBUTTONUP:
                case WM_RBUTTONUP:
                case WM_MBUTTONUP:
                case WM_XBUTTONUP:
                    break;
                default:
                    m_macrosExecutor.add_task([actions = macro.actions, delayRandomize = macro.randomizeDelays]() {
                        for (const auto& action : actions)
                        {
                            action.ExecuteAction(delayRandomize);
                        }
                    });
                    break;
                }

                return 1;
            }
        }
    }

    return CallNextHookEx(m_mouseHook, nCode, wParam, lParam);
}

LRESULT Worker::OnKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && GetKeyState(VK_SHIFT) & 0x8000 && wParam == WM_KEYUP)
    {
        // Check Shift+F9 program working combination
        PKBDLLHOOKSTRUCT hookStruct = (PKBDLLHOOKSTRUCT)lParam;
        if (hookStruct->vkCode == VK_F9)
        {
            auto& settings = ext::get_singleton<Settings>();
            settings.programWorking = !settings.programWorking;
            ext::send_event(&ISettingsChanged::OnSettingsChanged);

            return CallNextHookEx(m_keyboardHook, nCode, wParam, lParam);
        }
    }

    if (nCode == HC_ACTION && !!m_activeExeTabConfig)
    {
        PKBDLLHOOKSTRUCT hookStruct = (PKBDLLHOOKSTRUCT)lParam;
        for (auto&& [bind, macro] : m_activeExeTabConfig->macrosByBind)
        {
            if (bind.IsBind(wParam, hookStruct->vkCode))
            {
                // Execute macros on key down and ignore key up
                if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
                {
                    m_macrosExecutor.add_task([actions = macro.actions, delayRandomize = macro.randomizeDelays]() {
                        for (const auto& action : actions)
                        {
                            action.ExecuteAction(delayRandomize);
                        }
                    });
                }

                return 1;
            }
        }

        // Ignore Windows button press
        if (m_activeExeTabConfig->disableWinButton)
        {
            switch (hookStruct->vkCode)
            {
            case VK_LWIN:
            case VK_RWIN:
                {
                    auto now = std::chrono::system_clock::now();
                    if (m_lastWindowsIgnoreTimePoint.has_value() &&
                        (now - *m_lastWindowsIgnoreTimePoint) <= std::chrono::seconds(1))
                        break;

                    if (wParam == WM_KEYUP)
                        m_lastWindowsIgnoreTimePoint = std::move(now);
                }
                return 1;
            default:
                break;
            }
        }
    }

    return CallNextHookEx(m_keyboardHook, nCode, wParam, lParam);
}

void Worker::OnSettingsChanged()
{
    if (!ext::get_singleton<Settings>().programWorking)
    {
        if (m_activeExeTabConfig)
        {
            m_crosshairWindow.RemoveCrosshairWindow();
            m_activeExeTabConfig = nullptr;
        }
        return;
    }

    OnFocusChanged(GetForegroundWindow());
 
    // Save settings every 5 seconds after settings changed
    auto& scheduller = ext::Scheduler::GlobalInstance();
    if (m_saveSettingsTaskId != ext::kInvalidId)
        scheduller.RemoveTask(m_saveSettingsTaskId);
    m_saveSettingsTaskId = scheduller.SubscribeTaskAtTime([]()
    {
        ext::InvokeMethodAsync([]() {
            ext::get_singleton<Settings>().SaveSettings();
        });
    }, std::chrono::system_clock::now() + std::chrono::seconds(5));
}
