#include "pch.h"
#include "DisplayBrightnessController.h"
#include "WinRTMonitorInfo.h"

#include <highlevelmonitorconfigurationapi.h>
#include <wbemidl.h>
#include <unordered_map>

#include <ext/core/check.h>
#include <ext/scope/defer.h>
#include <ext/utils/com.h>

#pragma comment(lib, "Dxva2.lib")
#pragma comment(lib, "wbemuuid.lib")

namespace {

[[nodiscard]] std::string get_last_error_as_string() noexcept
{
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) {
        return std::string(); //No error message has been recorded
    }

    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);

    return message;
}

[[nodiscard]] std::vector<PHYSICAL_MONITOR> get_physical_monitors(HMONITOR hMonitor)
{
    // Return INVALID_HANDLE_VALUE if the monitor handle is invalid
    EXT_CHECK(hMonitor && hMonitor != INVALID_HANDLE_VALUE) << "Invalid monitor handle";

    DWORD numberOfPhysicalMonitors{ 0 };
    // Get the number of physical monitors and allocate memory for the physical monitors
    EXT_CHECK(GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &numberOfPhysicalMonitors))
        << "Failed to get physical monitors count, err " << get_last_error_as_string();

    std::vector<PHYSICAL_MONITOR> res(numberOfPhysicalMonitors);
    // Get the physical monitors from the monitor handle
    EXT_CHECK(GetPhysicalMonitorsFromHMONITOR(hMonitor, numberOfPhysicalMonitors, res.data()))
        << "Failed to get physical monitors, err " << get_last_error_as_string();
    return res;
}

void verify_result(HRESULT result, const char* errorText)
{
    if (result == WBEM_S_NO_ERROR)
        return;

    _com_error err(result);
    EXT_CHECK(false) << errorText << ", error message: " << err.ErrorMessage();
}

[[nodiscard]] std::unordered_map<std::wstring, std::wstring> device_id_by_device_name() noexcept
{
    std::unordered_map<std::wstring, std::wstring> res;

    DISPLAY_DEVICEW displayDevice;
    displayDevice.cb = sizeof(displayDevice);
    for (DWORD displayIndex = 0; EnumDisplayDevicesW(0, displayIndex, &displayDevice, 0); ++displayIndex) {
        std::wstring deviceName = displayDevice.DeviceName;
        for (DWORD monitorIndexForDisplay = 0;
            EnumDisplayDevicesW(deviceName.c_str(), monitorIndexForDisplay, &displayDevice, EDD_GET_DEVICE_INTERFACE_NAME);
            ++monitorIndexForDisplay)
        {
            EXT_TRACE() << EXT_TRACE_FUNCTION
                << "DeviceName: " << displayDevice.DeviceName << "; "
                << "DeviceID: " << displayDevice.DeviceID;

            res.emplace(std::make_pair(displayDevice.DeviceName, displayDevice.DeviceID));
        }
    }

    return res;
}

void CALLBACK WinPosChangedProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    if (idObject != OBJID_WINDOW)
        return;

    ext::get_singleton<DisplayBrightnessController>().OnWindowPosChanged(hwnd);
}

} // namespace

bool DisplayBrightnessController::BrightnessControlAvailable() noexcept
{
    bool brightnessControlAvailable = !EnumDisplayMonitors(NULL, NULL, [](HMONITOR hMonitor, HDC /*hdcMonitor*/, LPRECT /*lprcMonitor*/, LPARAM /*dwData*/) -> BOOL {
        return !PhysicalMonitors::CanControlBrightness(hMonitor);
    }, 0);

    EXT_TRACE() << "Brightness control available = " << std::boolalpha << brightnessControlAvailable;
    return brightnessControlAvailable;
}

bool DisplayBrightnessController::SetBrightnessByHWND(HWND hWnd, uint8_t desiredBrightness) noexcept
{
    // Get the handle of the monitor from the window
    HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    EXT_ASSERT(hMonitor);

    bool newMonitor = !m_currentMonitors.has_value() || !m_currentMonitors->IsSameMonitor(hMonitor);
    if (newMonitor)
    {
        m_currentMonitors.reset();

        try
        {
            m_currentMonitors.emplace(hMonitor);
        }
        catch (...)
        {
            ext::ManageException("Failed to init physical monitors controller");
            return false;
        }
    }
    EXT_ASSERT(m_currentMonitors.has_value());
    
    if (m_controlledHwnd != hWnd)
    {
        if (m_windowPosChangedHook)
        {
            UnhookWinEvent(m_windowPosChangedHook);
            m_windowPosChangedHook = nullptr;
        }

        DWORD pid = 0;
        GetWindowThreadProcessId(hWnd, &pid);
        m_windowPosChangedHook = SetWinEventHook(
            EVENT_OBJECT_LOCATIONCHANGE,
            EVENT_OBJECT_LOCATIONCHANGE,
            NULL,
            WinPosChangedProc,
            pid,
            0,
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
        );
    }
    else if (!newMonitor && m_installedBrightness == desiredBrightness)
        return true;

    m_controlledHwnd = hWnd;
    m_installedBrightness = desiredBrightness;

    return m_currentMonitors->SetMonitorsBrightness(desiredBrightness);
}

void DisplayBrightnessController::RestoreBrightness() noexcept
{
    if (m_windowPosChangedHook)
    {
        UnhookWinEvent(m_windowPosChangedHook);
        m_windowPosChangedHook = nullptr;
    }
    m_installedBrightness = -1;
    m_controlledHwnd = nullptr;

    m_currentMonitors.reset();
}

void DisplayBrightnessController::OnWindowPosChanged(HWND hWnd)
{
    if (m_controlledHwnd != hWnd)
        return;

    SetBrightnessByHWND(hWnd, m_installedBrightness);
}

DisplayBrightnessController::PhysicalMonitors::PhysicalMonitors(HMONITOR hMonitor)
    : std::vector<PHYSICAL_MONITOR>(get_physical_monitors(hMonitor))
    , m_monitor(hMonitor)
    , m_internalDisplayIndex(findInternalDisplayIndex(hMonitor, size()))
{
    saveOriginalBrightness();
}

DisplayBrightnessController::PhysicalMonitors::~PhysicalMonitors() noexcept
{
    try
    {
        restoreOriginalBrightness();
    }
    catch (...)
    {
        ext::ManageException("Failed to restore original brightness");
    }
    EXT_DUMP_IF(!DestroyPhysicalMonitors(size(), data())) << "Failed to destroy physical monitors";
}

bool DisplayBrightnessController::PhysicalMonitors::IsSameMonitor(HMONITOR hMonitor) const noexcept
{
    return hMonitor == m_monitor;
}

bool DisplayBrightnessController::PhysicalMonitors::SetMonitorsBrightness(uint8_t brightness) noexcept
{
    bool success = false;
    for (size_t i = 0, s = size(); i < s; ++i)
    {
        try
        {
            if (i == m_internalDisplayIndex)
                WMIBrightnessController::SetBrightness(brightness);
            else
                WinAPIBrightnessController::SetBrightness(operator[](i), brightness);
            success = true;
        }
        catch (...)
        {
            ext::ManageException((L"Failed to set brightness to " + std::wstring(operator[](i).szPhysicalMonitorDescription)).c_str());
        }
    }
    return success;
}

bool DisplayBrightnessController::PhysicalMonitors::CanControlBrightness(HMONITOR hMonitor) noexcept
{
    try
    {
        auto monitors = PhysicalMonitors(hMonitor);

        // If original brightness was saved it means that get brightness request executed successfully
        bool anyMonitorSupportsBrightness = std::any_of(
            monitors.m_originalMonitorBrightness.begin(),
            monitors.m_originalMonitorBrightness.end(),
            [](const auto& getBrightnessMethodSupported) { 
                return getBrightnessMethodSupported.has_value();
            });

        if (anyMonitorSupportsBrightness)
        {
            // Avoid brightness restoration
            std::fill(
                monitors.m_originalMonitorBrightness.begin(),
                monitors.m_originalMonitorBrightness.end(),
                std::nullopt);
            return true;
        }
    }
    catch (...)
    {
        ext::ManageException("Failed to check brightness");
    }

    return false;
}

size_t DisplayBrightnessController::PhysicalMonitors::findInternalDisplayIndex(HMONITOR hMonitor, size_t physicalMonitorsCount) noexcept
{
    const auto internalDisplayDeviceId = winrt_monitor_info::GetInternalDisplayDeviceId();
    if (!internalDisplayDeviceId.has_value())
        return size_t(-1);

    MONITORINFOEXW monitorInfo = {};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfoW(hMonitor, &monitorInfo))
    {
        EXT_TRACE() << EXT_TRACE_FUNCTION << "Monitor: " << monitorInfo.szDevice;

        const auto deviceIdByDeviceName = device_id_by_device_name();

        DISPLAY_DEVICEW displayDevice = {};
        displayDevice.cb = sizeof(displayDevice);
        for (size_t i = 0; i < physicalMonitorsCount; ++i)
        {
            if (EnumDisplayDevicesW(monitorInfo.szDevice, i, &displayDevice, 0))
            {
                EXT_TRACE() << EXT_TRACE_FUNCTION
                    << "DeviceID " << displayDevice.DeviceID << "; "
                    << "DeviceName " << displayDevice.DeviceName << "; "
                    << "DeviceKey " << displayDevice.DeviceKey;

                auto it = deviceIdByDeviceName.find(displayDevice.DeviceName);
                EXT_ASSERT(it != deviceIdByDeviceName.end());
                if (it == deviceIdByDeviceName.end())
                {
                    EXT_DUMP_IF(true) << displayDevice.DeviceID << " not found";
                    continue;
                }

                bool internalDisplay = (internalDisplayDeviceId.value() == it->second);
                if (internalDisplay)
                    return i;
            }
            else
                EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "Failed to get display device " << i
                    << ", err " << get_last_error_as_string();
        }
    }
    else
        EXT_TRACE_ERR() << EXT_TRACE_FUNCTION
            << "Failed to get monitor info, err " << get_last_error_as_string();

    return size_t(-1);
}

void DisplayBrightnessController::PhysicalMonitors::saveOriginalBrightness() noexcept
{
    for (size_t i = 0, s = size(); i < s; ++i)
    {
        std::optional<uint8_t> brightness;
        try
        {
            if (i == m_internalDisplayIndex)
                brightness.emplace(WMIBrightnessController::GetBrightness());
            else
                brightness.emplace(WinAPIBrightnessController::GetBrightness(operator[](i)));
        }
        catch (...)
        {
            ext::ManageException((L"Failed to get brightness from " + std::wstring(operator[](i).szPhysicalMonitorDescription)).c_str());
        }
        m_originalMonitorBrightness.emplace_back(std::move(brightness));
    }
}

void DisplayBrightnessController::PhysicalMonitors::restoreOriginalBrightness()
{
    EXT_EXPECT(size() == m_originalMonitorBrightness.size());

    for (size_t i = 0, s = size(); i < s; ++i)
    {
        const auto& originalBrightness = m_originalMonitorBrightness[i];
        if (!originalBrightness.has_value())
            continue;
        
        if (i == m_internalDisplayIndex)
            WMIBrightnessController::SetBrightness(originalBrightness.value());
        else
            WinAPIBrightnessController::SetBrightness(operator[](i), originalBrightness.value());
    }
}

uint8_t DisplayBrightnessController::WinAPIBrightnessController::GetBrightness(const PHYSICAL_MONITOR& physicalMonitor)
{
    // Get the current brightness of the of the HDMI display
    DWORD minBrighness, current, maxBrighness;
    EXT_CHECK(::GetMonitorBrightness(physicalMonitor.hPhysicalMonitor, &minBrighness, &current, &maxBrighness))
        << EXT_TRACE_FUNCTION << get_last_error_as_string();
    EXT_TRACE() << EXT_TRACE_FUNCTION << "Monitor " << physicalMonitor.szPhysicalMonitorDescription
        << ". Min: " << minBrighness << ", max: " << maxBrighness << ", cur: " << current;

    return (uint8_t)current;
}

void DisplayBrightnessController::WinAPIBrightnessController::SetBrightness(const PHYSICAL_MONITOR& physicalMonitor, uint8_t brightness)
{
    EXT_CHECK(::SetMonitorBrightness(physicalMonitor.hPhysicalMonitor, DWORD(brightness)))
        << EXT_TRACE_FUNCTION << get_last_error_as_string();
}

uint8_t DisplayBrightnessController::WMIBrightnessController::GetBrightness()
{
    init_com(false);

    // Create WbemLocator
    IWbemLocator* pLocator = NULL;
    verify_result(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLocator),
        "Failed to create WbemLocator");
    EXT_DEFER(pLocator->Release());

    // Connect to WMI
    IWbemServices* pNamespace = NULL;
    verify_result(pLocator->ConnectServer(_bstr_t(L"root\\wmi"), NULL, NULL, 0, NULL, 0, 0, &pNamespace),
        "Failed to connect to WMI namespace");
    EXT_DEFER(pNamespace->Release());

    // Set security levels on the proxy
    verify_result(CoSetProxyBlanket(pNamespace, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE),
        "Failed to set proxy blanket");

    // Execute query
    IEnumWbemClassObject* pEnum = NULL;
    verify_result(pNamespace->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM WmiMonitorBrightness"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnum),
        "Failed to execute query");
    EXT_DEFER(pEnum->Release());

    // Enumerate results
    ULONG returned = 0;
    IWbemClassObject* pclsObj = NULL;
    verify_result(pEnum->Next(WBEM_INFINITE, 1, &pclsObj, &returned),
        "Failed to iterate over enumeration");
    EXT_DEFER(pclsObj->Release());
    EXT_CHECK(returned > 0) << "Iteration failed";

    int brightnessLevel = -1;
    VARIANT vtBrightness;
    VariantInit(&vtBrightness);
    EXT_DEFER(VariantClear(&vtBrightness));
    verify_result(pclsObj->Get(L"CurrentBrightness", 0, &vtBrightness, 0, 0),
        "Failed to get current brightness");

    return vtBrightness.uintVal;
}

void DisplayBrightnessController::WMIBrightnessController::SetBrightness(uint8_t brightness)
{
    init_com(false);

    // Create WbemLocator
    IWbemLocator* pLocator = NULL;
    verify_result(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLocator),
        "Failed to create WbemLocator");
    EXT_DEFER(pLocator->Release());

    // Connect to WMI
    IWbemServices* pNamespace = NULL;
    verify_result(pLocator->ConnectServer(_bstr_t(L"root\\wmi"), NULL, NULL, 0, NULL, 0, 0, &pNamespace),
        "Failed to connect to WMI namespace");
    EXT_DEFER(pNamespace->Release());

    // Set security levels on the proxy
    verify_result(CoSetProxyBlanket(pNamespace, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE),
        "Failed to set proxy blanket");

    // Execute query
    IEnumWbemClassObject* pEnum = NULL;
    verify_result(pNamespace->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT * FROM WmiMonitorBrightnessMethods"),
        WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnum),
        "Failed to execute query");
    EXT_DEFER(pEnum->Release());

    ULONG ulReturned;
    IWbemClassObject* pObj;
    verify_result(pEnum->Next(WBEM_INFINITE, 1, &pObj, &ulReturned),
        "Failed to iterate over enumeration");
    EXT_DEFER(pObj->Release());

    IWbemClassObject* pClass = NULL;
    verify_result(pNamespace->GetObject(_bstr_t(L"WmiMonitorBrightnessMethods"), 0, NULL, &pClass, NULL),
        "Get object failed");
    EXT_DEFER(pClass->Release());

    IWbemClassObject* pInClass = NULL;
    verify_result(pClass->GetMethod(_bstr_t(L"WmiSetBrightness"), 0, &pInClass, NULL),
        "Get method failed");
    EXT_DEFER(pInClass->Release());

    IWbemClassObject* pInInst = NULL;
    verify_result(pInClass->SpawnInstance(0, &pInInst),
        "Failed to spawn instance");
    EXT_DEFER(pInInst->Release());

    // Set the value of the first argument (timeout)
    {
        VARIANT var1;
        VariantInit(&var1);
        EXT_DEFER(VariantClear(&var1));
        V_VT(&var1) = VT_BSTR;
        V_BSTR(&var1) = SysAllocString(L"0");
        verify_result(pInInst->Put(_bstr_t(L"Timeout"), 0, &var1, 0),
            "Put Timeout failed");
    }

    // Set the value of the second argument (brightness)
    {
        VARIANT var;
        VariantInit(&var);
        EXT_DEFER(VariantClear(&var));
        V_VT(&var) = VT_UI1;
        V_UI1(&var) = brightness;
        verify_result(pInInst->Put(_bstr_t(L"Brightness"), 0, &var, 0),
            "Put Brightness failed");
    }

    {
        // Call the method
        VARIANT pathVariable;
        VariantInit(&pathVariable);
        EXT_DEFER(VariantClear(&pathVariable));
        verify_result(pObj->Get(_bstr_t(L"__PATH"), 0, &pathVariable, NULL, NULL),
            "pObj Get failed");
        verify_result(pNamespace->ExecMethod(
            pathVariable.bstrVal,
            _bstr_t(L"WmiSetBrightness"),
            0,
            NULL,
            pInInst,
            NULL,
            NULL),
            "ExecMethod failed");
    }
}
