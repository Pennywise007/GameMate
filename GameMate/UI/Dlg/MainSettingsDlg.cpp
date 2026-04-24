#include "pch.h"
#include "GameMate.h"
#include "afxdialogex.h"
#include "MainSettingsDlg.h"

#include "core/events.h"
#include "core/Settings.h"
#include "UI/Dlg/InputEditorDlg.h"

#include <ext/core/tracer.h>
#include <ext/std/string.h>
#include <ext/std/filesystem.h>


IMPLEMENT_DYNAMIC(MainSettingsDlg, CDialogEx)

namespace {

const auto kUser = HKEY_CURRENT_USER;
constexpr auto kKey = LR"(Software\Microsoft\Windows\CurrentVersion\Run)";

const std::wstring kAppName = std::filesystem::get_binary_name();

} // namespace

MainSettingsDlg::MainSettingsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MAIN_SETTINGS, pParent)
{
    ext::send_event(&IKeyHandlerBlocker::OnBlockHandler);
}

MainSettingsDlg::~MainSettingsDlg()
{
    ext::send_event(&IKeyHandlerBlocker::OnUnblockHandler);
}

void MainSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CHECK_RUN_ON_STARTUP, m_checkboxRunOnStartup);
    DDX_Control(pDX, IDC_CHECK_ENABLE_TRACES, m_checkboxEnableTraces);
    DDX_Control(pDX, IDC_STATIC_SHOW_TIMER_BIND, m_staticShowTimerBind);
    DDX_Control(pDX, IDC_STATIC_ACTIVE_PROCESS_TOOLKIT_BIND, m_staticActiveProcessToolkitBind);
    DDX_Control(pDX, IDC_STATIC_ACTIONS_EXECUTOR_BIND, m_staticActionsExecutorBind);
}

BEGIN_MESSAGE_MAP(MainSettingsDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_CHAGE_SHOW_TIMER_BIND, &MainSettingsDlg::OnBnClickedButtonChageShowTimerBind)
    ON_BN_CLICKED(IDC_BUTTON_CHANGE_ACTIVE_PROCESS_TOOLKIT_BIND, &MainSettingsDlg::OnBnClickedButtonChangeActiveProcessToolkitBind)
    ON_BN_CLICKED(IDC_BUTTON_CHANGE_ACTIONS_EXECUTOR_BIND, &MainSettingsDlg::OnBnClickedButtonChangeActionsExecutorBind)
END_MESSAGE_MAP()

BOOL MainSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

    m_checkboxRunOnStartup.SetCheck(IsRunAtStartupEnabled() ? BST_CHECKED : BST_UNCHECKED);
    m_checkboxEnableTraces.SetCheck(ext::get_tracer().CanTrace(ext::ITracer::Level::eInfo) ? BST_CHECKED : BST_UNCHECKED);

    const auto& settings = ext::get_singleton<Settings>();
    m_bindShowTimer = settings.timer.showTimerBind;
    m_staticShowTimerBind.SetWindowTextW(m_bindShowTimer.ToString().c_str());
    m_bindActiveProcessToolkit = settings.process_toolkit.enableBind;
    m_staticActiveProcessToolkitBind.SetWindowTextW(m_bindActiveProcessToolkit.ToString().c_str());
    m_bindActionsExecutor = settings.actions_executor.enableBind;
    m_staticActionsExecutorBind.SetWindowTextW(m_bindActionsExecutor.ToString().c_str());

	return TRUE;  // return TRUE unless you set the focus to a control
}

void MainSettingsDlg::OnOK()
{
    ApplySettings();

    CDialogEx::OnOK();
}

void MainSettingsDlg::OnBnClickedButtonChageShowTimerBind()
{
    auto bind = CInputEditorDlg::EditBind(this, m_bindShowTimer);
    if (!bind.has_value())
        return;

    m_bindShowTimer = bind.value();
    m_staticShowTimerBind.SetWindowTextW(m_bindShowTimer.ToString().c_str());
}

void MainSettingsDlg::OnBnClickedButtonChangeActiveProcessToolkitBind()
{
    auto bind = CInputEditorDlg::EditBind(this, m_bindActiveProcessToolkit);
    if (!bind.has_value())
        return;

    m_bindActiveProcessToolkit = bind.value();
    m_staticActiveProcessToolkitBind.SetWindowTextW(m_bindActiveProcessToolkit.ToString().c_str());
}

void MainSettingsDlg::OnBnClickedButtonChangeActionsExecutorBind()
{
    auto bind = CInputEditorDlg::EditBind(this, m_bindActionsExecutor);
    if (!bind.has_value())
        return;

    m_bindActionsExecutor = bind.value();
    m_staticActionsExecutorBind.SetWindowTextW(m_bindActionsExecutor.ToString().c_str());
}

bool MainSettingsDlg::IsRunAtStartupEnabled() const
{
    CRegKey key;
    if (key.Open(kUser,kKey, KEY_READ) != ERROR_SUCCESS)
        return false;

    wchar_t value[4096];
    ULONG size = _countof(value);
    return key.QueryStringValue(kAppName.c_str(), value, &size) == ERROR_SUCCESS;
}

void MainSettingsDlg::UpdateStartupSettings(bool enable) const
{
    CRegKey key;
    if (auto res = key.Open(kUser, kKey, KEY_WRITE); res == ERROR_SUCCESS)
    {
        if (enable)
        {
            const std::wstring value = std::string_swprintf(LR"("%s" --minimized)", std::filesystem::get_full_exe_path().c_str());
            key.SetStringValue(kAppName.c_str(), value.c_str());
        }
        else
        {
            key.DeleteValue(kAppName.c_str());
        }
    }
    else
    {
        std::error_code ec(res, std::system_category());
        ::MessageBox(m_hWnd, std::widen(ec.message()).c_str(),
                     L"Failed to update startup settings",
                     MB_OK | MB_ICONERROR);
    }
}

void MainSettingsDlg::ApplySettings() const
{
    auto& settings = ext::get_singleton<Settings>();
    if (m_checkboxEnableTraces.GetCheck() == BST_CHECKED)
    {
        if (!settings.tracesEnabled)
        {
            ext::get_tracer().Enable();
            settings.tracesEnabled = true;
            ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eGeneralSettings);
        }
    }
    else
    {
        if (settings.tracesEnabled)
        {
            ext::get_tracer().Reset();
            settings.tracesEnabled = false;
            ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eGeneralSettings);
        }
    }

    UpdateStartupSettings(m_checkboxRunOnStartup.GetCheck() == BST_CHECKED);

    settings.timer.showTimerBind = m_bindShowTimer;
    ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eTimer);
    settings.process_toolkit.enableBind = m_bindActiveProcessToolkit;
    ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
    settings.actions_executor.enableBind = m_bindActionsExecutor;
    ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eActionsExecutorEnableChanged);
}
