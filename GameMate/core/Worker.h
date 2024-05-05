#pragma once

#include <chrono>
#include <optional>

#include "Crosshairs.h"
#include "Settings.h"

#include <ext/thread/thread_pool.h>
#include <ext/thread/scheduler.h>

class Worker : ext::events::ScopeSubscription<ISettingsChanged>
{
    friend ext::Singleton<Worker>;

    Worker();
    ~Worker();
public:
    void OnForegroundChanged(HWND hWnd, const std::wstring& processName);

private: // ISettingsChanged
    void OnSettingsChanged() override;

private:
    bool OnKeyOrMouseEvent(WORD mouseVkKey, bool down);

private:
    // Active window changed hook
    HWINEVENTHOOK m_activeWindowHook = nullptr;
    // Transparent window to show top most crosshair
    crosshair::CrosshairWindow m_crosshairWindow;
    // we use 1 thread to put macroses in a single queue
    ext::thread_pool m_macrosExecutor = { 1 };
    // Current active window and process name, we store it just to avoid problems with getting
    // process name from GetForegroundWindow and with protected processes
    HWND m_activeWindow = nullptr;
    std::wstring m_activeProcessName;
    // Active window program configuration
    std::shared_ptr<TabConfiguration> m_activeExeTabConfig;
    // Task id of the saving settings task
    ext::TaskId m_saveSettingsTaskId;
    // Time point when we ignored Windows button for active process
    std::optional<std::chrono::system_clock::time_point> m_lastWindowsIgnoreTimePoint;
};
