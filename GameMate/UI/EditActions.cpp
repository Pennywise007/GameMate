#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "AddActionDlg.h"
#include "EditActions.h"
#include "EditValueDlg.h"
#include "InputManager.h"

#include <ext/thread/invoker.h>

#include <Controls/Layout/Layout.h>
#include <Controls/Tooltip/ToolTip.h>

#include <ext/core/check.h>

namespace {

using TableItemType = std::list<Action>;

enum class RecordModes
{
	eNoMouseMovements,
	eRecordCursorPosition,
	eRecordCursorDelta,
	eRecordCursorDeltaWithDirectX
};

} // namespace

IMPLEMENT_DYNCREATE(CActionsEditorView, CFormView)

BEGIN_MESSAGE_MAP(CActionsEditorView, CFormView)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CActionsEditorView::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CActionsEditorView::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &CActionsEditorView::OnBnClickedButtonRecord)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_UP, &CActionsEditorView::OnBnClickedButtonMoveUp)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_DOWN, &CActionsEditorView::OnBnClickedButtonMoveDown)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_DELAY, &CActionsEditorView::OnBnClickedButtonEditDelay)
	ON_EN_CHANGE(IDC_EDIT_RANDOMIZE_DELAYS, &CActionsEditorView::OnEnChangeEditRandomizeDelays)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MACROSES, &CActionsEditorView::OnLvnItemchangedListActions)
	ON_BN_CLICKED(IDC_CHECK_UNITE_MOVEMENTS, &CActionsEditorView::OnBnClickedCheckUniteMovements)
END_MESSAGE_MAP()

CActionsEditorView::CActionsEditorView()
	: CFormView(IDD_VIEW_ACTIONS_EDITOR)
{
}

void CActionsEditorView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MACROSES, m_listActions);
	DDX_Control(pDX, IDC_BUTTON_RECORD, m_buttonRecord);
	DDX_Control(pDX, IDC_EDIT_RANDOMIZE_DELAYS, m_editRandomizeDelays);
	DDX_Control(pDX, IDC_STATIC_DELAY_HELP, m_staticDelayHelp);
	DDX_Control(pDX, IDC_BUTTON_MOVE_UP, m_buttonMoveUp);
	DDX_Control(pDX, IDC_BUTTON_MOVE_DOWN, m_buttonMoveDown);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_buttonAdd);
	DDX_Control(pDX, IDC_BUTTON_REMOVE, m_buttonDelete);
	DDX_Control(pDX, IDC_CHECK_UNITE_MOVEMENTS, m_checkUniteMouseMovements);
	DDX_Control(pDX, IDC_COMBO_RECORD_MODE, m_comboRecordMode);
	DDX_Control(pDX, IDC_BUTTON_EDIT_DELAY, m_buttonEditDelay);
}

BOOL CActionsEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
	auto res = CFormView::PreCreateWindow(cs);
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	return res;
}

void CActionsEditorView::Init(Actions& actions, OnSettingsChangedCallback callback, bool captureMousePositions)
{
	m_actions = &actions;

	CRect rect;
	m_staticDelayHelp.GetClientRect(rect);

	HICON icon;
	auto res = SUCCEEDED(::LoadIconWithScaleDown(NULL, IDI_INFORMATION, rect.Height(), rect.Height(), &icon));
	ASSERT(res);

	m_staticDelayHelp.ModifyStyle(0, SS_ICON);
	m_staticDelayHelp.SetIcon(icon);

	controls::SetTooltip(m_staticDelayHelp, L"Some game guards may check if you press buttons with the same delay and can ban you.\n"
		L"To avoid this you can set a percentage of the delay which will be applied to the action delay randomly.\n"
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
		[&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams) -> std::shared_ptr<CWnd>
		{
			auto edit = std::make_shared<CEditBase>();

			// Don't edit united rows
			TableItemType* itemData = (TableItemType*)m_listActions.GetItemDataPtr(pParams->iItem);
			if (itemData->size() > 1)
			{
				MessageBox(L"Stop items uniting and edit them separately", L"Can't edit united items", MB_OK);
				return nullptr;
			}

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

			TableItemType* action = (TableItemType*)list->GetItemDataPtr(pParams->iItem);
			EXT_ASSERT(action->size() == 1);

			action->front().delayInMilliseconds = _wtoi(currentEditorText);
			onSettingsChanged();

			return true;
		});
	m_listActions.setSubItemEditorController(Columns::eAction,
		[&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			// Don't edit united rows
			TableItemType* itemData = (TableItemType*)m_listActions.GetItemDataPtr(pParams->iItem);
			if (itemData->size() > 1)
			{
				MessageBox(L"Stop items uniting and edit them separately", L"Can't edit united items", MB_OK);
				return nullptr;
			}

			Action& action = itemData->front();

			auto newAction = CAddActionDlg::EditAction(this, action);
			if (newAction.has_value())
			{
				action = std::move(*newAction);
				m_listActions.SetItemText(pParams->iItem, Columns::eAction, action.ToString().c_str());
				onSettingsChanged();
			}

			return nullptr;
		});

	m_editRandomizeDelays.UsePositiveDigitsOnly();
	m_editRandomizeDelays.SetMinMaxLimits(0, 100);

	addActions(m_actions->actions);

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
	m_buttonEditDelay.SetBitmap(IDB_PNG_DELAY, Alignment::CenterCenter);
	m_buttonEditDelay.SetWindowTextW(L"");

	/// TODO add help
	m_comboRecordMode.InsertString((int)RecordModes::eNoMouseMovements, L"Don't record mouse");
	m_comboRecordMode.InsertString((int)RecordModes::eRecordCursorPosition, L"Record cursor position");
	m_comboRecordMode.InsertString((int)RecordModes::eRecordCursorDelta, L"Record cursor delta");
	m_comboRecordMode.InsertString((int)RecordModes::eRecordCursorDeltaWithDirectX, L"Record DirectX cursor delta");
	m_comboRecordMode.SetCurSel(int(captureMousePositions ? RecordModes::eRecordCursorPosition : RecordModes::eNoMouseMovements));

	// TODO FIX TAB TOPS FOR ARROWS
	controls::SetTooltip(m_buttonAdd, L"Add action");
	controls::SetTooltip(m_buttonDelete, L"Delete selected action");
	controls::SetTooltip(m_buttonMoveUp, L"Move selected up");
	controls::SetTooltip(m_buttonMoveDown, L"Move selected up");
	controls::SetTooltip(m_buttonEditDelay, L"Set delay for all selected items");

	updateButtonStates();

	LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);
}

void CActionsEditorView::onSettingsChanged()
{
	const auto size = m_listActions.GetItemCount();
	m_actions->actions.clear();
	for (auto item = 0; item < size; ++item)
	{
		TableItemType* actionPtr((TableItemType*)m_listActions.GetItemDataPtr(item));
		for (auto&& action : *actionPtr)
		{
			m_actions->actions.emplace_back(std::move(action));
		}
	}

	if (m_onSettingsChangedCallback)
		m_onSettingsChangedCallback();
}

void CActionsEditorView::OnDestroy()
{
	stopRecoring();

	// Free memory
	for (auto item = 0, size = m_listActions.GetItemCount(); item < size; ++item)
	{
		std::unique_ptr<TableItemType> actionPtr((TableItemType*)m_listActions.GetItemDataPtr(item));
	}

	CFormView::OnDestroy();
}

BOOL CActionsEditorView::PreTranslateMessage(MSG* pMsg)
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

	return CFormView::PreTranslateMessage(pMsg);
}

inline void CActionsEditorView::startRecording()
{
	EXT_ASSERT(m_mouseMoveSubscriptionId == -1 && m_keyPressedSubscriptionId == -1 && m_mouseMoveDirectXSubscriptionId == -1);
	EXT_ASSERT(m_recordedActions.empty());

	RecordModes recordMode = (RecordModes)m_comboRecordMode.GetCurSel();

	// If we record mouse movements we can receive 3 mouse move event in 1 ms and to avoid delays we will add them in a table after finishing the recording

	// TODO add text in table or place recording banner
	switch (recordMode)
	{
	case RecordModes::eRecordCursorPosition:
	case RecordModes::eRecordCursorDelta:
		m_mouseMoveSubscriptionId = InputManager::AddMouseMoveHandler([&, recordMode](const POINT& position, const POINT& delta) {
			if (delta.x == 0 && delta.y == 0)
				return;

			std::unique_lock l(mutex);
			// EXT_ASSERT(m_lastActionTime.has_value());
			auto curTime = std::chrono::steady_clock::now();
			auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - *m_lastActionTime).count();
			m_lastActionTime = std::move(curTime);

			switch (recordMode)
			{
			case RecordModes::eRecordCursorPosition:
				m_recordedActions.emplace_back(Action::NewMousePosition(position.x, position.y, (int)delay));
				break;
			case RecordModes::eRecordCursorDelta:
				m_recordedActions.emplace_back(Action::NewMouseMove(delta.x, delta.y, (int)delay, false));
				break;
			default:
				EXT_UNREACHABLE();
			}
		});
		break;
	case RecordModes::eRecordCursorDeltaWithDirectX:
		m_mouseMoveDirectXSubscriptionId = InputManager::AddDirectInputMouseMoveHandler(AfxGetInstanceHandle(), [&](const POINT& delta) {
			//EXT_TRACE() << "Mouse move " << delta.x << "," << delta.y;

			std::unique_lock l(mutex);
			//EXT_ASSERT(m_lastActionTime.has_value());
			auto curTime = std::chrono::steady_clock::now();
			auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - *m_lastActionTime).count();
			m_lastActionTime = std::move(curTime);
			m_recordedActions.emplace_back(Action::NewMouseMove(delta.x, delta.y, (int)delay, true));
		});

		if (m_mouseMoveDirectXSubscriptionId == -1)
		{
			EXT_ASSERT(false);
			// TODO show warning and cancel recording
		}
		break;
	}

	m_keyPressedSubscriptionId = InputManager::AddKeyOrMouseHandler([&, recordMode](WORD vkCode, bool isDown) -> bool {
		EXT_ASSERT(m_lastActionTime.has_value());

		switch (vkCode)
		{
		case VK_LBUTTON:
		{
			CPoint cursor;
			::GetCursorPos(&cursor);

			// Process stop recording and OK click
			auto window = ::WindowFromPoint(cursor);
			if (window == m_buttonRecord.m_hWnd || (!!GetOwner()->GetDlgItem(IDOK) && window == GetOwner()->GetDlgItem(IDOK)->m_hWnd))
			{
				onSettingsChanged();
				return false;
			}

			[[fallthrough]];
		}
		default:
		{
			std::unique_lock l(mutex);
			auto curTime = std::chrono::steady_clock::now();
			auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - *m_lastActionTime).count();
			m_lastActionTime = std::move(curTime);

			if (recordMode == RecordModes::eNoMouseMovements)
				addAction(Action::NewAction(vkCode, isDown, int(delay)));
			else
				m_recordedActions.emplace_back(Action::NewAction(vkCode, isDown, int(delay)));

			// allow action in other programs except ours
			auto activeWindow = GetForegroundWindow();
			return (activeWindow == this ||
				activeWindow == GetOwner() ||
				activeWindow == AfxGetMainWnd());
		}
		}
	});
}

inline void CActionsEditorView::stopRecoring()
{
	if (m_keyPressedSubscriptionId != -1)
		InputManager::RemoveKeyOrMouseHandler(m_keyPressedSubscriptionId);
	if (m_mouseMoveSubscriptionId != -1)
		InputManager::RemoveMouseMoveHandler(m_mouseMoveSubscriptionId);
	if (m_mouseMoveDirectXSubscriptionId != -1)
		InputManager::RemoveDirectInputMouseMoveHandler(m_mouseMoveDirectXSubscriptionId);
	m_mouseMoveDirectXSubscriptionId = m_keyPressedSubscriptionId = m_mouseMoveSubscriptionId = -1;

	addActions(m_recordedActions);
	m_recordedActions.clear();
}

void CActionsEditorView::addActions(const std::list<Action>& actions)
{
	if (actions.empty())
		return;

	// TODO replace cursor on waiting?

	m_listActions.LockWindowUpdate();
	m_listActions.SetRedraw(FALSE);

	int item = m_listActions.GetLastSelectedItem();
	if (item == -1)
		item = m_listActions.GetItemCount() - 1;

	if (m_showMovementsTogether)
	{
		std::list<Action> mousePosChangeActions;
		auto addActions = [&]() {
			if (mousePosChangeActions.empty())
				return;

			long long delay = 0, sumX = 0, sumY = 0;
			for (const auto& action : mousePosChangeActions)
			{
				delay += action.delayInMilliseconds;
				sumX += action.mouseX;
				sumY += action.mouseY;
			}

			CString text;
			if (mousePosChangeActions.size() == 1)
				text = mousePosChangeActions.front().ToString().c_str();
			else if (mousePosChangeActions.front().type == Action::Type::eCursorPosition)
				text = CString(L"United: ") + mousePosChangeActions.back().ToString().c_str();
			else if (mousePosChangeActions.front().type == Action::Type::eMouseMoveDirectInput)
				text.Format(L"United: DirectX mouse move(%lld,%lld)", sumX, sumY);
			else
			{
				EXT_ASSERT(mousePosChangeActions.front().type == Action::Type::eMouseMove);
				text.Format(L"United: Move mouse(%lld,%lld)", sumX, sumY);
			}

			item = m_listActions.InsertItem(item + 1, std::to_wstring(delay).c_str());
			m_listActions.SetItemText(item, Columns::eAction, text.GetString());
			m_listActions.SelectItem(item);

			std::unique_ptr<TableItemType> actionPtr = std::make_unique<TableItemType>(std::move(mousePosChangeActions));
			m_listActions.SetItemDataPtr(item, actionPtr.release());

			mousePosChangeActions = {};
		};

		for (const auto& action : actions)
		{
			switch (action.type)
			{
			case Action::Type::eKeyOrMouseAction:
			case Action::Type::eRunScript:
				addActions();
				item = addAction(item + 1, action);
				break;
			case Action::Type::eMouseMove:
			case Action::Type::eMouseMoveDirectInput:
			case Action::Type::eCursorPosition:
				if (!mousePosChangeActions.empty() && mousePosChangeActions.back().type != action.type)
					addActions();
				
				mousePosChangeActions.emplace_back(action);
				break;
			default:
				EXT_UNREACHABLE();
			}
		}

		addActions();
	}
	else
	{
		for (const auto& action : actions)
		{
			item = addAction(item + 1, action);
		}
	}

	m_listActions.UnlockWindowUpdate();
	m_listActions.SetRedraw(TRUE);
	m_listActions.Invalidate();
}

void CActionsEditorView::addAction(Action&& action)
{
	int item = m_listActions.GetLastSelectedItem() + 1;
	if (item == 0)
		item = m_listActions.GetItemCount();
	addAction(item, std::move(action));
}

int CActionsEditorView::addAction(int item, Action action)
{
	item = m_listActions.InsertItem(item, std::to_wstring(action.delayInMilliseconds).c_str());
	m_listActions.SetItemText(item, Columns::eAction, action.ToString().c_str());
	std::unique_ptr<TableItemType> actionPtr = std::make_unique<TableItemType>(TableItemType{ std::move(action) });
	m_listActions.SetItemDataPtr(item, actionPtr.release());
	m_listActions.SelectItem(item);
	return item;
}

void CActionsEditorView::updateButtonStates()
{
	auto selectionExist = m_listActions.GetSelectedCount() > 0;
	m_buttonDelete.EnableWindow(selectionExist);
	m_buttonMoveUp.EnableWindow(selectionExist);
	m_buttonMoveDown.EnableWindow(selectionExist);

	bool noUnitedItemsInSelection = selectionExist;
	if (m_checkUniteMouseMovements.GetCheck() == TRUE)
	{
		std::vector<int> selectedActions = m_listActions.GetSelectedItems();
		for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end && noUnitedItemsInSelection; ++it)
		{
			TableItemType* actionPtr((TableItemType*)m_listActions.GetItemDataPtr(*it));
			if (actionPtr->size() != 1)
			{
				noUnitedItemsInSelection = false;
				break;
			}
		}
	}
	m_buttonEditDelay.EnableWindow(noUnitedItemsInSelection);
}

void CActionsEditorView::OnBnClickedButtonAdd()
{
	std::optional<Action> action = CAddActionDlg::EditAction(this);
	if (!action.has_value())
		return;

	addAction(std::move(*action));

	onSettingsChanged();
}

void CActionsEditorView::OnBnClickedButtonRemove()
{
	std::vector<int> selectedActions = m_listActions.GetSelectedItems();
	EXT_ASSERT(!selectedActions.empty());

	m_listActions.LockWindowUpdate();
	m_listActions.SetRedraw(FALSE);

	for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end; ++it)
	{
		std::unique_ptr<TableItemType> actionPtr((TableItemType*)m_listActions.GetItemDataPtr(*it));
		m_listActions.DeleteItem(*it);
	}
	m_listActions.SelectItem(selectedActions.back() - int(selectedActions.size()) + 1);

	m_listActions.UnlockWindowUpdate();
	m_listActions.SetRedraw(TRUE);
	m_listActions.Invalidate();

	onSettingsChanged();
}

void CActionsEditorView::OnBnClickedButtonEditDelay()
{
	std::vector<int> selectedActions = m_listActions.GetSelectedItems();
	EXT_ASSERT(!selectedActions.empty());

	auto newDalay = CEditValueDlg::EditValue(this, L"Enter delay, ms", selectedActions.front(), false);
	if (!newDalay.has_value())
		return;

	auto text = std::to_wstring(newDalay.value());
	for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end; ++it)
	{
		TableItemType* actionPtr((TableItemType*)m_listActions.GetItemDataPtr(*it));
		EXT_ASSERT(actionPtr->size() == 1);

		auto& action = actionPtr->front();
		action.delayInMilliseconds = newDalay.value();

		m_listActions.SetItemText(*it, Columns::eDelay, text.c_str());
	}

	onSettingsChanged();
}

void CActionsEditorView::OnBnClickedButtonRecord()
{
	CFormView::KillTimer(kRecordingTimer0Id);
	CFormView::KillTimer(kRecordingTimer1Id);
	CFormView::KillTimer(kRecordingTimer2Id);

	if (m_buttonRecord.GetCheck() == BST_CHECKED)
	{
		CFormView::SetTimer(kRecordingTimer0Id, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Recording starts in 3...");
		m_buttonRecord.SetIcon(IDI_ICON_STOP_RECORDING, Alignment::LeftCenter);
	}
	else
	{
		m_buttonRecord.SetWindowTextW(L"Saving actions...");
		stopRecoring();
		m_buttonRecord.SetWindowTextW(L"Record actions");
		m_buttonRecord.SetIcon(IDI_ICON_START_RECORDING, Alignment::LeftCenter);
		onSettingsChanged();
	}
}

void CActionsEditorView::OnTimer(UINT_PTR nIDEvent)
{
	CFormView::KillTimer(nIDEvent);

	switch (nIDEvent)
	{
	case kRecordingTimer0Id:
		CFormView::SetTimer(kRecordingTimer1Id, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Recording starts in 2...");
		break;
	case kRecordingTimer1Id:
		CFormView::SetTimer(kRecordingTimer2Id, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Recording starts in 1...");
		break;
	case kRecordingTimer2Id:
		m_buttonRecord.SetWindowTextW(L"Stop recording");
		m_lastActionTime = std::chrono::steady_clock::now();
		startRecording();
		break;
	default:
		EXT_ASSERT(false) << "Unknown event id " << nIDEvent;
		break;
	}

	CFormView::OnTimer(nIDEvent);
}

void CActionsEditorView::OnBnClickedButtonMoveUp()
{
	m_listActions.MoveSelectedItems(true);
	onSettingsChanged();
}

void CActionsEditorView::OnBnClickedButtonMoveDown()
{
	m_listActions.MoveSelectedItems(false);
	onSettingsChanged();
}

void CActionsEditorView::OnLvnItemchangedListActions(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (pNMLV->uChanged & LVIF_STATE)
	{
		if ((pNMLV->uNewState & LVIS_SELECTED) == (pNMLV->uOldState & LVIS_SELECTED))
		{
			// selection changed
			updateButtonStates();
		}
	}

	*pResult = 0;
}

void CActionsEditorView::OnEnChangeEditRandomizeDelays()
{
	CString text;
	m_editRandomizeDelays.GetWindowTextW(text);
	std::wistringstream str(text.GetString());
	str >> m_actions->randomizeDelays;

	onSettingsChanged();
}

void CActionsEditorView::OnBnClickedCheckUniteMovements()
{
	m_showMovementsTogether = m_checkUniteMouseMovements.GetCheck();

	const auto size = m_listActions.GetItemCount();
	m_actions->actions.clear();
	for (auto item = 0; item < size; ++item)
	{
		std::unique_ptr<TableItemType> actionPtr((TableItemType*)m_listActions.GetItemDataPtr(item));
		for (auto&& action : *actionPtr)
		{
			m_actions->actions.emplace_back(std::move(action));
		}
	}
	m_listActions.DeleteAllItems();

	addActions(m_actions->actions);
}

IMPLEMENT_DYNAMIC(CActionsEditDlg, CDialogEx)

BEGIN_MESSAGE_MAP(CActionsEditDlg, CDialogEx)
END_MESSAGE_MAP()

CActionsEditDlg::CActionsEditDlg(CWnd* pParent, Actions& actions)
	: CDialogEx(IDD_DIALOG_EDIT_ACTIONS, pParent)
	, m_actions(actions)
{
}

BOOL CActionsEditDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CCreateContext  ctx;
	// members we do not use, so set them to null
	ctx.m_pNewViewClass = RUNTIME_CLASS(CActionsEditorView);
	ctx.m_pNewDocTemplate = NULL;
	ctx.m_pLastView = NULL;
	ctx.m_pCurrentFrame = NULL;

	CFrameWnd* pFrameWnd = (CFrameWnd*)this;
	CFormView* pView = (CFormView*)pFrameWnd->CreateView(&ctx);
	EXT_EXPECT(pView && pView->GetSafeHwnd() != NULL);
	pView->OnInitialUpdate();
	m_editorView = dynamic_cast<CActionsEditorView*>(pView);
	EXT_ASSERT(m_editorView);

	CWnd* placeholder = GetDlgItem(IDC_STATIC_ACTIONS_EDITOR_PLACEHOLDER);

	CRect editorRect;
	placeholder->GetWindowRect(editorRect);
	ScreenToClient(editorRect);

	CSize requiredEditorRect = m_editorView->GetTotalSize();

	CRect windowRect;
	GetWindowRect(windowRect);
	CSize extraSpace = requiredEditorRect - editorRect.Size();
	windowRect.right += extraSpace.cx;
	windowRect.bottom += extraSpace.cy;
	MoveWindow(windowRect);

	editorRect.right = editorRect.left + requiredEditorRect.cx;
	editorRect.bottom = editorRect.top + requiredEditorRect.cy;
	m_editorView->MoveWindow(editorRect);
	m_editorView->SetOwner(this);

	m_editorView->Init(m_actions, nullptr, false);

	placeholder->ShowWindow(SW_HIDE);
	m_editorView->ShowWindow(SW_SHOW);

	LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);

	return TRUE;
}

std::optional<Actions> CActionsEditDlg::ExecModal(CWnd* pParent, const Actions& currentActions)
{
	Actions actions = currentActions;
	if (CActionsEditDlg(pParent, actions).DoModal() != IDOK)
		return std::nullopt;

	return actions;
}
