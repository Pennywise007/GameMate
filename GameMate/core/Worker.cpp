#include "pch.h"
#include "psapi.h"
#include "resource.h"

#include "InputManager.h"
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
        ExitProcess(500);
    }

    try
    {
        using namespace std::placeholders;
        const int id = InputManager::AddKeyOrMouseHandler(std::bind(&Worker::OnKeyOrMouseEvent, this, _1, _2));
        EXT_ASSERT(id == 0) << EXT_TRACE_FUNCTION << "We should be the first subscriber, if not change call of the RemoveKeyOrMouseHandler";
    }
    catch (...)
    {
        UnhookWinEvent(m_activeWindowHook);
        MessageBox(0, ext::ManageExceptionText(L"").c_str(), L"Failed to start program", MB_OK | MB_ICONERROR);
        ExitProcess(501);
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

    InputManager::RemoveKeyOrMouseHandler(0);
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

bool Worker::OnKeyOrMouseEvent(WORD vkCode, bool down)
{
    // Check if program working mode switched
    if (vkCode == VK_F9 && !down && InputManager::GetKeyState(VK_SHIFT))
    {
        auto& settings = ext::get_singleton<Settings>();
        settings.programWorking = !settings.programWorking;
        ext::send_event(&ISettingsChanged::OnSettingsChanged);

        return false;
    }

    if (!!m_activeExeTabConfig)
    {
        for (auto&& [bind, action] : m_activeExeTabConfig->actionsByBind)
        {
            if (bind.IsBindPressed(vkCode))
            {
                // Execute macros on key down and ignore key up
                if (down)
                {
                    m_macrosExecutor.add_task([actions = action.actions, delayRandomize = action.randomizeDelays]() {
                        for (const auto& action : actions)
                        {
                            action.ExecuteAction(delayRandomize, false);
                        }
                    });
                }

                return true;
            }
        }

        // Ignore Windows button press
        if (m_activeExeTabConfig->disableWinButton)
        {
            switch (vkCode)
            {
            case VK_LWIN:
            case VK_RWIN:
                {
                    auto now = std::chrono::system_clock::now();
                    if (m_lastWindowsIgnoreTimePoint.has_value() &&
                        (now - *m_lastWindowsIgnoreTimePoint) <= std::chrono::seconds(1))
                        break;

                    if (!down)
                        m_lastWindowsIgnoreTimePoint = std::move(now);
                }
                return true;
            default:
                break;
            }
        }
    }

    return false;
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
