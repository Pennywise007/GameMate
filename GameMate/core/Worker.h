#pragma once

#include <chrono>
#include <optional>

#include "events.h"
#include "Crosshairs.h"
#include "Settings.h"

#include <ext/thread/thread_pool.h>
#include <ext/thread/scheduler.h>

class Worker : ext::events::ScopeSubscription<ISettingsChanged, IKeyHandlerBlocker>
{
    friend ext::Singleton<Worker>;

    Worker();
    ~Worker();
public:
    void OnForegroundChanged(HWND hWnd, const std::wstring& processName);

private: // ISettingsChanged
    void OnSettingsChanged(ISettingsChanged::ChangedType changedType) override;
    
private: // IKeyHandlerBlocker
    void OnBlockHandler() override;
    void OnUnblockHandler() override;

private:
    bool OnKeyOrMouseEvent(WORD mouseVkCode, bool down);

private:
    bool m_keyHandlingBlocked = false;
    int m_keyMauseHandlerId = -1;
    // Active window changed hook
    HWINEVENTHOOK m_activeWindowHook = nullptr;
    // Transparent window to show top most cross hair
    process_toolkit::crosshair::AttachableCrosshairWindow m_crosshairWindow;
    // we use 1 thread to put macroses in a single queue
    ext::thread_pool m_macrosExecutor = { 1 };
    ext::thread_pool m_actionExecutor = { 1 };
    // Current active window and process name, we store it just to avoid problems with getting
    // process name from GetForegroundWindow and with protected processes
    HWND m_activeWindow = nullptr;
    std::wstring m_activeProcessName;
    // Active window program configuration
    std::shared_ptr<process_toolkit::ProcessConfiguration> m_activeExeConfig;
    // Task id of the saving settings task
    ext::TaskId m_saveSettingsTaskId;
    // Time point when we ignored Windows button for active process
    std::optional<std::chrono::system_clock::time_point> m_lastWindowsIgnoreTimePoint;
};
