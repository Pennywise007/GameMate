#pragma once

#include <Windows.h>
#include <iostream>
#include <optional>
#include <physicalmonitorenumerationapi.h>
#include <stdint.h>
#include <vector>

#include <ext/core/singleton.h>

class DisplayBrightnessController {
    friend ext::Singleton<DisplayBrightnessController>;
public:
    // Check if we can control brightness at least for 1 monitor
    static [[nodiscard]] bool BrightnessControlAvailable() noexcept;

    // Change display brightness on which hwnd is displayed
    bool SetBrightnessByHWND(HWND hWnd, uint8_t desiredBrightness) noexcept;
    // Restore brightness if it was set
    void RestoreBrightness() noexcept;
    // Notification about window position changed
    void OnWindowPosChanged(HWND hWnd);
private:
    // physical monitors associated with current monitor
    struct PhysicalMonitors : std::vector<PHYSICAL_MONITOR>
    {
        PhysicalMonitors(HMONITOR hMonitor);
        ~PhysicalMonitors() noexcept;

        // Check if monitor matches current physical monitors
        [[nodiscard]] bool IsSameMonitor(HMONITOR hMonitor) const noexcept;
        // Set brightness to all physical monitors, true if we changed at least 1 monitor brightness
        bool SetMonitorsBrightness(uint8_t brightness) noexcept;
        // Check if any of the physical monitors associated with monitor supports brightness control
        [[nodiscard]] static bool CanControlBrightness(HMONITOR hMonitor) noexcept;

    private:
        static size_t findInternalDisplayIndex(HMONITOR hMonitor, size_t physicalMonitorsCount) noexcept;
        void saveOriginalBrightness() noexcept;
        void restoreOriginalBrightness();

    private:
        // Monitor which we control now
        const HMONITOR m_monitor;
        // Array of internal displays
        const size_t m_internalDisplayIndex;
        // We save original monitor brightness to be able to restore it when we start control new monitor
        std::vector<std::optional<uint8_t>> m_originalMonitorBrightness;
    };
    // Hook for monitor window position changes
    HWINEVENTHOOK m_windowPosChangedHook = nullptr;
    HWND m_controlledHwnd = nullptr;
    // Brightness which was requested last time
    uint8_t m_installedBrightness = -1;
    // Current controllable monitors
    std::optional<PhysicalMonitors> m_currentMonitors;

    // Controlling brightness with WinAPI functions, used for HDMI monitors(if supported)
    struct WinAPIBrightnessController
    {
        [[nodiscard]] static uint8_t GetBrightness(const PHYSICAL_MONITOR& physicalMonitor);
        static void SetBrightness(const PHYSICAL_MONITOR& physicalMonitor, uint8_t brightness);
    };
    // Controlling build-in monitor with WMI command
    struct WMIBrightnessController
    {
        [[nodiscard]] static uint8_t GetBrightness();
        static void SetBrightness(uint8_t brightness);
    };
};
