#include "pch.h"
#include "resource.h"

#include "core/Settings.h"

#include "UI/Dlg/EditBindDlg.h"
#include "UI/Dlg/TimerSettingsDlg.h"

namespace {

const COLORREF kDefaultTextColor = timer::Settings().textColor;
const COLORREF kDefaultBackgroundColor = timer::Settings().backgroundColor;

} // namespace

IMPLEMENT_DYNAMIC(CTimerSettings, CDialogEx)

CTimerSettings::CTimerSettings(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_TIMER_SETTINGS, pParent)
{}

void CTimerSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_DISPLAY_HOURS, m_checkDisplayHours);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_TEXT, m_textColor());
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_BACKGROUND, m_backgroundColor());
	DDX_Control(pDX, IDC_STATIC_START_BIND, m_staticStartPauseBind);
	DDX_Control(pDX, IDC_STATIC_RESET_BIND, m_staticResetBind);
	DDX_Control(pDX, IDC_CHECK_HIDE_INTERFACE, m_checkHideInterface);
}

BEGIN_MESSAGE_MAP(CTimerSettings, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_CHANGE_START_BIND, &CTimerSettings::OnBnClickedButtonChangeStartBind)
	ON_BN_CLICKED(IDC_BUTTON_CHANGE_RESET_BIND, &CTimerSettings::OnBnClickedButtonChangeResetBind)
END_MESSAGE_MAP()

BOOL CTimerSettings::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	const auto& timerSettings = ext::get_singleton<Settings>().timer;

	m_checkHideInterface.SetCheck(timerSettings.minimizeInterface ? BST_CHECKED : BST_UNCHECKED);
	m_checkDisplayHours.SetCheck(timerSettings.displayHours ? BST_CHECKED : BST_UNCHECKED);
	m_textColor.SetColor(timerSettings.textColor);
	m_textColor.EnableAutomaticButton(L"Default", kDefaultTextColor);
	m_backgroundColor.SetColor(timerSettings.backgroundColor);
	m_backgroundColor.EnableAutomaticButton(L"Default", kDefaultBackgroundColor);

	m_pauseBind = timerSettings.pauseTimerBind;
	m_staticStartPauseBind.SetWindowTextW(m_pauseBind.ToString().c_str());
	m_resetBind = timerSettings.resetTimerBind;
	m_staticResetBind.SetWindowTextW(m_resetBind.ToString().c_str());

	return TRUE;
}

void CTimerSettings::OnOK()
{
	auto& timerSettings = ext::get_singleton<Settings>().timer;

	timerSettings.minimizeInterface = m_checkHideInterface.GetCheck() == BST_CHECKED;
	timerSettings.displayHours = m_checkDisplayHours.GetCheck() == BST_CHECKED;
	timerSettings.backgroundColor = m_backgroundColor.GetColor();
	timerSettings.textColor = m_textColor.GetColor();
	timerSettings.pauseTimerBind = m_pauseBind;
	timerSettings.resetTimerBind = m_resetBind;

	CDialogEx::OnOK();
}

void CTimerSettings::OnBnClickedButtonChangeStartBind()
{
	auto bind = CEditBindDlg::EditBind(this, m_pauseBind);
	if (!bind.has_value())
		return;

	m_pauseBind = bind.value();
	m_staticStartPauseBind.SetWindowTextW(m_pauseBind.ToString().c_str());
}

void CTimerSettings::OnBnClickedButtonChangeResetBind()
{
	auto bind = CEditBindDlg::EditBind(this, m_resetBind);
	if (!bind.has_value())
		return;

	m_resetBind = bind.value();
	m_staticResetBind.SetWindowTextW(m_resetBind.ToString().c_str());
}
