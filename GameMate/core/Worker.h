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
    void OnFocusChanged(HWND hWnd);
    LRESULT OnMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    LRESULT OnKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

private: // ISettingsChanged
    void OnSettingsChanged() override;

private:
    HWINEVENTHOOK m_activeWindowHook = nullptr;
    HHOOK m_mouseHook = nullptr;
    HHOOK m_keyboardHook = nullptr;

    crosshair::CrosshairWindow m_crosshairWindow;

    // we use 1 thread to put macros in a single queue
    ext::thread_pool m_macrosExecutor = { 1 };
    std::shared_ptr<TabConfiguration> m_activeExeTabConfig;
    ext::TaskId m_saveSettingsTaskId;

    std::optional<std::chrono::system_clock::time_point> m_lastWindowsIgnoreTimePoint;
};
