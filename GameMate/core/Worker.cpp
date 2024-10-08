﻿#include "pch.h"
#include "psapi.h"
#include "resource.h"

#include "Crosshairs.h"
#include "DisplayBrightnessController.h"
#include "InputManager.h"
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
        // Win + Space window, it takes focus but don't return it back
    if (_tcscmp(className, _T("Shell_InputSwitchTopLevelWindow")) == 0 ||
        // Window appears when you do Win + ArrowLeft/Right and it shows you a list of other opened windows to attach to the opposite side
        _tcscmp(className, _T("XamlExplorerHostIslandWindow")) == 0)
    {
        EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << L"Ignore active window with class name: " << className;
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
        m_keyMauseHandlerId = InputManager::AddKeyOrMouseHandler(std::bind(&Worker::OnKeyOrMouseEvent, this, _1, _2));
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

    updateKeyBindings();
}

Worker::~Worker()
{
    ASSERT(m_activeWindowHook);
    UnhookWinEvent(m_activeWindowHook);

    InputManager::RemoveKeyOrMouseHandler(m_keyMauseHandlerId);
}

void Worker::OnForegroundChanged(HWND hWnd, const std::wstring& processName)
{
    m_activeWindow = hWnd;
    m_activeProcessName = processName;

    std::scoped_lock l(m_dataMutex);
    if (m_activeWindowConfiguration.has_value())
    {
        ext::get_singleton<DisplayBrightnessController>().RestoreBrightness();
        m_crosshairWindow.RemoveCrosshairWindow();

        m_activeWindowConfiguration.reset();
    }

    auto& settings = ext::get_singleton<Settings>().process_toolkit;
    if (!settings.enabled)
        return;

    for (auto& program : settings.processConfigurations)
    {
        if (program->MatchExeName(m_activeProcessName))
        {
            m_activeWindowConfiguration = *program;
            break;
        }
    }

    if (!m_activeWindowConfiguration.has_value())
        return;

    EXT_TRACE() << EXT_TRACE_FUNCTION << "active config " << m_activeWindowConfiguration->name;

    if (m_activeWindowConfiguration->changeBrightness)
        ext::get_singleton<DisplayBrightnessController>().SetBrightnessByHWND(m_activeWindow, m_activeWindowConfiguration->brightnessLevel);

    auto& crossahair = m_activeWindowConfiguration->crosshairSettings;
    if (crossahair.show)
        m_crosshairWindow.AttachCrosshairToWindow(crossahair, m_activeWindow);
}

bool Worker::OnKeyOrMouseEvent(WORD vkCode, bool down)
{
    if (m_keyHandlingBlocked)
        return false;

    std::scoped_lock l(m_dataMutex);
    // Execute program callbacks
    for (auto&& [key, callback] : m_keyBindingsCallbacks)
    {
        if (key.IsPressed(vkCode, down))
        {
            ext::InvokeMethodAsync([&callback]() {
                callback();
            });
            return false;
        }
    }

    if (!m_activeWindowConfiguration.has_value())
        return false;

    // Ignore accidental press
    for (auto& key : m_activeWindowConfiguration->keysToIgnoreAccidentalPress)
    {
        if (!key.IsPressed(vkCode, down))
            continue;

        auto now = std::chrono::system_clock::now();
        if (key.lastTimeWhenKeyWasIgnored.has_value() &&
            (now - *key.lastTimeWhenKeyWasIgnored) <= std::chrono::seconds(1))
        {
            break;
        }

        if (down)
            key.lastTimeWhenKeyWasIgnored = std::move(now);

        return true;
    }

    // Key remapping
    for (auto&& [keyToReplace, replacingKey] : m_activeWindowConfiguration->keysRemapping)
    {
        if (!keyToReplace.IsPressed(vkCode, down))
            continue;

        Action::NewAction(replacingKey.vkCode, down, 0).ExecuteAction(0);
        return true;
    }

    // Execute binds commands
    for (auto&& [bind, actions] : m_activeWindowConfiguration->actionsByBind)
    {
        if (bind.IsPressed(vkCode, down))
        {
            m_macrosExecutor.add_task([](Actions actions) { actions.Execute(); }, actions);
            return true;
        }
    }

    return false;
}

void Worker::updateKeyBindings()
{
    auto& settings = ext::get_singleton<Settings>();
    decltype(m_keyBindingsCallbacks) bindings = {
        {
            settings.process_toolkit.enableBind,
            []() {
                auto& settings = ext::get_singleton<Settings>();
                settings.process_toolkit.enabled = !settings.process_toolkit.enabled;
                ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
            }
        },
        {
            settings.actions_executor.enableBind,
            []() {
                auto& settings = ext::get_singleton<Settings>();
                settings.actions_executor.enabled = !settings.actions_executor.enabled;
                ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eActionsExecutorEnableChanged);
            }
        },
        {
            settings.timer.showTimerBind,
            []() { ext::send_event(&ITimerNotifications::OnShowHideTimer); }
        },
        {
            settings.timer.startPauseTimerBind,
            []() { ext::send_event(&ITimerNotifications::OnStartOrPauseTimer); }
        },
        {
            settings.timer.resetTimerBind,
            []() { ext::send_event(&ITimerNotifications::OnResetTimer); }
        },
    };

    std::scoped_lock l(m_dataMutex);
    m_keyBindingsCallbacks = std::move(bindings);
}

void Worker::OnSettingsChanged(ISettingsChanged::ChangedType changedType)
{
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

    // There might be change in our binds, update them
    switch (changedType)
    {
    case ISettingsChanged::ChangedType::eProcessToolkit:
    case ISettingsChanged::ChangedType::eActionsExecutor:
    case ISettingsChanged::ChangedType::eTimer:
        updateKeyBindings();
        break;
    }

    switch (changedType)
    {
    case ISettingsChanged::ChangedType::eProcessToolkit:
        // Force crosshairs and m_activeExeConfig to apply new changes
        OnForegroundChanged(m_activeWindow, m_activeProcessName);
        break;
    case ISettingsChanged::ChangedType::eActionsExecutorEnableChanged:
        {
            auto& settings = ext::get_singleton<Settings>().actions_executor;
            if (settings.enabled)
            {
                EXT_ASSERT(m_actionExecutor.running_tasks_count() == 0);
                m_actionExecutor.add_task([](actions_executor::Settings actionsExecutor) -> void {
                    actionsExecutor.Execute();

                    if (!ext::this_thread::interruption_requested())
                        ext::InvokeMethodAsync([]() {
                            // Changing UI enable button state
                            ext::get_singleton<Settings>().actions_executor.enabled = false;
                            ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eActionsExecutorEnableChanged);
                        });
                }, settings);
            }
            else
            {
                m_actionExecutor.interrupt_and_remove_all_tasks();
            }
        }
        break;
    }
}

void Worker::OnBlockHandler()
{
    m_keyHandlingBlocked = true;
}

void Worker::OnUnblockHandler()
{
    m_keyHandlingBlocked = false;
}
