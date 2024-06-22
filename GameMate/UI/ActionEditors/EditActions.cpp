#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "core/events.h"

#include "UI/Dlg/AddActionDlg.h"
#include "UI/ActionEditors/EditActions.h"
#include "UI/Dlg/EditValueDlg.h"
#include "InputManager.h"

#include <ext/thread/invoker.h>

#include <Controls/Layout/Layout.h>
#include <Controls/Tooltip/ToolTip.h>

#include <ext/core/check.h>
#include <ext/serialization/iserializable.h>

namespace {

constexpr int kRandomDelayColumnWidth = 110;
constexpr int kDelayColumnWidth = 80;

using MouseRecordMode = Actions::MouseRecordMode;
// Saved in table item data type
using TableItemType = std::list<Action>;
// Special delay value which should be replaced during recording handling
constexpr unsigned kDelayPlaceholder = -1;
// Timer settings
constexpr UINT kTimerInterval1Sec = 1000;
enum TimerIds : UINT
{
	eRecordingCounter0 = 0,
	eRecordingCounter1,
	eRecordingCounter2,
};
// Clipboard actions struct
struct ClipboardData
{
	REGISTER_SERIALIZABLE_OBJECT();
	DECLARE_SERIALIZABLE_FIELD(std::list<Action>, actions);
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
	ON_BN_CLICKED(IDC_CHECK_UNITE_MOVEMENTS, &CActionsEditorView::OnBnClickedCheckUniteMovements)
	ON_EN_CHANGE(IDC_EDIT_RANDOMIZE_DELAYS, &CActionsEditorView::OnEnChangeEditRandomizeDelays)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MACROSES, &CActionsEditorView::OnLvnItemchangedListActions)
	ON_CBN_SELENDOK(IDC_COMBO_RECORD_MODE, &CActionsEditorView::OnCbnSelendokComboRecordMode)
	ON_BN_CLICKED(IDC_CHECK_RANDOMIZE_DELAY, &CActionsEditorView::OnBnClickedCheckRandomizeDelay)
END_MESSAGE_MAP()

CActionsEditorView::CActionsEditorView()
	: CFormView(IDD_VIEW_ACTIONS_EDITOR)
	, m_actionsTableSubItemEditor(std::make_shared<ActionsTableSubItemEditorController>(this))
{}

void CActionsEditorView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_RECORD, m_buttonRecord);
	DDX_Control(pDX, IDC_CHECK_UNITE_MOVEMENTS, m_checkUniteMouseMovements);
	DDX_Control(pDX, IDC_COMBO_RECORD_MODE, m_comboMouseRecordMode);
	DDX_Control(pDX, IDC_STATIC_MAUSE_MOVE_HELP, m_staticMouseMoveHelp);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_buttonAdd);
	DDX_Control(pDX, IDC_BUTTON_REMOVE, m_buttonDelete);
	DDX_Control(pDX, IDC_BUTTON_MOVE_UP, m_buttonMoveUp);
	DDX_Control(pDX, IDC_BUTTON_MOVE_DOWN, m_buttonMoveDown);
	DDX_Control(pDX, IDC_BUTTON_EDIT_DELAY, m_buttonEditDelay);
	DDX_Control(pDX, IDC_LIST_MACROSES, m_listActions);
	DDX_Control(pDX, IDC_EDIT_RANDOMIZE_DELAYS, m_editRandomizeDelays);
	DDX_Control(pDX, IDC_STATIC_DELAY_HELP, m_staticDelayHelp);
	DDX_Control(pDX, IDC_CHECK_RANDOMIZE_DELAY, m_checkRandomizeDelay);
}

BOOL CActionsEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
	auto res = CFormView::PreCreateWindow(cs);
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	return res;
}

void CActionsEditorView::Init(Actions& actions, OnSettingsChangedCallback callback)
{
	m_actions = &actions;
	m_onSettingsChangedCallback = std::move(callback);

	const auto initHelpStatic = [](CStatic& control, const wchar_t* tooltip) {
		CRect rect;
		control.GetClientRect(rect);

		HICON icon;
		auto res = SUCCEEDED(::LoadIconWithScaleDown(NULL, IDI_INFORMATION, rect.Height(), rect.Height(), &icon));
		EXT_ASSERT(res);

		control.ModifyStyle(0, SS_ICON);
		control.SetIcon(icon);
		controls::SetTooltip(control, tooltip);
	};
	initHelpStatic(m_staticDelayHelp, L"Some game guards may check if you execute actions with the same intervals and can ban you.\n"
		L"To avoid this you can set a delay value in ms which will be applied to the action delay randomly.\n"
		L"Formula: action_delay ± random(randomize_dalay)");
	initHelpStatic(m_staticMouseMoveHelp, L"Switch mouse recording mode. Mouse mode descriptions:\n"
		L"Record cursor position - record cursor coordinates on every mouse move\n"
		L"Record cursor delta - record cursor movement on every mouse move\n"
		L"Record DirectX cursor delta - special mode for games, record raw cursor movement");

	m_editRandomizeDelays.UsePositiveDigitsOnly();
	m_editRandomizeDelays.SetMinMaxLimits(0, 100);

	m_checkRandomizeDelay.SetCheck(m_actions->enableRandomDelay ? BST_CHECKED : BST_UNCHECKED);

	std::wostringstream str;
	str << m_actions->randomizeDelayMs;
	m_editRandomizeDelays.SetWindowTextW(str.str().c_str());

	// TODO find better icons, maybe https://www.iconfinder.com/icons/22947/red_stop_icon?coming-from=related-results
	m_buttonRecord.SetImageOffset(0);
	m_buttonRecord.SetIcon(IDI_ICON_START_RECORDING, Alignment::LeftCenter);

	m_checkUniteMouseMovements.SetCheck(m_actions->showMouseMovementsUnited);

	const auto initButton = [](CIconButton& button, UINT bitmapId, const wchar_t* tooltip) {
		button.SetBitmap(bitmapId, Alignment::CenterCenter);
		button.SetWindowTextW(L"");
		controls::SetTooltip(button, tooltip);
	};
	initButton(m_buttonAdd, IDB_PNG_ADD, L"Add action");
	initButton(m_buttonDelete, IDB_PNG_DELETE, L"Delete selected action");
	initButton(m_buttonMoveUp, IDB_PNG_ARROW_UP, L"Move selected up");
	initButton(m_buttonMoveDown, IDB_PNG_ARROW_DOWN, L"Move selected down");
	initButton(m_buttonEditDelay, IDB_PNG_DELAY, L"Set delay for all selected items");

	m_comboMouseRecordMode.InsertString((int)MouseRecordMode::eNoMouseMovements, L"Do not record mouse");
	m_comboMouseRecordMode.InsertString((int)MouseRecordMode::eRecordCursorPosition, L"Record cursor position");
	m_comboMouseRecordMode.InsertString((int)MouseRecordMode::eRecordCursorDelta, L"Record cursor delta");
	m_comboMouseRecordMode.InsertString((int)MouseRecordMode::eRecordCursorDeltaWithDirectX, L"Record DirectX cursor delta");
	m_comboMouseRecordMode.SetCurSel((int)m_actions->mouseRecordMode);

	CRect rect;
	m_listActions.GetClientRect(rect);

	m_columnDelayIndex = 0;
	m_columnActionIndex = 1;
	m_listActions.InsertColumn(m_columnDelayIndex, L"Delay(ms)", LVCFMT_CENTER, kDelayColumnWidth);
	m_listActions.InsertColumn(m_columnActionIndex, L"Action", LVCFMT_LEFT, rect.Width() - kDelayColumnWidth);

	LVCOLUMN colInfo;
	colInfo.mask = LVCF_FMT;
	m_listActions.GetColumn(m_columnDelayIndex, &colInfo);
	colInfo.fmt |= LVCFMT_FIXED_WIDTH | LVCFMT_CENTER;
	m_listActions.SetColumn(m_columnDelayIndex, &colInfo);

	// Set editors
	for (int i = 0; i < 3; ++i)
	{
		m_listActions.setSubItemEditorController(i, m_actionsTableSubItemEditor);
	}

	if (m_actions->enableRandomDelay)
		switchRandomizeDelayMode();
	else
		m_listActions.SetProportionalResizingColumns({ m_columnActionIndex });

	// Adding items to table
	addActions(m_actions->actions, false);

	updateButtonStates();

	LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);
}

void CActionsEditorView::onSettingsChanged(bool changesInActionsList)
{
	if (changesInActionsList)
	{
		const auto size = m_listActions.GetItemCount();
		m_actions->actions.clear();
		for (auto item = 0; item < size; ++item)
		{
			TableItemType* actionPtr((TableItemType*)m_listActions.GetItemDataPtr(item));
			for (const auto& action : *actionPtr)
			{
				m_actions->actions.emplace_back(action);
			}
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
	case WM_KEYUP:
		switch (pMsg->wParam)
		{
		case VK_DELETE:
		case VK_BACK:
			if (m_buttonDelete.IsWindowEnabled())
				OnBnClickedButtonRemove();
			break;
		case 'C':
			if (InputManager::IsKeyPressed(VK_CONTROL))
				copyItemsToClipboard();
			break;
		case 'V':
			if (InputManager::IsKeyPressed(VK_CONTROL))
				pasteItemsFromClipboard();
			break;
		}
		break;
	}

	return CFormView::PreTranslateMessage(pMsg);
}

void CActionsEditorView::switchRandomizeDelayMode()
{
	if (!m_actions->enableRandomDelay)
	{
		m_listActions.ModifyExtendedStyle(LVS_EX_CHECKBOXES, 0);

		m_columnDelayIndex = 0;
		m_columnActionIndex = 1;

		m_listActions.DeleteColumn(0);
	}
	else
	{
		m_listActions.SetRedraw(FALSE);

		m_ignoreCheckChanged = true;
		EXT_DEFER(m_ignoreCheckChanged = false);

		m_listActions.ModifyExtendedStyle(0, LVS_EX_CHECKBOXES);

		constexpr int kRandomDelayColumnWidth = 110;
		ENSURE(m_listActions.InsertColumn(0, L"Randomize delay", LVCFMT_CENTER, kRandomDelayColumnWidth) == 0);

		m_columnDelayIndex = 1;
		m_columnActionIndex = 2;

		const auto columnsCount = m_listActions.GetHeaderCtrl()->GetItemCount();
		std::list<int> checkedItems;
		for (int item = 0, countRows = m_listActions.GetItemCount(); item < countRows; ++item)
		{
			const TableItemType* actions = (TableItemType*)m_listActions.GetItemDataPtr(item);
			EXT_ASSERT(actions);

			bool allActionsChecked = true;
			for (auto it = actions->cbegin(), end = actions->cend(); allActionsChecked && it != end; ++it)
			{
				allActionsChecked &= it->randomizeDelay;
			}
			if (allActionsChecked)
				m_listActions.SetCheck(item, TRUE);

			// move data to the right
			for (int subItem = columnsCount - 2; subItem >= 0; --subItem)
			{
				m_listActions.SetItemText(item, subItem + 1, m_listActions.GetItemText(item, subItem));
			}
			m_listActions.SetItemText(item, 0, L"");
		}

		m_listActions.SetRedraw(TRUE);
		m_listActions.Invalidate();
	}

	// Restore columns width
	m_listActions.SetColumnWidth(m_columnDelayIndex, kDelayColumnWidth);

	int allColumnsWidthExceptActions = 0;
	for (int column = 0, count = m_listActions.GetHeaderCtrl()->GetItemCount(); column < count; ++column)
	{
		if (column != m_columnActionIndex)
			allColumnsWidthExceptActions += m_listActions.GetColumnWidth(column);
	}

	CRect rect;
	m_listActions.GetClientRect(rect);
	m_listActions.SetColumnWidth(m_columnActionIndex, rect.Width() - allColumnsWidthExceptActions);
	m_listActions.SetProportionalResizingColumns({ m_columnActionIndex });
}

inline void CActionsEditorView::startRecording()
{
	EXT_ASSERT(m_mouseMoveSubscriptionId == -1 && m_keyPressedSubscriptionId == -1 && m_mouseMoveDirectXSubscriptionId == -1);
	EXT_ASSERT(m_recordedActions.empty() && !m_lastActionTime.has_value());

	ext::send_event(&IKeyHandlerBlocker::OnBlockHandler);

	// Show recording banner if we won't push items to table immediately
	m_listActions.ShowWindow(canDynamicallyAddRecordedActions() ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC_RECORDING_OVERLAY)->ShowWindow(canDynamicallyAddRecordedActions() ? SW_HIDE : SW_SHOW);

	switch (m_actions->mouseRecordMode)
	{
	case MouseRecordMode::eRecordCursorPosition:
	case MouseRecordMode::eRecordCursorDelta:
		m_mouseMoveSubscriptionId = InputManager::AddMouseMoveHandler([&](const POINT& position, const POINT& delta) {
			if (delta.x == 0 && delta.y == 0)
				return;

			switch (m_actions->mouseRecordMode)
			{
			case MouseRecordMode::eRecordCursorPosition:
				handleRecordedAction(Action::NewMousePosition(position.x, position.y, kDelayPlaceholder));
				break;
			case MouseRecordMode::eRecordCursorDelta:
				handleRecordedAction(Action::NewMouseMove(delta.x, delta.y, kDelayPlaceholder, false));
				break;
			default:
				EXT_UNREACHABLE();
			}
		});
		break;
	case MouseRecordMode::eRecordCursorDeltaWithDirectX:
		m_mouseMoveDirectXSubscriptionId = InputManager::AddDirectInputMouseMoveHandler(AfxGetInstanceHandle(), [&](const POINT& delta) {
			handleRecordedAction(Action::NewMouseMove(delta.x, delta.y, kDelayPlaceholder, true));
		});

		if (m_mouseMoveDirectXSubscriptionId == -1)
		{
			MessageBox(L"Failed to subscribe on DirectX input, check if DirectX is installed or don't record DirectX mouse movements",
				L"Failed to subscribe on DirectX input", MB_ICONERROR);
			// Stop recording
			m_buttonRecord.SetCheck(BST_UNCHECKED);
			OnBnClickedButtonRecord();
			return;
		}
		break;
	}

	m_keyPressedSubscriptionId = InputManager::AddKeyOrMouseHandler([&](WORD vkCode, bool isDown) -> bool {
		const static auto kCurrentProcessId = GetCurrentProcessId();
		
		switch (vkCode)
		{
		case VK_LBUTTON:
		{
			CPoint cursor;
			::GetCursorPos(&cursor);

			// Process stop recording and OK click
			auto window = ::WindowFromPoint(cursor);
			if (window == m_buttonRecord.m_hWnd || (!!GetOwner()->GetDlgItem(IDOK) && window == GetOwner()->GetDlgItem(IDOK)->m_hWnd))
				return false;

			handleRecordedAction(Action::NewAction(vkCode, isDown, kDelayPlaceholder));

			// allow clicks in other programs except ours
			if (CWnd* wnd = WindowFromPoint(cursor); wnd)
			{
				DWORD dwProcessId;
				if (GetWindowThreadProcessId(wnd->m_hWnd, &dwProcessId) != 0 && dwProcessId == kCurrentProcessId)
					return true;
			}

			return false;
		}
		default:
		{
			handleRecordedAction(Action::NewAction(vkCode, isDown, kDelayPlaceholder));

			// allow action in other programs except ours
			if (CWnd* wnd = GetForegroundWindow(); wnd)
			{
				DWORD dwProcessId;
				if (GetWindowThreadProcessId(wnd->m_hWnd, &dwProcessId) != 0 && dwProcessId == kCurrentProcessId)
					return true;
			}
			
			return false;
		}
		}
	});
}

inline void CActionsEditorView::stopRecoring()
{
	ext::send_event(&IKeyHandlerBlocker::OnUnblockHandler);

	if (m_keyPressedSubscriptionId != -1)
		InputManager::RemoveKeyOrMouseHandler(m_keyPressedSubscriptionId);
	if (m_mouseMoveSubscriptionId != -1)
		InputManager::RemoveMouseMoveHandler(m_mouseMoveSubscriptionId);
	if (m_mouseMoveDirectXSubscriptionId != -1)
		InputManager::RemoveDirectInputMouseMoveHandler(m_mouseMoveDirectXSubscriptionId);
	m_mouseMoveDirectXSubscriptionId = m_keyPressedSubscriptionId = m_mouseMoveSubscriptionId = -1;
	m_lastActionTime.reset();

	if (!m_recordedActions.empty())
	{
		// If we show a recording overlay it means that we don't dynamically update the table and we need to clear selection
		if (GetDlgItem(IDC_STATIC_RECORDING_OVERLAY)->IsWindowVisible())
			m_listActions.ClearSelection();
		addActions(m_recordedActions, true);
		m_recordedActions.clear();
	}
	m_listActions.ShowWindow(SW_SHOW);
	GetDlgItem(IDC_STATIC_RECORDING_OVERLAY)->ShowWindow(SW_HIDE);

	onSettingsChanged(true);
}

bool CActionsEditorView::canDynamicallyAddRecordedActions() const
{
	return m_actions->showMouseMovementsUnited || m_actions->mouseRecordMode == MouseRecordMode::eNoMouseMovements;
}

void CActionsEditorView::handleRecordedAction(Action&& action)
{
	EXT_ASSERT(action.delayInMilliseconds == kDelayPlaceholder) << "Delay should be added in this funcion";
	EXT_DEFER(EXT_ASSERT(action.delayInMilliseconds != kDelayPlaceholder) << "Delay placeholder expected to be replaced");

	auto curTime = std::chrono::steady_clock::now();

	std::unique_lock l(m_recordingActionsMutex);
	if (!m_lastActionTime.has_value())
		m_lastActionTime = curTime;
	EXT_ASSERT(curTime >= m_lastActionTime);
	auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - *m_lastActionTime).count();
	m_lastActionTime = std::move(curTime);

	action.delayInMilliseconds = (unsigned)delay;

	// If we record mouse movements we can receive 3 mouse move event in 1 ms and to avoid delays we will add them in a table after finishing the recording
	if (canDynamicallyAddRecordedActions())
	{
		switch (action.type)
		{
		case Action::Type::eKeyOrMouseAction:
			// If we recorded some mouse movements, add them to the table before our action
			if (!m_recordedActions.empty())
			{
				addActions(m_recordedActions, true);
				m_recordedActions.clear();
			}
			addAction(std::move(action));
			break;
		case Action::Type::eCursorPosition:
		case Action::Type::eMouseMove:
		case Action::Type::eMouseMoveDirectInput:
			EXT_ASSERT(m_recordedActions.empty() || m_recordedActions.back().type == action.type);
			m_recordedActions.emplace_back(std::move(action));
			break;
		default:
			EXT_UNREACHABLE();
		}
	}
	else
		m_recordedActions.emplace_back(std::move(action));
}

void CActionsEditorView::addActions(const std::list<Action>& actions, bool lockRedraw)
{
	if (actions.empty())
		return;

	m_ignoreSelectionChanged = true;
	EXT_DEFER(m_ignoreSelectionChanged = false);
	EXT_DEFER(updateButtonStates());

	if (lockRedraw)
		m_listActions.SetRedraw(FALSE);

	int item = m_listActions.GetLastSelectedItem();
	if (item == -1)
		item = m_listActions.GetItemCount() - 1;

	if (m_actions->showMouseMovementsUnited)
	{
		std::list<Action> mousePosChangeActions;
		// Uniting same actions in a group and show common name
		auto addActions = [&]() {
			if (mousePosChangeActions.empty())
				return;

			long long delay = 0, sumX = 0, sumY = 0;
			bool allRandomizeDelay = true;
			for (const auto& action : mousePosChangeActions)
			{
				delay += action.delayInMilliseconds;
				sumX += action.mouseX;
				sumY += action.mouseY;
				allRandomizeDelay &= action.randomizeDelay;
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

			if (m_actions->enableRandomDelay)
			{
				item = m_listActions.InsertItem(item + 1, L"");
				m_listActions.SetCheck(item, allRandomizeDelay);
				m_listActions.SetItemText(item, m_columnDelayIndex, std::to_wstring(delay).c_str());
			}
			else
				item = m_listActions.InsertItem(item + 1, std::to_wstring(delay).c_str());

			m_listActions.SetItemText(item, m_columnActionIndex, text.GetString());
			std::unique_ptr<TableItemType> actionPtr = std::make_unique<TableItemType>(std::move(mousePosChangeActions));
			m_listActions.SetItemDataPtr(item, actionPtr.release());
			m_listActions.SelectItem(item);

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

	if (lockRedraw)
	{
		m_listActions.SetRedraw(TRUE);
		m_listActions.Invalidate();
	}
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
	if (m_actions->enableRandomDelay)
	{
		item = m_listActions.InsertItem(item, L"");
		m_listActions.SetCheck(item, action.randomizeDelay);
		m_listActions.SetItemText(item, m_columnDelayIndex, std::to_wstring(action.delayInMilliseconds).c_str());
	}
	else
		item = m_listActions.InsertItem(item, std::to_wstring(action.delayInMilliseconds).c_str());

	m_listActions.SetItemText(item, m_columnActionIndex, action.ToString().c_str());
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

void CActionsEditorView::copyItemsToClipboard()
{
	using namespace ext::serializer;

	ClipboardData clipboardData;
	for (auto i : m_listActions.GetSelectedItems())
	{
		TableItemType* actionPtr((TableItemType*)m_listActions.GetItemDataPtr(i));
		std::copy(actionPtr->cbegin(), actionPtr->cend(), std::back_inserter(clipboardData.actions));
	}

	std::wstring text;
	EXT_EXPECT(SerializeObject(Factory::TextSerializer(text), clipboardData));

	if (!OpenClipboard())
		return;
	EXT_DEFER(CloseClipboard());

	HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, sizeof(WCHAR) * (text.size() + 1));
	if (!hClipboardData)
		return;

	WCHAR* pchData = (WCHAR*)GlobalLock(hClipboardData);
	if (!pchData)
		return;
	EXT_DEFER(GlobalUnlock(hClipboardData));
	wcscpy_s(pchData, text.size() + 1, text.c_str());

	SetClipboardData(CF_UNICODETEXT, hClipboardData);
}

void CActionsEditorView::pasteItemsFromClipboard()
{
	std::wstring text;
	{
		if (!OpenClipboard())
			return;
		EXT_DEFER(CloseClipboard());

		HANDLE hData = GetClipboardData(CF_UNICODETEXT);
		if (!hData)
			return;

		const wchar_t* pwszText = static_cast<wchar_t*>(GlobalLock(hData));
		if (!pwszText)
			return;
		text = pwszText;
		GlobalUnlock(hData);
	}

	using namespace ext::serializer;

	ClipboardData copiedData;
	try
	{
		if (!DeserializeObject(Factory::TextDeserializer(text), copiedData))
			return;
	}
	catch (...)
	{
		return;
	}

	if (copiedData.actions.empty())
		return;

	m_listActions.ClearSelection();
	addActions(copiedData.actions, true);
	onSettingsChanged(true);
}

void CActionsEditorView::OnBnClickedButtonAdd()
{
	std::optional<Action> action = CAddActionDlg::EditAction(this);
	if (!action.has_value())
		return;

	addAction(std::move(*action));

	onSettingsChanged(true);
}

void CActionsEditorView::OnBnClickedButtonRemove()
{
	std::vector<int> selectedActions = m_listActions.GetSelectedItems();
	EXT_ASSERT(!selectedActions.empty());

	m_listActions.SetRedraw(FALSE);

	m_ignoreSelectionChanged = true;
	for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end; ++it)
	{
		std::unique_ptr<TableItemType> actionPtr((TableItemType*)m_listActions.GetItemDataPtr(*it));
		m_listActions.DeleteItem(*it);
	}
	m_ignoreSelectionChanged = false;
	m_listActions.SelectItem(selectedActions.back() - int(selectedActions.size()) + 1);
	updateButtonStates();

	m_listActions.SetRedraw(TRUE);
	m_listActions.Invalidate();

	onSettingsChanged(true);
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

		m_listActions.SetItemText(*it, m_columnDelayIndex, text.c_str());
	}

	onSettingsChanged(true);
}

void CActionsEditorView::OnCbnSelendokComboRecordMode()
{
	m_actions->mouseRecordMode = (Actions::MouseRecordMode)m_comboMouseRecordMode.GetCurSel();
	onSettingsChanged(false);
}

void CActionsEditorView::OnBnClickedButtonRecord()
{
	CFormView::KillTimer(TimerIds::eRecordingCounter0);
	CFormView::KillTimer(TimerIds::eRecordingCounter1);
	CFormView::KillTimer(TimerIds::eRecordingCounter2);

	if (m_buttonRecord.GetCheck() == BST_CHECKED)
	{
		CFormView::SetTimer(TimerIds::eRecordingCounter0, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Recording starts in 3...");
		m_buttonRecord.SetIcon(IDI_ICON_STOP_RECORDING, Alignment::LeftCenter);
	}
	else
	{
		m_buttonRecord.SetWindowTextW(L"Saving actions...");
		stopRecoring();
		m_buttonRecord.SetWindowTextW(L"Record actions");
		m_buttonRecord.SetIcon(IDI_ICON_START_RECORDING, Alignment::LeftCenter);
	}
}

void CActionsEditorView::OnTimer(UINT_PTR nIDEvent)
{
	CFormView::KillTimer(nIDEvent);

	switch (nIDEvent)
	{
	case TimerIds::eRecordingCounter0:
		CFormView::SetTimer(TimerIds::eRecordingCounter1, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Recording starts in 2...");
		break;
	case TimerIds::eRecordingCounter1:
		CFormView::SetTimer(TimerIds::eRecordingCounter2, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Recording starts in 1...");
		break;
	case TimerIds::eRecordingCounter2:
		m_buttonRecord.SetWindowTextW(L"Stop recording");
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
	onSettingsChanged(true);
}

void CActionsEditorView::OnBnClickedButtonMoveDown()
{
	m_listActions.MoveSelectedItems(false);
	onSettingsChanged(true);
}

void CActionsEditorView::OnBnClickedCheckUniteMovements()
{
	m_actions->showMouseMovementsUnited = m_checkUniteMouseMovements.GetCheck();

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

	addActions(m_actions->actions, true);
	onSettingsChanged(false);
}

void CActionsEditorView::OnLvnItemchangedListActions(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (pNMLV->uChanged & LVIF_STATE)
	{
		if ((pNMLV->uNewState & LVIS_SELECTED) != (pNMLV->uOldState & LVIS_SELECTED) && !m_ignoreSelectionChanged)
		{
			// selection changed
			updateButtonStates();
		}
		if (m_actions->enableRandomDelay && pNMLV->uNewState & LVIS_STATEIMAGEMASK && !m_ignoreCheckChanged)
		{
			auto newState = pNMLV->uNewState & LVIS_STATEIMAGEMASK;
			constexpr auto CHECKED_STATE = 0x2000;
			constexpr auto UNCHECKED_STATE = 0x1000;
			if ((newState & CHECKED_STATE) != 0 || (newState & UNCHECKED_STATE) != 0)
			{
				bool itemWasChecked = newState & CHECKED_STATE;
				TableItemType* actions = (TableItemType*)m_listActions.GetItemDataPtr(pNMLV->iItem);
				if (actions)
				{
					// We might receive this event during adding items to the table
					for (auto& action : *actions)
					{
						action.randomizeDelay = itemWasChecked;
					}
					onSettingsChanged(true);
				}
			}
		}
	}

	*pResult = 0;
}

void CActionsEditorView::OnBnClickedCheckRandomizeDelay()
{
	m_actions->enableRandomDelay = m_checkRandomizeDelay.GetCheck() == BST_CHECKED;
	switchRandomizeDelayMode();

	onSettingsChanged(true);
}

void CActionsEditorView::OnEnChangeEditRandomizeDelays()
{
	CString text;
	m_editRandomizeDelays.GetWindowTextW(text);
	std::wistringstream str(text.GetString());
	str >> m_actions->randomizeDelayMs;

	onSettingsChanged(false);
}

IMPLEMENT_DYNAMIC(CActionsEditDlg, CDialogEx)

BEGIN_MESSAGE_MAP(CActionsEditDlg, CDialogEx)
END_MESSAGE_MAP()

CActionsEditDlg::CActionsEditDlg(CWnd* pParent, Actions& actions)
	: CDialogEx(IDD_DIALOG_EDIT_ACTIONS, pParent)
	, m_actions(actions)
{}

void CActionsEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_editDescription);
}

BOOL CActionsEditDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CCreateContext ctx = {};
	ctx.m_pNewViewClass = RUNTIME_CLASS(CActionsEditorView);

	CFrameWnd* pFrameWnd = (CFrameWnd*)this;
	CFormView* pView = (CFormView*)pFrameWnd->CreateView(&ctx);
	EXT_EXPECT(pView && pView->GetSafeHwnd() != NULL);
	pView->OnInitialUpdate();
	pView->ModifyStyle(0, WS_TABSTOP);

	CActionsEditorView* editorView = dynamic_cast<CActionsEditorView*>(pView);
	EXT_ASSERT(editorView);

	CWnd* placeholder = GetDlgItem(IDC_STATIC_ACTIONS_EDITOR_PLACEHOLDER);

	CRect editorRect;
	placeholder->GetWindowRect(editorRect);
	ScreenToClient(editorRect);

	CSize requiredEditorRect = editorView->GetTotalSize();

	CRect windowRect;
	GetWindowRect(windowRect);
	CSize extraSpace = requiredEditorRect - editorRect.Size();
	windowRect.right += extraSpace.cx;
	windowRect.bottom += extraSpace.cy;
	MoveWindow(windowRect);

	editorRect.right = editorRect.left + requiredEditorRect.cx;
	editorRect.bottom = editorRect.top + requiredEditorRect.cy;
	editorView->MoveWindow(editorRect);
	editorView->SetOwner(this);

	editorView->Init(m_actions, nullptr);

	placeholder->ShowWindow(SW_HIDE);
	editorView->ShowWindow(SW_SHOW);

	m_editDescription.SetCueBanner(L"Actions description(optional)...");
	m_editDescription.SetWindowTextW(m_actions.description.c_str());

	LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);

	return TRUE;
}

void CActionsEditDlg::OnOK()
{
	CString text;
	m_editDescription.GetWindowTextW(text);
	m_actions.description = text;

	CDialogEx::OnOK();
}

std::optional<Actions> CActionsEditDlg::ExecModal(CWnd* pParent, const Actions& currentActions)
{
	Actions actions = currentActions;
	if (CActionsEditDlg(pParent, actions).DoModal() != IDOK)
		return std::nullopt;

	return actions;
}

std::shared_ptr<CWnd> ActionsTableSubItemEditorController::createEditorControl(CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
{
	auto* list = dynamic_cast<CListGroupCtrl*>(pList);
	EXT_ASSERT(list);

	if (pParams->iSubItem == m_editorView->m_columnDelayIndex)
	{
		auto edit = std::make_shared<CEditBase>();

		// Don't edit united rows
		TableItemType* itemData = (TableItemType*)list->GetItemDataPtr(pParams->iItem);
		if (itemData->size() > 1)
		{
			::MessageBox(parentWindow->m_hWnd, L"Stop items uniting and edit them separately", L"Can't edit united items", MB_OK);
			return nullptr;
		}

		edit->Create(controls::list::widgets::SubItemEditorControllerBase::getStandartEditorWndStyle() |
			ES_CENTER | CBS_AUTOHSCROLL,
			CRect(), parentWindow, 0);
		edit->UsePositiveDigitsOnly();
		edit->SetUseOnlyIntegersValue();

		CString curSubItemText = pList->GetItemText(pParams->iItem, pParams->iSubItem);
		edit->SetWindowTextW(curSubItemText);

		return std::shared_ptr<CWnd>(edit);
	}
	else if (pParams->iSubItem == m_editorView->m_columnActionIndex)
	{
		// Don't edit united rows
		TableItemType* itemData = (TableItemType*)list->GetItemDataPtr(pParams->iItem);
		if (itemData->size() > 1)
		{
			::MessageBox(parentWindow->m_hWnd, L"Stop items uniting and edit them separately", L"Can't edit united items", MB_OK);
			return nullptr;
		}

		Action& action = itemData->front();

		auto newAction = CAddActionDlg::EditAction(parentWindow, action);
		if (newAction.has_value())
		{
			action = std::move(*newAction);
			list->SetItemText(pParams->iItem, m_editorView->m_columnActionIndex, action.ToString().c_str());
			m_editorView->onSettingsChanged(true);
		}
	}

	return nullptr;
}

void ActionsTableSubItemEditorController::onEndEditSubItem(CListCtrl* pList, CWnd* editorControl, const LVSubItemParams* pParams, bool bAcceptResult)
{
	ASSERT(pParams->iSubItem == m_editorView->m_columnDelayIndex);

	if (!bAcceptResult)
		return;

	CString currentEditorText;
	editorControl->GetWindowTextW(currentEditorText);

	auto* list = dynamic_cast<CListGroupCtrl*>(pList);
	EXT_ASSERT(list);

	TableItemType* action = (TableItemType*)list->GetItemDataPtr(pParams->iItem);
	EXT_ASSERT(action->size() == 1);

	action->front().delayInMilliseconds = _wtoi(currentEditorText);
	m_editorView->onSettingsChanged(true);

	SubItemEditorControllerBase::onEndEditSubItem(pList, editorControl, pParams, bAcceptResult);
}
