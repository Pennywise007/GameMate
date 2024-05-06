#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "ActionsExecutorDlg.h"
#include "BaseKeyEditDlg.h"
#include "ActionsEditDlg.h"

#include "core/Settings.h"

namespace {

enum Columns {
	eDelay = 0,
	eAction
};

} // namespace

IMPLEMENT_DYNAMIC(CActionsExecutorDlg, CDialogEx)

CActionsExecutorDlg::CActionsExecutorDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_ACTIONS_EXECUTOR, pParent)
{
}

void CActionsExecutorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCBUTTON_HOTKEY, m_buttonHotkey);
	DDX_Control(pDX, IDC_CHECK_ENABLED, m_buttonEnable);
	DDX_Control(pDX, IDC_EDIT_INTERVAL_MIN, m_editIntervalMinutes);
	DDX_Control(pDX, IDC_EDIT_INTERVAL_SEC, m_editIntervalSeconds);
	DDX_Control(pDX, IDC_EDIT_INTERVAL_MILLISEC, m_editIntervalMilliseconds);
	DDX_Control(pDX, IDC_RADIO_REPEAT_UNTIL_STOPPED, m_radioRepeatUntilStop);
	DDX_Control(pDX, IDC_RADIO_REPEAT_TIMES, m_radioRepeatTimes);
	DDX_Control(pDX, IDC_EDIT_REPEAT_TIMES, m_editRepeatTimes);
	DDX_Control(pDX, IDC_LIST_ACTIONS, m_listActions);
}

BEGIN_MESSAGE_MAP(CActionsExecutorDlg, CDialogEx)
	ON_BN_CLICKED(IDC_RADIO_REPEAT_UNTIL_STOPPED, &CActionsExecutorDlg::OnBnClickedRadioRepeatUntilStopped)
	ON_BN_CLICKED(IDC_RADIO_REPEAT_TIMES, &CActionsExecutorDlg::OnBnClickedRadioRepeatTimes)
	ON_BN_CLICKED(IDC_CHECK_ENABLED, &CActionsExecutorDlg::OnBnClickedCheckEnabled)
	ON_BN_CLICKED(IDC_MFCBUTTON_HOTKEY, &CActionsExecutorDlg::OnBnClickedMfcbuttonHotkey)
	ON_EN_CHANGE(IDC_EDIT_INTERVAL_MIN, &CActionsExecutorDlg::OnEnChangeEditIntervalMin)
	ON_EN_CHANGE(IDC_EDIT_INTERVAL_SEC, &CActionsExecutorDlg::OnEnChangeEditIntervalSec)
	ON_EN_CHANGE(IDC_EDIT_INTERVAL_MILLISEC, &CActionsExecutorDlg::OnEnChangeEditIntervalMillisec)
	ON_EN_CHANGE(IDC_EDIT_REPEAT_TIMES, &CActionsExecutorDlg::OnEnChangeEditRepeatTimes)
	ON_NOTIFY(NM_CLICK, IDC_LIST_ACTIONS, &CActionsExecutorDlg::OnNMClickListActions)
END_MESSAGE_MAP()

BOOL CActionsExecutorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CRect rect;
	m_listActions.GetClientRect(rect);

	constexpr int kDelayColumnWidth = 70;
	m_listActions.InsertColumn(Columns::eDelay, L"Delay(ms)", LVCFMT_CENTER, kDelayColumnWidth);
	m_listActions.InsertColumn(Columns::eAction, L"Action", LVCFMT_LEFT, rect.Width() - kDelayColumnWidth);

	LVCOLUMN colInfo;
	colInfo.mask = LVCF_FMT;
	m_listActions.GetColumn(Columns::eDelay, &colInfo);
	colInfo.fmt |= LVCFMT_FIXED_WIDTH | LVCFMT_CENTER;
	m_listActions.SetColumn(Columns::eDelay, &colInfo);
	m_listActions.SetProportionalResizingColumns({ Columns::eAction });
	
	UpdateActions();

	const auto initSpinEdit = [](CSpinEdit& edit, unsigned value) {
		edit.UsePositiveDigitsOnly();
		edit.SetUseOnlyIntegersValue();

		std::wstringstream str;
		str << value;
		edit.SetWindowTextW(str.str().c_str());
	};

	auto& settings = ext::get_singleton<Settings>().actions_executor;

	initSpinEdit(m_editIntervalMinutes, settings.repeatIntervalMinutes);
	initSpinEdit(m_editIntervalSeconds, settings.repeatIntervalSeconds);
	initSpinEdit(m_editIntervalMilliseconds, settings.repeatIntervalMilliseconds);
	initSpinEdit(m_editRepeatTimes, settings.repeatTimes);

	switch (settings.repeatMode)
	{
	case actions_executor::RepeatMode::eTimes:
		m_radioRepeatTimes.SetCheck(1);
		break;
	case actions_executor::RepeatMode::eUntilStopped:
		m_radioRepeatUntilStop.SetCheck(1);
		break;
	default:
		EXT_ASSERT(false) << "unknown repeat mode";
		break;
	};

	UpdateEnableButtonText();

	m_buttonHotkey.SetBitmap(IDB_PNG_SETTINGS, Alignment::CenterCenter);

	return TRUE;
}

void CActionsExecutorDlg::OnBnClickedRadioRepeatUntilStopped()
{
	m_radioRepeatTimes.SetCheck(0);
	ext::get_singleton<Settings>().actions_executor.repeatMode = actions_executor::RepeatMode::eUntilStopped;
	ext::send_event(&ISettingsChanged::OnSettingsChanged);
}

void CActionsExecutorDlg::OnBnClickedRadioRepeatTimes()
{
	m_radioRepeatUntilStop.SetCheck(0);
	ext::get_singleton<Settings>().actions_executor.repeatMode = actions_executor::RepeatMode::eTimes;
	ext::send_event(&ISettingsChanged::OnSettingsChanged);
}

void CActionsExecutorDlg::OnEnChangeEditIntervalMin()
{
	UpdateSettingsFromControl(m_editIntervalMinutes,
		ext::get_singleton<Settings>().actions_executor.repeatIntervalMinutes);
}

void CActionsExecutorDlg::OnEnChangeEditIntervalSec()
{
	UpdateSettingsFromControl(m_editIntervalSeconds,
		ext::get_singleton<Settings>().actions_executor.repeatIntervalSeconds);
}

void CActionsExecutorDlg::OnEnChangeEditIntervalMillisec()
{
	UpdateSettingsFromControl(m_editIntervalMilliseconds,
		ext::get_singleton<Settings>().actions_executor.repeatIntervalMilliseconds);
}

void CActionsExecutorDlg::OnEnChangeEditRepeatTimes()
{
	UpdateSettingsFromControl(m_editRepeatTimes,
		ext::get_singleton<Settings>().actions_executor.repeatTimes);
}

void CActionsExecutorDlg::OnBnClickedCheckEnabled()
{
	auto& settings = ext::get_singleton<Settings>().actions_executor;
	settings.enabled = !settings.enabled;

	ext::send_event(&ISettingsChanged::OnSettingsChanged);
}

void CActionsExecutorDlg::OnBnClickedMfcbuttonHotkey()
{
	auto& currentBind = ext::get_singleton<Settings>().actions_executor.enableBind;
	auto bind = CBindEditDlg::EditBind(this, currentBind);
	if (!bind.has_value() || currentBind.ToString() == bind->ToString())
		return;

	currentBind = bind.value();
	ext::send_event(&ISettingsChanged::OnSettingsChanged);
}

void CActionsExecutorDlg::OnSettingsChanged()
{
	UpdateEnableButtonText();
}

void CActionsExecutorDlg::UpdateEnableButtonText()
{
	const auto& settings = ext::get_singleton<Settings>().actions_executor;
	const auto buttonName = std::string_swprintf(L"%s\n(%s)",
		settings.enabled ? L"Stop" : L"False",
		settings.enableBind.ToString().c_str());
	m_buttonEnable.SetWindowTextW(buttonName.c_str());
}

void CActionsExecutorDlg::UpdateSettingsFromControl(CSpinEdit& edit, unsigned& setting)
{
	CString controlText;
	edit.GetWindowTextW(controlText);

	std::wstringstream str(controlText.GetString());
	str >> setting;

	ext::send_event(&ISettingsChanged::OnSettingsChanged);
}

void CActionsExecutorDlg::EditActions()
{
	auto& currentActions = ext::get_singleton<Settings>().actions_executor.actionsSettings;
	auto updatedActions = CActionsEditDlg::ExecModal(this, currentActions);
	if (!updatedActions.has_value())
		return;

	UpdateActions();

	currentActions = std::move(updatedActions.value());
	ext::send_event(&ISettingsChanged::OnSettingsChanged);
}

void CActionsExecutorDlg::UpdateActions()
{
	m_listActions.DeleteAllItems();

	const auto& actions = ext::get_singleton<Settings>().actions_executor.actionsSettings.actions;

	auto itemId = 0;
	for (const auto& action : actions)
	{
		auto item = m_listActions.InsertItem(itemId++, std::to_wstring(action.delayInMilliseconds).c_str());
		m_listActions.SetItemText(item, Columns::eAction, action.ToString().c_str());
	}
}

void CActionsExecutorDlg::OnNMClickListActions(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	EditActions();
}

