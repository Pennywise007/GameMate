#include "pch.h"
#include "psapi.h"
#include "resource.h"

#include "Crosshairs.h"
#include "Worker.h"

#include <ext/thread/invoker.h>

namespace {

void GetProcessName(DWORD dwProcessId, std::wstring& processName)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);
    if (hProcess != nullptr)
    {
        TCHAR processPath[MAX_PATH * 10];
        if (GetModuleFileNameEx(hProcess, nullptr, processPath, MAX_PATH * 10) != 0)
            processName = PathFindFileName(processPath);
        else
            EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << L"Failed to get module file name from process, err code: " << GetLastError();
        CloseHandle(hProcess);
    }
    else
        EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << L"Failed to open process, err code: " << GetLastError();
}

bool GetProcessName(HWND hWnd, std::wstring& processName)
{
    if (TCHAR processPath[MAX_PATH * 10]; GetWindowModuleFileName(hWnd, processPath, MAX_PATH * 10) != 0)
        processName = PathFindFileName(processPath);
    else
    {
        // Getting process name failed, probably because of the access restrictions or some window protection
        // Trying to get window process id, which will most likely also fail
        DWORD dwProcessId;
        if (GetWindowThreadProcessId(hWnd, &dwProcessId) == 0)
            return false;

        GetProcessName(dwProcessId, processName);
    }

    return true;
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    return ext::get_singleton<Worker>().OnMouseProc(nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    return ext::get_singleton<Worker>().OnKeyboardProc(nCode, wParam, lParam);
}

void CALLBACK WindowForegroundChangedProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    TCHAR className[MAX_PATH];
    GetClassName(hWnd, className, MAX_PATH);
    // Ignores specific windows
    if (_tcscmp(className, _T("Shell_InputSwitchTopLevelWindow")) == 0) { // Win + Space window, it takes focus but don't return it back
        return;
    }

    std::wstring processName;
    if (!GetProcessName(hWnd, processName))
    {
        // We get the process id from the thread id
        HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, dwEventThread);
        if (hThread != NULL)
        {
            DWORD dwProcessId = GetProcessIdOfThread(hThread);
            if (dwProcessId != 0)
                GetProcessName(dwProcessId, processName);
            else
                EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << L"Failed to get process id from thread id, err code: " << GetLastError();

            CloseHandle(hThread);
        }
        else
            EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << L"Failed to get process id from hwnd and event thread id, err code: " << GetLastError();
    }

    EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << L"Active window change, process name: " << processName << L", class name: " << className;

    ext::get_singleton<Worker>().OnForegroundChanged(hWnd, processName);
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

    auto currentActiveWindow = GetForegroundWindow();

    std::wstring currentActiveProcessName;
    EXT_DUMP_IF(!GetProcessName(currentActiveWindow, currentActiveProcessName)) << "Failed to get active window process name," <<
        " most likely active window is our process and we should be able to get it's name";
    OnForegroundChanged(currentActiveWindow, currentActiveProcessName);
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

void Worker::OnForegroundChanged(HWND hWnd, const std::wstring& processName)
{
    m_activeWindow = hWnd;
    m_activeProcessName = processName;

    if (m_activeExeTabConfig)
    {
        m_crosshairWindow.RemoveCrosshairWindow();
        m_activeExeTabConfig = nullptr;
    }

    auto& settings = ext::get_singleton<Settings>();
    if (!settings.programWorking)
        return;

    for (auto& tab : settings.tabs)
    {
        if (tab->exeName == m_activeProcessName)
        {
            if (tab->enabled)
                m_activeExeTabConfig = tab;
            break;
        }
    }

    // add game mode detection
    if (!m_activeExeTabConfig)
        return;

    m_lastWindowsIgnoreTimePoint.reset();

    auto& crossahair = m_activeExeTabConfig->crosshairSettings;
    if (crossahair.show)
        m_crosshairWindow.AttachCrosshairToWindow(m_activeWindow, crossahair);
}

LRESULT Worker::OnMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && !!m_activeExeTabConfig)
    {
        PMSLLHOOKSTRUCT hookStruct = (PMSLLHOOKSTRUCT)lParam;
        for (auto&& [bind, macro] : m_activeExeTabConfig->macrosByBind)
        {
            if (bind.IsBind(UINT(wParam), wParam))
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
            if (bind.IsBind(UINT(wParam), hookStruct->vkCode))
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
    // Force crosshairs and m_activeExeTabConfig to apply new changes
    OnForegroundChanged(m_activeWindow, m_activeProcessName);

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
