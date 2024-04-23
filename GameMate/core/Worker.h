#pragma once

#include "Crosshairs.h"
#include "Settings.h"

#include <ext/thread/thread_pool.h>

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
    void OnSettingsChangedByUser() override;

private:
    HWINEVENTHOOK m_activeWindowHook = nullptr;
    HHOOK m_mouseHook = nullptr;
    HHOOK m_keyboardHook = nullptr;

    ext::thread_pool m_macrosExecutors;
    std::shared_ptr<TabConfiguration> m_activeExeTabConfig;
    crosshair::CrosshairWindow m_crosshairWindow;
};
