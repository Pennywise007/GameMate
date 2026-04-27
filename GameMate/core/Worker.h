#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <array>

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
    void updateKeyBindings();

private:
    std::atomic_int m_keyHandlingBlocked = 0;
    int m_keyMauseHandlerId = -1;
    // Active window changed hook
    HWINEVENTHOOK m_activeWindowHook = nullptr;
    // Transparent window to show top most crosshair
    process_toolkit::crosshair::AttachableCrosshairWindow m_crosshairWindow;
    // Current active window and process name, we store it just to avoid problems with getting
    // process name from GetForegroundWindow and with protected processes
    HWND m_activeWindow = nullptr;
    std::wstring m_activeProcessName;
    // Mutex for key press information
    std::recursive_mutex m_dataMutex;
    // Active window program configuration
    std::optional<process_toolkit::ProcessConfiguration> m_activeWindowConfiguration;
    // Task id of the saving settings task
    ext::TaskId m_saveSettingsTaskId;

    // Array of handlers by VK code
    std::array<std::list<std::function<bool(bool down)>>, 256> m_vkHandlers;
    ext::thread_pool m_actionExecutor;
};
