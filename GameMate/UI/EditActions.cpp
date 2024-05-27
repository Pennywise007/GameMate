#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "AddActionDlg.h"
#include "EditActions.h"
#include "InputManager.h"

#include <Controls/Layout/Layout.h>
#include <Controls/Tooltip/ToolTip.h>

#include <ext/core/check.h>

IMPLEMENT_DYNCREATE(CActionsEditorView, CFormView)

BEGIN_MESSAGE_MAP(CActionsEditorView, CFormView)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CActionsEditorView::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CActionsEditorView::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &CActionsEditorView::OnBnClickedButtonRecord)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_UP, &CActionsEditorView::OnBnClickedButtonMoveUp)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_DOWN, &CActionsEditorView::OnBnClickedButtonMoveDown)
	ON_EN_CHANGE(IDC_EDIT_RANDOMIZE_DELAYS, &CActionsEditorView::OnEnChangeEditRandomizeDelays)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MACROSES, &CActionsEditorView::OnLvnItemchangedListActions)
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
			onSettingsChanged();

			return true;
		});
	m_listActions.setSubItemEditorController(Columns::eAction,
		[&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			Action* action = (Action*)m_listActions.GetItemDataPtr(pParams->iItem);

			auto newAction = CAddActionDlg::EditAction(this, *action);
			if (newAction.has_value())
			{
				*action = std::move(*newAction);
				m_listActions.SetItemText(pParams->iItem, Columns::eAction, action->ToString().c_str());
				onSettingsChanged();
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

	LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);
}

void CActionsEditorView::onSettingsChanged()
{
	const auto size = m_listActions.GetItemCount();

	m_actions->actions.resize(size);
	auto it = m_actions->actions.begin();
	for (auto item = 0; item < size; ++item, ++it)
	{
		*it = *(Action*)m_listActions.GetItemDataPtr(item);
	}

	if (m_onSettingsChangedCallback)
		m_onSettingsChangedCallback();
}

void CActionsEditorView::OnDestroy()
{
	unsubscribeFromInputEvents();

	CFormView::OnDestroy();

	// Free memory
	for (auto item = 0, size = m_listActions.GetItemCount(); item < size; ++item)
	{
		std::unique_ptr<Action> actionPtr((Action*)m_listActions.GetItemDataPtr(item));
	}
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

inline void CActionsEditorView::subscribeOnInputEvents()
{
	EXT_ASSERT(m_mouseMoveSubscriptionId == -1 && m_keyPressedSubscriptionId == -1);

	if (m_captureMousePositions)
	{
		m_mouseMoveSubscriptionId = InputManager::AddMouseMoveHandler([&](const POINT& position, const POINT& delta) {
			EXT_ASSERT(m_lastActionTime.has_value());

			auto curTime = std::chrono::steady_clock::now();
			auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - *m_lastActionTime).count();

			addAction(Action::NewMouseMove(position.x, position.y, int(delay)));
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
			if (window == m_buttonRecord.m_hWnd || window == CFormView::GetDlgItem(IDOK)->m_hWnd)
			{
				onSettingsChanged();
				return false;
			}

			[[fallthrough]];
		}
		default:
		{
			auto curTime = std::chrono::steady_clock::now();
			auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - *m_lastActionTime).count();

			addAction(Action::NewAction(vkCode, isDown, int(delay)));
			m_lastActionTime = std::move(curTime);
			return true;
		}
		}
		});
}

inline void CActionsEditorView::unsubscribeFromInputEvents()
{
	if (m_keyPressedSubscriptionId != -1)
		InputManager::RemoveKeyOrMouseHandler(m_keyPressedSubscriptionId);
	if (m_mouseMoveSubscriptionId != -1)
		InputManager::RemoveMouseMoveHandler(m_mouseMoveSubscriptionId);
	m_keyPressedSubscriptionId = m_mouseMoveSubscriptionId = -1;
}

void CActionsEditorView::addAction(Action action)
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

void CActionsEditorView::updateButtonStates()
{
	auto selectionExist = m_listActions.GetSelectedCount() > 0;
	m_buttonDelete.EnableWindow(selectionExist);
	m_buttonMoveUp.EnableWindow(selectionExist);
	m_buttonMoveDown.EnableWindow(selectionExist);
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

	for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end; ++it)
	{
		std::unique_ptr<Action> actionPtr((Action*)m_listActions.GetItemDataPtr(*it));
		m_listActions.DeleteItem(*it);
	}
	m_listActions.SelectItem(selectedActions.back() - int(selectedActions.size()) + 1);
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
		unsubscribeFromInputEvents();
		m_buttonRecord.SetWindowTextW(L"Record actions");
		m_buttonRecord.SetIcon(IDI_ICON_START_RECORDING, Alignment::LeftCenter);
		m_lastActionTime.reset();
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
		m_buttonRecord.SetWindowTextW(L"Cancel recording");
		subscribeOnInputEvents();
		m_lastActionTime = std::chrono::steady_clock::now();
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
	updateButtonStates();
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
	// memebers we do not use, so set them to null
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

	m_editorView->Init(m_actions, nullptr, false);

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
