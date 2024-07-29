#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "UI/Tab/ActionsExecutorTab.h"
#include "UI/Dlg/InputEditorDlg.h"

#include "core/Settings.h"

#include <ext/reflection/enum.h>

#include <Controls/Layout/Layout.h>

IMPLEMENT_DYNAMIC(CActionsExecutorTab, CDialogEx)

CActionsExecutorTab::CActionsExecutorTab(CWnd* pParent)
	: CDialogEx(IDD_DIALOG_ACTIONS_EXECUTOR, pParent)
{
}

void CActionsExecutorTab::DoDataExchange(CDataExchange* pDX)
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
	DDX_Control(pDX, IDC_STATIC_ACTIONS, m_actionsGroup);
}

BEGIN_MESSAGE_MAP(CActionsExecutorTab, CDialogEx)
	ON_BN_CLICKED(IDC_RADIO_REPEAT_UNTIL_STOPPED, &CActionsExecutorTab::OnBnClickedRadioRepeatUntilStopped)
	ON_BN_CLICKED(IDC_RADIO_REPEAT_TIMES, &CActionsExecutorTab::OnBnClickedRadioRepeatTimes)
	ON_BN_CLICKED(IDC_CHECK_ENABLED, &CActionsExecutorTab::OnBnClickedCheckEnabled)
	ON_BN_CLICKED(IDC_MFCBUTTON_HOTKEY, &CActionsExecutorTab::OnBnClickedMfcbuttonHotkey)
	ON_EN_CHANGE(IDC_EDIT_INTERVAL_MIN, &CActionsExecutorTab::OnEnChangeEditIntervalMin)
	ON_EN_CHANGE(IDC_EDIT_INTERVAL_SEC, &CActionsExecutorTab::OnEnChangeEditIntervalSec)
	ON_EN_CHANGE(IDC_EDIT_INTERVAL_MILLISEC, &CActionsExecutorTab::OnEnChangeEditIntervalMillisec)
	ON_EN_CHANGE(IDC_EDIT_REPEAT_TIMES, &CActionsExecutorTab::OnEnChangeEditRepeatTimes)
END_MESSAGE_MAP()

BOOL CActionsExecutorTab::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	const auto initSpinEdit = [](CSpinEdit& edit, unsigned value) {
		edit.UsePositiveDigitsOnly();
		edit.SetUseOnlyIntegersValue();

		std::wstringstream str;
		str << value;
		edit.SetWindowTextW(str.str().c_str());
		edit.SetMinMaxLimits(0., 9999.);
	};

	auto& settings = ext::get_singleton<Settings>().actions_executor;

	initSpinEdit(m_editIntervalMinutes, settings.repeatIntervalMinutes);
	initSpinEdit(m_editIntervalSeconds, settings.repeatIntervalSeconds);
	initSpinEdit(m_editIntervalMilliseconds, settings.repeatIntervalMilliseconds);
	initSpinEdit(m_editRepeatTimes, settings.repeatTimes);
	m_editRepeatTimes.SetMinMaxLimits(1., 9999);

	switch (settings.repeatMode)
	{
	case actions_executor::RepeatMode::eTimes:
		m_radioRepeatTimes.SetCheck(1);
		break;
	case actions_executor::RepeatMode::eUntilStopped:
		m_radioRepeatUntilStop.SetCheck(1);
		break;
	default:
		static_assert(ext::reflection::get_enum_size<actions_executor::RepeatMode>() == 2, "Not handled enum value");
		EXT_ASSERT(false) << "unknown repeat mode";
		break;
	};

	UpdateEnableButtonText();

	m_buttonHotkey.SetBitmap(IDB_PNG_SETTINGS, Alignment::CenterCenter);

	CCreateContext  ctx;
	ctx.m_pNewViewClass = RUNTIME_CLASS(CActionsEditorView);
	ctx.m_pNewDocTemplate = NULL;
	ctx.m_pLastView = NULL;
	ctx.m_pCurrentFrame = NULL;

	CFrameWnd* pFrameWnd = (CFrameWnd*)&m_actionsGroup;
	CFormView* pView = (CFormView*)pFrameWnd->CreateView(&ctx);
	EXT_EXPECT(pView && pView->GetSafeHwnd() != NULL) << "Failed to create";
	pView->OnInitialUpdate();

	m_actionsEditView = dynamic_cast<CActionsEditorView*>(pView);
	EXT_EXPECT(m_actionsEditView) << "Failed to create";

	CRect rect;
	m_actionsGroup.GetClientRect(rect);
	m_actionsEditView->MoveWindow(rect);

	Layout::AnchorWindow(*m_actionsEditView, m_actionsGroup, { AnchorSide::eLeft }, AnchorSide::eLeft, 100);
	Layout::AnchorWindow(*m_actionsEditView, m_actionsGroup, { AnchorSide::eTop }, AnchorSide::eTop, 100);
	Layout::AnchorWindow(*m_actionsEditView, m_actionsGroup, { AnchorSide::eRight }, AnchorSide::eRight, 100);
	Layout::AnchorWindow(*m_actionsEditView, m_actionsGroup, { AnchorSide::eBottom }, AnchorSide::eBottom, 100);

	m_actionsEditView->Init(
		ext::get_singleton<Settings>().actions_executor.actions,
		[]() { ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eActionsExecutor); });

	pView->ShowWindow(SW_NORMAL);

	LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);

	return TRUE;
}

void CActionsExecutorTab::OnOK()
{
	// CDialogEx::OnOK();
}

void CActionsExecutorTab::OnCancel()
{
	// CDialogEx::OnCancel();
}

void CActionsExecutorTab::OnBnClickedRadioRepeatUntilStopped()
{
	m_radioRepeatTimes.SetCheck(0);
	ext::get_singleton<Settings>().actions_executor.repeatMode = actions_executor::RepeatMode::eUntilStopped;
	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eActionsExecutor);
}

void CActionsExecutorTab::OnBnClickedRadioRepeatTimes()
{
	m_radioRepeatUntilStop.SetCheck(0);
	ext::get_singleton<Settings>().actions_executor.repeatMode = actions_executor::RepeatMode::eTimes;
	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eActionsExecutor);
}

void CActionsExecutorTab::OnEnChangeEditIntervalMin()
{
	UpdateSettingsFromControl(m_editIntervalMinutes,
		ext::get_singleton<Settings>().actions_executor.repeatIntervalMinutes);
}

void CActionsExecutorTab::OnEnChangeEditIntervalSec()
{
	UpdateSettingsFromControl(m_editIntervalSeconds,
		ext::get_singleton<Settings>().actions_executor.repeatIntervalSeconds);
}

void CActionsExecutorTab::OnEnChangeEditIntervalMillisec()
{
	UpdateSettingsFromControl(m_editIntervalMilliseconds,
		ext::get_singleton<Settings>().actions_executor.repeatIntervalMilliseconds);
}

void CActionsExecutorTab::OnEnChangeEditRepeatTimes()
{
	UpdateSettingsFromControl(m_editRepeatTimes,
		ext::get_singleton<Settings>().actions_executor.repeatTimes);
}

void CActionsExecutorTab::OnBnClickedCheckEnabled()
{
	auto& settings = ext::get_singleton<Settings>().actions_executor;
	settings.enabled = !settings.enabled;
	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eActionsExecutorEnableChanged);
}

void CActionsExecutorTab::OnBnClickedMfcbuttonHotkey()
{
	auto& currentBind = ext::get_singleton<Settings>().actions_executor.enableBind;
	auto bind = CInputEditorDlg::EditBind(this, currentBind);
	if (!bind.has_value() || currentBind.ToString() == bind->ToString())
		return;

	currentBind = bind.value();
	UpdateEnableButtonText();
	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eActionsExecutor);
}

void CActionsExecutorTab::OnSettingsChanged(ISettingsChanged::ChangedType changedType)
{
	if (changedType != ISettingsChanged::ChangedType::eActionsExecutorEnableChanged)
		return;

	UpdateEnableButtonText();

	struct Data
	{
		bool enable;
		std::set<CWnd*> excludedControls;
	};

	const Data data { 
		.enable = !ext::get_singleton<Settings>().actions_executor.enabled,
		.excludedControls = { &m_buttonEnable, &m_buttonHotkey }
	};

	EnumChildWindows(m_hWnd, [](HWND hWnd, LPARAM lParam)
		{
			const auto* data = reinterpret_cast<Data*>(lParam);

			CWnd* pWnd = CWnd::FromHandle(hWnd);
			if (data->excludedControls.find(pWnd) == data->excludedControls.end())
				pWnd->EnableWindow(data->enable);

			return TRUE;
		}, LPARAM(&data));
}

void CActionsExecutorTab::UpdateEnableButtonText()
{
	const auto& settings = ext::get_singleton<Settings>().actions_executor;
	const auto buttonName = std::string_swprintf(L"%s\n(%s)",
		settings.enabled ? L"Stop" : L"Start",
		settings.enableBind.ToString().c_str());
	m_buttonEnable.SetWindowTextW(buttonName.c_str());
}

void CActionsExecutorTab::UpdateSettingsFromControl(CSpinEdit& edit, unsigned& setting)
{
	CString controlText;
	edit.GetWindowTextW(controlText);

	std::wstringstream str(controlText.GetString());
	str >> setting;

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eActionsExecutor);
}

void CActionsExecutorTab::EditActions()
{
	auto& currentActions = ext::get_singleton<Settings>().actions_executor.actions;
	auto updatedActions = CActionsEditDlg::ExecModal(this, currentActions);
	if (!updatedActions.has_value())
		return;

	currentActions = std::move(updatedActions.value());
	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eActionsExecutor);
}
