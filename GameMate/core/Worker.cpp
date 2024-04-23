#include "pch.h"
#include "psapi.h"
#include "resource.h"

#include "Crosshairs.h"
#include "Worker.h"

#include <ext/thread/thread_pool.h>

namespace {

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    return ext::get_service<Worker>().OnMouseProc(nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    return ext::get_service<Worker>().OnKeyboardProc(nCode, wParam, lParam);
}

void CALLBACK windowFocusChanged(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    ext::get_service<Worker>().OnFocusChanged(hWnd);
}

} // namespace

Worker::Worker()
{
    ext::get_tracer().Enable();

    m_activeWindowHook = SetWinEventHook(EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, NULL, &windowFocusChanged, 0, 0, WINEVENT_OUTOFCONTEXT);
    if (m_activeWindowHook == nullptr) {
        MessageBox(0, L"Failed to set active window hook", L"Failed to start program", MB_OK | MB_ICONERROR);
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

    OnSettingsChangedByUser();
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

    EXT_TRACE_DBG() << L"Active process: " << activeWindowProcessname;

    if (m_activeExeTabConfig)
    {
        m_crosshairWindow.ShowWindow(SW_HIDE);
        m_activeExeTabConfig = nullptr;
    }

    //if (m_activeProcessName == activeProcessName)
     //   return;

    // TODO don't allow exe with same names in UI

    auto& settings = ext::get_service<Settings>();
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

    auto& crossahair = m_activeExeTabConfig->crosshairSettings;
    if (crossahair.show)
    {
        auto currentActiveWindow = GetForegroundWindow();
        if (currentActiveWindow == NULL)
            currentActiveWindow = hWnd;
        m_crosshairWindow.SetCrosshair(currentActiveWindow, crossahair);
    }
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
                    m_macrosExecutors.add_task([actions = macro.actions]() {
                        for (const auto& action : actions)
                        {
                            action.ExecuteAction();
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
                    m_macrosExecutors.add_task([actions = macro.actions]() {
                        for (const auto& action : actions)
                        {
                            action.ExecuteAction();
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
                // TODO allow double win press
                return 1;
            default:
                break;
            }
        }
    }

    return CallNextHookEx(m_keyboardHook, nCode, wParam, lParam);
}

void Worker::OnSettingsChangedByUser()
{
    OnFocusChanged(GetForegroundWindow());
}
