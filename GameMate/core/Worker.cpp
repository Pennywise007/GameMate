#include "pch.h"
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
    : m_actionExecutor(std::thread::hardware_concurrency() <= 2 ? 1 : std::thread::hardware_concurrency() - 2)
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
    m_actionExecutor.interrupt_and_remove_all_tasks();

    ASSERT(m_activeWindowHook);
    UnhookWinEvent(m_activeWindowHook);

    InputManager::RemoveKeyOrMouseHandler(m_keyMauseHandlerId);
}

void Worker::OnForegroundChanged(HWND hWnd, const std::wstring& processName)
{
    m_activeWindow = hWnd;
    m_activeProcessName = processName;

    std::scoped_lock l(m_dataMutex);
    const bool oldConfigExisted = m_activeWindowConfiguration.has_value();
    if (oldConfigExisted)
    {
        EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << L"Resetting previous active window configuration: " << m_activeWindowConfiguration->name;

        ext::get_singleton<DisplayBrightnessController>().RestoreBrightness();
        m_crosshairWindow.RemoveCrosshairWindow();

        m_activeWindowConfiguration.reset();
    }

    EXT_DEFER(
        if (oldConfigExisted || m_activeWindowConfiguration.has_value())
            updateKeyBindings();
    );

    auto& settings = ext::get_singleton<Settings>().process_toolkit;
    if (!settings.enabled)
        return;

    for (auto& program : settings.processConfigurations)
    {
        if (program->enabled && program->MatchExeName(m_activeProcessName))
        {
            m_activeWindowConfiguration = *program;
            break;
        }
    }

    if (!m_activeWindowConfiguration.has_value())
        return;

    EXT_TRACE() << EXT_TRACE_FUNCTION << "Active config " << m_activeWindowConfiguration->name;

    if (m_activeWindowConfiguration->changeBrightness)
        ext::get_singleton<DisplayBrightnessController>().SetBrightnessByHWND(m_activeWindow, m_activeWindowConfiguration->brightnessLevel);

    auto& crossahair = m_activeWindowConfiguration->crosshairSettings;
    if (crossahair.show)
        m_crosshairWindow.AttachCrosshairToWindow(crossahair, m_activeWindow);
}

bool Worker::OnKeyOrMouseEvent(WORD vkCode, bool down)
{
    if (m_keyHandlingBlocked != 0)
        return false;

    std::scoped_lock l(m_dataMutex);

    if (vkCode > m_vkHandlers.size())
    {
        EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << L"vkCode is bigger than handlers size, vkCode: " << vkCode << L", handlers size: " << m_vkHandlers.size();
        return false;
    }

    for (const auto& handler : m_vkHandlers[vkCode])
    {
        if (handler(down))
            return true;
    }

    return false;
}

void Worker::updateKeyBindings()
{
    std::scoped_lock l(m_dataMutex);

    auto& settings = ext::get_singleton<Settings>();

    // Callbacks for each key bind
    std::map<Bind, std::function<void()>> keyBindingsCallbacks = {
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

    m_vkHandlers.fill({});

    // first add handlers for binds, so they will have higher priority
    for (auto&& [key, callback] : keyBindingsCallbacks)
    {
        m_vkHandlers[key.vkCode].emplace_back([input = key, callback = callback](bool down)
        {
            if (!input.IsPressed(input.vkCode, down))
                return false;

            ext::InvokeMethodAsync([&callback]() {
                callback();
            });
            EXT_TRACE() << EXT_TRACE_FUNCTION << "keyBindingsCallbacks " << input.vkCode;
            return true;
        });
    }

    if (!m_activeWindowConfiguration.has_value())
    {
        EXT_TRACE() << EXT_TRACE_FUNCTION << "No active window";
        return;
    }

    // Ignore accidental press
    for (auto& key : m_activeWindowConfiguration->keysToIgnoreAccidentalPress)
    {
        m_vkHandlers[key.vkCode].emplace_back(
            [input = key, lastTimeWhenKeyWasIgnored = std::optional<std::chrono::system_clock::time_point>{}](bool down) mutable {
                if (!input.IsPressed(input.vkCode, down))
                    return false;

                auto now = std::chrono::system_clock::now();
                if (lastTimeWhenKeyWasIgnored.has_value() &&
                    (now - *lastTimeWhenKeyWasIgnored) <= std::chrono::seconds(1))
                {
                    return false;
                }

                if (down)
                    lastTimeWhenKeyWasIgnored = std::move(now);

                EXT_TRACE() << EXT_TRACE_FUNCTION << "keysToIgnoreAccidentalPress " << input.vkCode;
                return true;
            });
    }

    // Key remapping
    for (auto&& [keyToReplace, replacingKey] : m_activeWindowConfiguration->keysRemapping)
    {
        m_vkHandlers[keyToReplace.vkCode].emplace_back([input = keyToReplace, replacingKey](bool down) {
            if (!input.IsPressed(input.vkCode, down))
                return false;

            Action::NewAction(replacingKey.vkCode, down, 0).ExecuteAction(0);
            EXT_TRACE() << EXT_TRACE_FUNCTION << "keysRemapping " << input.vkCode;
            return true;
        });
    }

    // Execute binds commands
    for (auto&& [bind, actions] : m_activeWindowConfiguration->actionsByBind)
    {
        if (bind.whileHold)
        {
            m_vkHandlers[bind.vkCode].emplace_back(
                [
                    input = bind,
                    bindActions = actions,
                    actionExecutor = &m_actionExecutor,
                    taskId = std::optional<ext::thread_pool::TaskId>{}
                ](bool down) mutable {

                    const bool pressed = input.IsPressed(input.vkCode, down);
                    if (!pressed)
                    {
                        EXT_TRACE() << EXT_TRACE_FUNCTION << "Not pressed: " << input.vkCode;
                        if (taskId.has_value())
                        {
                            EXT_TRACE() << EXT_TRACE_FUNCTION << "Interrupting while hold bind actions, vkCode: " << input.vkCode;
                            // Key was released but it was hold bind, we need to stop it's execution
                            actionExecutor->stop_and_remove_task(taskId.value());
                            taskId.reset();
                            return true;
                        }
                        EXT_TRACE() << "No task: " << input.vkCode;
                        return false;
                    }

                    if (pressed && !taskId.has_value())
                    {
                        EXT_TRACE() << EXT_TRACE_FUNCTION << "Starting while hold bind actions, vkCode: " << input.vkCode;
                        taskId.emplace(actionExecutor->add_task([](Actions actions) {
                            auto stopToken = ext::this_thread::get_stop_token();
                            do
                            {
                                actions.Execute(stopToken);
                            } while (!stopToken.stop_requested());
                        }, bindActions).first);
                    }

                    EXT_TRACE() << EXT_TRACE_FUNCTION << "actionsByBind " << input.vkCode;
                    return true;
                });
        }
        else
        {
            m_vkHandlers[bind.vkCode].emplace_back(
                [
                    input = bind,
                    bindActions = actions,
                    actionExecutor = &m_actionExecutor
                ](bool down) {
                    if (!input.IsPressed(input.vkCode, down))
                        return false;

                    actionExecutor->add_task([](Actions actions) {
                        actions.Execute(ext::this_thread::get_stop_token());
                    }, bindActions);

                    EXT_TRACE() << EXT_TRACE_FUNCTION << "actionsByBind " << input.vkCode;
                    return true;
                });
        }
    }
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
    ++m_keyHandlingBlocked;
}

void Worker::OnUnblockHandler()
{
    --m_keyHandlingBlocked;
}
