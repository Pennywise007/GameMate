#include "pch.h"
#include "WinRTMonitorInfo.h"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Display.h>
#include <winrt/Windows.Devices.Enumeration.h>

#include <ext/core/check.h>
#include <ext/scope/defer.h>
#include <ext/utils/com.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Display;
using namespace Windows::Devices::Enumeration;

namespace {

[[nodiscard]] std::optional<std::wstring> get_internal_display_device_id() noexcept
{
    constexpr auto* kDeviceInstanceIdKey = L"System.Devices.DeviceInstanceId";

    try
    {
        init_com(true);

        init_apartment();
        EXT_DEFER(uninit_apartment());

        auto devices = DeviceInformation::FindAllAsync(DisplayMonitor::GetDeviceSelector(), { kDeviceInstanceIdKey }).get();
        for (const auto& device : devices) {
            if (!device.Properties().HasKey(kDeviceInstanceIdKey))
                continue;

            auto deviceInstanceId = unbox_value_or<hstring>(device.Properties().Lookup(kDeviceInstanceIdKey), L"");
            if (deviceInstanceId.empty())
                continue;

            auto displayMonitor = DisplayMonitor::FromInterfaceIdAsync(device.Id()).get();
            if (!displayMonitor)
                continue;

            EXT_TRACE() << EXT_TRACE_FUNCTION
                << "DeviceInstanceId: " << deviceInstanceId.c_str() << "; "
                << "DeviceId: " << displayMonitor.DeviceId().c_str() << "; "
                << "DisplayName: " << displayMonitor.DisplayName().c_str() << "; "
                << "Internal: " << std::boolalpha << (displayMonitor.ConnectionKind() == DisplayMonitorConnectionKind::Internal);

            if (displayMonitor.ConnectionKind() == DisplayMonitorConnectionKind::Internal)
                return std::wstring(displayMonitor.DeviceId().c_str());
        }
    }
    catch (const hresult_error& e) {
        ext::ManageException((L"Failed to get internal display info from WinRT. " + e.message()).c_str());
    }
    catch (...)
    {
        ext::ManageException("Failed to get internal display info from WinRT");
    }

    return std::nullopt;
}

} // namespace

namespace winrt_monitor_info {

std::optional<std::wstring> GetInternalDisplayDeviceId() noexcept
{
    static const auto internalDisplayDeviceId = get_internal_display_device_id();
    return internalDisplayDeviceId;
}

} // namespace winrt_monitor_info
