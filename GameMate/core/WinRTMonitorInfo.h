#pragma once

#include <optional>
#include <string>

namespace winrt_monitor_info {

// Getting internal display instance id(if exists)
// We use WinRT because it looks like WinAPI doesn't have any working way to get information about internal display
[[nodiscard]] std::optional<std::wstring> GetInternalDisplayDeviceId() noexcept;

} // namespace winrt_monitor_info
