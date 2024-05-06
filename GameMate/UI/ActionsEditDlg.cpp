#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "ActionsEditDlg.h"
#include "BaseKeyEditDlg.h"
#include "InputManager.h"

#include <Controls/Layout/Layout.h>
#include <Controls/Tables/List/Widgets/SubItemsEditor/SubItemsEditor.h>
#include <Controls/Tooltip/ToolTip.h>

#include <ext/core/check.h>

using namespace controls::list::widgets;

namespace {

enum Columns {
	eDelay = 0,
	eAction
};

constexpr UINT kTimerInterval1Sec = 1000;

constexpr UINT kRecordingTimer0Id = 0;
constexpr UINT kRecordingTimer1Id = 1;
constexpr UINT kRecordingTimer2Id = 2;

} // namespace

IMPLEMENT_DYNAMIC(CActionsEditDlg, CDialogEx)

CActionsEditDlg::CActionsEditDlg(CWnd* pParent, Actions& macros)
	: CDialogEx(IDD_DIALOG_ACTIONS_EDIT, pParent)
	, m_macros(macros)
{
}

std::optional<Actions> CActionsEditDlg::ExecModal(CWnd* pParent, const Actions& currentActions)
{
	Actions actions = currentActions;
	if (CActionsEditDlg(pParent, actions).DoModal() != IDOK)
		return std::nullopt;

	return actions;
}

void CActionsEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MACROSES, m_listActions);
	DDX_Control(pDX, IDC_BUTTON_RECORD, m_buttonRecord);
	DDX_Control(pDX, IDC_EDIT_RANDOMIZE_DELAYS, m_editRandomizeDelays);
	DDX_Control(pDX, IDC_STATIC_DELAY_HELP, m_staticDelayHelp);
	DDX_Control(pDX, IDC_BUTTON_MOVE_UP, m_buttonMoveUp);
	DDX_Control(pDX, IDC_BUTTON_MOVE_DOWN, m_buttonMoveDown);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_buttonAdd);
	DDX_Control(pDX, IDC_BUTTON_REMOVE, m_buttonDelete);
}

BEGIN_MESSAGE_MAP(CActionsEditDlg, CDialogEx)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CActionsEditDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CActionsEditDlg::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &CActionsEditDlg::OnBnClickedButtonRecord)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MACROSES, &CActionsEditDlg::OnLvnItemchangedListActions)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_UP, &CActionsEditDlg::OnBnClickedButtonMoveUp)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_DOWN, &CActionsEditDlg::OnBnClickedButtonMoveDown)
	ON_EN_CHANGE(IDC_EDIT_RANDOMIZE_DELAYS, &CActionsEditDlg::OnEnChangeEditRandomizeDelays)
END_MESSAGE_MAP()

BOOL CActionsEditDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CRect rect;
	m_staticDelayHelp.GetClientRect(rect);

	HICON icon;
	auto res = SUCCEEDED(::LoadIconWithScaleDown(NULL, IDI_INFORMATION, rect.Height(), rect.Height(), &icon));
	ASSERT(res);

	m_staticDelayHelp.ModifyStyle(0, SS_ICON);
	m_staticDelayHelp.SetIcon(icon);

	controls::SetTooltip(m_staticDelayHelp, L"Some game guards may check if you press buttons with the same delay and can ban you.\n"
		L"To avoid this you can set a percentage of the delay which will be applied to te ation delay randomly.\n"
		L"Formula: delay ± random(percentage)\n");

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
	// TODO CHECK LOSING FOCUS AFTER EDITTING PROBLEM
	m_listActions.setSubItemEditorController(Columns::eDelay,
		[](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto edit = std::make_shared<CEditBase>();

			edit->Create(SubItemEditorControllerBase::getStandartEditorWndStyle() | 
				ES_CENTER | CBS_AUTOHSCROLL,
				CRect(), parentWindow, 0);
			edit->UsePositiveDigitsOnly();
			edit->SetUseOnlyIntegersValue();

			CString curSubItemText = pList->GetItemText(pParams->iItem, pParams->iSubItem);
			edit->SetWindowTextW(curSubItemText);

			return std::shared_ptr<CWnd>(edit);
		},
		[&](CListCtrl* pList, CWnd* editorControl, const LVSubItemParams* pParams, bool bAcceptResult)
		{
			if (!bAcceptResult)
				return false;

			CString currentEditorText;
			editorControl->GetWindowTextW(currentEditorText);

			auto* list = dynamic_cast<CListGroupCtrl*>(pList);
			ASSERT(list);

			Action* action = (Action*)list->GetItemDataPtr(pParams->iItem);
			action->delayInMilliseconds = _wtoi(currentEditorText);
			OnSettingsChanged();

			return true;
		});
	m_listActions.setSubItemEditorController(Columns::eAction,
		[&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto* list = dynamic_cast<CListGroupCtrl*>(pList);
			ASSERT(list);

			Action* action = (Action*)list->GetItemDataPtr(pParams->iItem);

			auto newAction = CActionEditDlg::EditAction(this, *action);
			if (newAction.has_value())
			{
                *action = std::move(*newAction);
                m_listActions.SetItemText(pParams->iItem, Columns::eAction, action->ToString().c_str());
				OnSettingsChanged();
			}

			return nullptr;
		});

	m_editRandomizeDelays.UsePositiveDigitsOnly();
	m_editRandomizeDelays.SetMinMaxLimits(0, 100);

	for (const auto& action : m_macros.actions)
	{
		addAction(action);
	}

	std::wostringstream str;
	str << m_macros.randomizeDelays;
	m_editRandomizeDelays.SetWindowTextW(str.str().c_str());

	// TODO find better icons, maybe https://www.iconfinder.com/icons/22947/red_stop_icon?coming-from=related-results
	m_buttonRecord.SetImageOffset(0);
	m_buttonRecord.SetIcon(IDI_ICON_START_RECORDING, Alignment::LeftCenter);

	m_buttonMoveUp.SetBitmap(IDB_PNG_ARROW_UP, Alignment::CenterCenter);
	m_buttonMoveUp.SetWindowTextW(L"");
	m_buttonMoveDown.SetBitmap(IDB_PNG_ARROW_DOWN, Alignment::CenterCenter);
	m_buttonMoveDown.SetWindowTextW(L"");
	m_buttonAdd.SetBitmap(IDB_PNG_ADD, Alignment::CenterCenter);
	m_buttonAdd.SetWindowTextW(L"");
	m_buttonDelete.SetBitmap(IDB_PNG_DELETE, Alignment::CenterCenter);
	m_buttonDelete.SetWindowTextW(L"");

	// TODO FIX TAB TOPS FOR ARROWS

	controls::SetTooltip(m_buttonAdd, L"Add action");
	controls::SetTooltip(m_buttonDelete, L"Delete selected action");
	controls::SetTooltip(m_buttonMoveUp, L"Move selected up");
	controls::SetTooltip(m_buttonMoveDown, L"Move selected up");

	m_keyPressedSubscriptionId = InputManager::AddKeyOrMouseHandler([&](WORD vkCode, bool isDown) -> bool {
		if (!m_lastActionTime.has_value())
			return false;

		switch (vkCode)
		{
		case VK_LBUTTON:
			{
				CPoint cursor;
				::GetCursorPos(&cursor);

				// Process stop recording and OK click
				auto window = ::WindowFromPoint(cursor);
				if (window == m_buttonRecord.m_hWnd || window == GetDlgItem(IDOK)->m_hWnd)
				{
					OnSettingsChanged();
					return false;
				}

				[[fallthrough]];
			}
		default:
			{
				auto curTime = std::chrono::steady_clock::now();
				auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - *m_lastActionTime).count();

				addAction(Action(vkCode, isDown, int(delay)));
				m_lastActionTime = std::move(curTime);
				return true;
			}
		}
	});


	updateButtonStates();

	return TRUE;
}

void CActionsEditDlg::OnSettingsChanged()
{
	const auto size = m_listActions.GetItemCount();

	m_macros.actions.resize(size);
	auto it = m_macros.actions.begin();
	for (auto item = 0; item < size; ++item, ++it)
	{
		*it = *(Action*)m_listActions.GetItemDataPtr(item);
	}
}

void CActionsEditDlg::OnDestroy()
{
	InputManager::RemoveKeyOrMouseHandler(m_keyPressedSubscriptionId);

	CDialogEx::OnDestroy();

	// Free memory
	for (auto item = 0, size = m_listActions.GetItemCount(); item < size; ++item)
	{
		std::unique_ptr<Action> actionPtr((Action*)m_listActions.GetItemDataPtr(item));
	}
}

BOOL CActionsEditDlg::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
		case VK_ESCAPE:
			// Don't close window on enter and escape
			return TRUE;
		}
		break;
	}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CActionsEditDlg::addAction(Action action)
{
	std::vector<int> selectedActions = m_listActions.GetSelectedItems();

	int item;
	if (selectedActions.empty())
		item = m_listActions.GetItemCount();
	else
		item = selectedActions.back() + 1;

	item = m_listActions.InsertItem(item, std::to_wstring(action.delayInMilliseconds).c_str());
	m_listActions.SetItemText(item, Columns::eAction, action.ToString().c_str());
	m_listActions.SelectItem(item);

	std::unique_ptr<Action> actionPtr = std::make_unique<Action>(std::move(action));
	m_listActions.SetItemDataPtr(item, actionPtr.release());
}

void CActionsEditDlg::updateButtonStates()
{
	auto selectionExist = m_listActions.GetSelectedCount() > 0;
	m_buttonDelete.EnableWindow(selectionExist);
	m_buttonMoveUp.EnableWindow(selectionExist);
	m_buttonMoveDown.EnableWindow(selectionExist);
}

void CActionsEditDlg::OnBnClickedButtonAdd()
{
	std::optional<Action> action = CActionEditDlg::EditAction(this);
	if (!action.has_value())
		return;

	addAction(std::move(*action));

	OnSettingsChanged();
}

void CActionsEditDlg::OnBnClickedButtonRemove()
{
	std::vector<int> selectedActions = m_listActions.GetSelectedItems();

	for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end; ++it)
	{
		std::unique_ptr<Action> actionPtr((Action*)m_listActions.GetItemDataPtr(*it));
		m_listActions.DeleteItem(*it);
	}
	m_listActions.SelectItem(selectedActions.back() - int(selectedActions.size()) + 1);
	OnSettingsChanged();
}

void CActionsEditDlg::OnBnClickedButtonRecord()
{
	KillTimer(kRecordingTimer0Id);
	KillTimer(kRecordingTimer1Id);
	KillTimer(kRecordingTimer2Id);

	if (m_buttonRecord.GetCheck() == BST_CHECKED)
	{
		SetTimer(kRecordingTimer0Id, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Recording starts in 3...");
		m_buttonRecord.SetIcon(IDI_ICON_STOP_RECORDING, Alignment::LeftCenter);
	}
	else
	{
		m_buttonRecord.SetWindowTextW(L"Record actions");
		m_buttonRecord.SetIcon(IDI_ICON_START_RECORDING, Alignment::LeftCenter);
		m_lastActionTime.reset();
		OnSettingsChanged();
	}
}

void CActionsEditDlg::OnTimer(UINT_PTR nIDEvent)
{
	KillTimer(nIDEvent);

	switch (nIDEvent)
	{
	case kRecordingTimer0Id:
		SetTimer(kRecordingTimer1Id, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Recording starts in 2...");
		break;
	case kRecordingTimer1Id:
		SetTimer(kRecordingTimer2Id, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Recording starts in 1...");
		break;
	case kRecordingTimer2Id:
		m_buttonRecord.SetWindowTextW(L"Cancel recording");
		m_lastActionTime = std::chrono::steady_clock::now();
		break;
	default:
		EXT_ASSERT(false) << "Unknown event id " << nIDEvent;
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CActionsEditDlg::OnBnClickedButtonMoveUp()
{
	m_listActions.MoveSelectedItems(true);
	OnSettingsChanged();
}

void CActionsEditDlg::OnBnClickedButtonMoveDown()
{
	m_listActions.MoveSelectedItems(false);
	OnSettingsChanged();
}

void CActionsEditDlg::OnLvnItemchangedListActions(NMHDR* pNMHDR, LRESULT* pResult)
{
	updateButtonStates();
	*pResult = 0;
}

void CActionsEditDlg::OnEnChangeEditRandomizeDelays()
{
	CString text;
	m_editRandomizeDelays.GetWindowTextW(text);
	std::wistringstream str(text.GetString());
	str >> m_macros.randomizeDelays;

	OnSettingsChanged();
}

IMPLEMENT_DYNAMIC(CActionsEditView, CActionsEditDlg)

BEGIN_MESSAGE_MAP(CActionsEditView, CActionsEditDlg)
END_MESSAGE_MAP()

CActionsEditView::CActionsEditView(CWnd* pParent, Actions& actions, OnSettingsChangedCallback callback)
	: CActionsEditDlg(pParent, actions)
	, m_onSettingsChangedCallback(std::move(callback))
{
}

BOOL CActionsEditView::OnInitDialog()
{
	auto res = CActionsEditDlg::OnInitDialog();

	// We need to hide OK button and remove extra offset from the sides
	GetDlgItem(IDOK)->ShowWindow(SW_HIDE);

	auto parent = GetParent();
	ASSERT(parent);

	CRect rect;
	parent->GetClientRect(rect);
	rect.InflateRect(5, 5);

	MoveWindow(rect);
	ShowWindow(SW_SHOW);

	Layout::AnchorWindow(*this, *parent, { AnchorSide::eLeft }, AnchorSide::eLeft, 100);
	Layout::AnchorWindow(*this, *parent, { AnchorSide::eTop }, AnchorSide::eTop, 100);
	Layout::AnchorWindow(*this, *parent, { AnchorSide::eRight }, AnchorSide::eRight, 100);
	Layout::AnchorWindow(*this, *parent, { AnchorSide::eBottom }, AnchorSide::eBottom, 100);

	LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);

	return res;
}

void CActionsEditView::PreSubclassWindow()
{
	// Changing dlg to view
	LONG lStyle = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_POPUP | DS_MODALFRAME);
	lStyle |= WS_CHILDWINDOW;
	SetWindowLong(m_hWnd, GWL_STYLE, lStyle);

	LONG lExStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE);
	lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	SetWindowLong(m_hWnd, GWL_EXSTYLE, lExStyle);

	CActionsEditDlg::PreSubclassWindow();
}

void CActionsEditView::OnSettingsChanged()
{
	CActionsEditDlg::OnSettingsChanged();

	if (m_onSettingsChangedCallback)
		m_onSettingsChangedCallback();
}
