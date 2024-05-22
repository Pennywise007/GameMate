#pragma once

#include "ActionsEdit.h"
#include "BaseKeyEditDlg.h"

#include <Controls/Tooltip/ToolTip.h>

template <class CBaseWindow>
template <typename... Args>
CActionsEditBase<CBaseWindow>::CActionsEditBase(Args&&... args)
	: CBaseWindow(std::forward<Args>(args)...)
{
}

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::DoDataExchange(CDataExchange* pDX)
{
	CBaseWindow::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MACROSES, m_listActions);
	DDX_Control(pDX, IDC_BUTTON_RECORD, m_buttonRecord);
	DDX_Control(pDX, IDC_EDIT_RANDOMIZE_DELAYS, m_editRandomizeDelays);
	DDX_Control(pDX, IDC_STATIC_DELAY_HELP, m_staticDelayHelp);
	DDX_Control(pDX, IDC_BUTTON_MOVE_UP, m_buttonMoveUp);
	DDX_Control(pDX, IDC_BUTTON_MOVE_DOWN, m_buttonMoveDown);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_buttonAdd);
	DDX_Control(pDX, IDC_BUTTON_REMOVE, m_buttonDelete);
}

BEGIN_TEMPLATE_MESSAGE_MAP(CActionsEditBase, CBaseWindow, CBaseWindow)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CActionsEditBase::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CActionsEditBase::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &CActionsEditBase::OnBnClickedButtonRecord)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_UP, &CActionsEditBase::OnBnClickedButtonMoveUp)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_DOWN, &CActionsEditBase::OnBnClickedButtonMoveDown)
	ON_EN_CHANGE(IDC_EDIT_RANDOMIZE_DELAYS, &CActionsEditBase::OnEnChangeEditRandomizeDelays)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MACROSES, &CActionsEditBase::OnLvnItemchangedListActions)
END_MESSAGE_MAP()

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::OnInit(Actions& actions, bool captureMousePositions)
{
	m_actions = &actions;
	m_captureMousePositions = captureMousePositions;

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

	using namespace controls::list::widgets;
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
			Action* action = (Action*)m_listActions.GetItemDataPtr(pParams->iItem);

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

	for (const auto& action : m_actions->actions)
	{
		addAction(action);
	}

	std::wostringstream str;
	str << m_actions->randomizeDelays;
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

	updateButtonStates();
}

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::OnSettingsChanged()
{
	const auto size = m_listActions.GetItemCount();

	m_actions->actions.resize(size);
	auto it = m_actions->actions.begin();
	for (auto item = 0; item < size; ++item, ++it)
	{
		*it = *(Action*)m_listActions.GetItemDataPtr(item);
	}
}

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::OnDestroy()
{
	unsubscribeFromInputEvents();

	CBaseWindow::OnDestroy();

	// Free memory
	for (auto item = 0, size = m_listActions.GetItemCount(); item < size; ++item)
	{
		std::unique_ptr<Action> actionPtr((Action*)m_listActions.GetItemDataPtr(item));
	}
}

template <class CBaseWindow>
BOOL CActionsEditBase<CBaseWindow>::PreTranslateMessage(MSG* pMsg)
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

	return CBaseWindow::PreTranslateMessage(pMsg);
}

template<class CBaseWindow>
inline void CActionsEditBase<CBaseWindow>::subscribeOnInputEvents()
{
	EXT_ASSERT(m_mouseMoveSubscriptionId == -1 && m_keyPressedSubscriptionId == -1);

	if (m_captureMousePositions)
	{
		m_mouseMoveSubscriptionId = InputManager::AddMouseMoveHandler([&](const POINT& position, const POINT& delta) {
			EXT_ASSERT(m_lastActionTime.has_value());

			auto curTime = std::chrono::steady_clock::now();
			auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - *m_lastActionTime).count();

			addAction(Action(position.x, position.y, int(delay)));
			m_lastActionTime = std::move(curTime);
		});
	}
	m_keyPressedSubscriptionId = InputManager::AddKeyOrMouseHandler([&](WORD vkCode, bool isDown) -> bool {
		EXT_ASSERT(m_lastActionTime.has_value());

		switch (vkCode)
		{
		case VK_LBUTTON:
		{
			CPoint cursor;
			::GetCursorPos(&cursor);

			// Process stop recording and OK click
			auto window = ::WindowFromPoint(cursor);
			if (window == m_buttonRecord.m_hWnd || window == CBaseWindow::GetDlgItem(IDOK)->m_hWnd)
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
}

template<class CBaseWindow>
inline void CActionsEditBase<CBaseWindow>::unsubscribeFromInputEvents()
{
	if (m_keyPressedSubscriptionId != -1)
		InputManager::RemoveKeyOrMouseHandler(m_keyPressedSubscriptionId);
	if (m_mouseMoveSubscriptionId != -1)
		InputManager::RemoveMouseMoveHandler(m_mouseMoveSubscriptionId);
	m_keyPressedSubscriptionId = m_mouseMoveSubscriptionId = -1;
}

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::addAction(Action action)
{
	std::vector<int> selectedActions = m_listActions.GetSelectedItems();

	int item;
	if (selectedActions.empty())
		item = m_listActions.GetItemCount();
	else
		item = selectedActions.back() + 1;

	item = m_listActions.InsertItem(item, std::to_wstring(action.delayInMilliseconds).c_str());
	auto text = action.ToString();
	if (m_captureMousePositions && action.type != Action::Type::eKeyAction)
		text += std::string_swprintf(L"(%d,%d)", action.mouseX, action.mouseY);
	m_listActions.SetItemText(item, Columns::eAction, text.c_str());
	m_listActions.SelectItem(item);

	std::unique_ptr<Action> actionPtr = std::make_unique<Action>(std::move(action));
	m_listActions.SetItemDataPtr(item, actionPtr.release());
}

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::updateButtonStates()
{
	auto selectionExist = m_listActions.GetSelectedCount() > 0;
	m_buttonDelete.EnableWindow(selectionExist);
	m_buttonMoveUp.EnableWindow(selectionExist);
	m_buttonMoveDown.EnableWindow(selectionExist);
}

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::OnBnClickedButtonAdd()
{
	std::optional<Action> action = CActionEditDlg::EditAction(this);
	if (!action.has_value())
		return;

	addAction(std::move(*action));

	OnSettingsChanged();
}

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::OnBnClickedButtonRemove()
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

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::OnBnClickedButtonRecord()
{
	CBaseWindow::KillTimer(kRecordingTimer0Id);
	CBaseWindow::KillTimer(kRecordingTimer1Id);
	CBaseWindow::KillTimer(kRecordingTimer2Id);

	if (m_buttonRecord.GetCheck() == BST_CHECKED)
	{
		CBaseWindow::SetTimer(kRecordingTimer0Id, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Recording starts in 3...");
		m_buttonRecord.SetIcon(IDI_ICON_STOP_RECORDING, Alignment::LeftCenter);
	}
	else
	{
		unsubscribeFromInputEvents();
		m_buttonRecord.SetWindowTextW(L"Record actions");
		m_buttonRecord.SetIcon(IDI_ICON_START_RECORDING, Alignment::LeftCenter);
		m_lastActionTime.reset();
		OnSettingsChanged();
	}
}

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::OnTimer(UINT_PTR nIDEvent)
{
	CBaseWindow::KillTimer(nIDEvent);

	switch (nIDEvent)
	{
	case kRecordingTimer0Id:
		CBaseWindow::SetTimer(kRecordingTimer1Id, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Recording starts in 2...");
		break;
	case kRecordingTimer1Id:
		CBaseWindow::SetTimer(kRecordingTimer2Id, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Recording starts in 1...");
		break;
	case kRecordingTimer2Id:
		m_buttonRecord.SetWindowTextW(L"Cancel recording");
		subscribeOnInputEvents();
		m_lastActionTime = std::chrono::steady_clock::now();
		break;
	default:
		EXT_ASSERT(false) << "Unknown event id " << nIDEvent;
		break;
	}

	CBaseWindow::OnTimer(nIDEvent);
}

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::OnBnClickedButtonMoveUp()
{
	m_listActions.MoveSelectedItems(true);
	OnSettingsChanged();
}

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::OnBnClickedButtonMoveDown()
{
	m_listActions.MoveSelectedItems(false);
	OnSettingsChanged();
}

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::OnLvnItemchangedListActions(NMHDR* pNMHDR, LRESULT* pResult)
{
	updateButtonStates();
	*pResult = 0;
}

template <class CBaseWindow>
void CActionsEditBase<CBaseWindow>::OnEnChangeEditRandomizeDelays()
{
	CString text;
	m_editRandomizeDelays.GetWindowTextW(text);
	std::wistringstream str(text.GetString());
	str >> m_actions->randomizeDelays;

	OnSettingsChanged();
}
