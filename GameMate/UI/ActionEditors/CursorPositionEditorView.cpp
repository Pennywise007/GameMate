#include "pch.h"
#include "resource.h"
#include <afxtoolbarimages.h>
#include <vector>
#include <string>

#include <ext/thread/invoker.h>

#include <Controls/Layout/Layout.h>

#include "UI/ActionEditors/CursorPositionEditorView.h"

CMouseMovementEditorView::CMouseMovementEditorView()
	: ActionsEditor(IDD_VIEW_EDIT_MOUSE_MOVE)
{
	m_action.type = Action::Type::eMouseMove;
}

void CMouseMovementEditorView::DoDataExchange(CDataExchange* pDX)
{
	ActionsEditor::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_MOUSE_POSITION_SELECT, m_buttonMousePositionSelect);
	DDX_Control(pDX, IDC_EDIT_MOUSE_POSITION_X, m_editMousePositionX);
	DDX_Control(pDX, IDC_EDIT_MOUSE_POSITION_Y, m_editMousePositionY);
	DDX_Control(pDX, IDC_CHECK_USE_DIRECT_INPUT, m_checkUseDirectInput);
}

BEGIN_MESSAGE_MAP(CMouseMovementEditorView, ActionsEditor)
	ON_BN_CLICKED(IDC_BUTTON_MOUSE_POSITION_SELECT, &CMouseMovementEditorView::OnBnClickedButtonMousePositionSelect)
END_MESSAGE_MAP()

void CMouseMovementEditorView::OnInitialUpdate()
{
	ActionsEditor::OnInitialUpdate();

	m_editMousePositionX.SetUseOnlyIntegersValue();
	m_editMousePositionY.SetUseOnlyIntegersValue();

	updateMousePositionControlsStates();
}

void CMouseMovementEditorView::OnDestroy()
{
	ActionsEditor::OnDestroy();

	ASSERT(m_keyPressedSubscriptionId != -1 && m_mouseMoveSubscriptionId != -1 &&
		!::IsWindow(m_cursorReplacingWindow.m_hWnd) && !::IsWindow(m_toolWindow.m_hWnd));
}

void CMouseMovementEditorView::OnBnClickedButtonMousePositionSelect()
{
	if (HGDIOBJ obj = m_cursorBitmap; !obj)
	{
		try
		{
			CPngImage pngImage;
			EXT_CHECK(pngImage.Load(IDB_PNG_CURSOR_POSITION_SELECTION, AfxGetResourceHandle()))
				<< "Failed to load cursor position image";
			EXT_CHECK(m_cursorBitmap.Attach(pngImage.Detach())) << "Failed to load resource";
		}
		catch (...)
		{
			MessageBox(ext::ManageExceptionText(L"").c_str(), L"Failed to load image", MB_ICONERROR);
			return;
		}
	}

	showProgramWindows(SW_HIDE);

	ASSERT(m_keyPressedSubscriptionId == -1 && m_mouseMoveSubscriptionId == -1 &&
		!::IsWindow(m_cursorReplacingWindow.m_hWnd) && !::IsWindow(m_toolWindow.m_hWnd));

	m_cursorReplacingWindow.Create(std::move(m_cursorBitmap));
	m_toolWindow.Create(m_cursorReplacingWindow.m_hWnd, true);
	m_toolWindow.SetLabel(L"Left click to select point or ESC to cancel");

	m_keyPressedSubscriptionId = InputManager::AddKeyOrMouseHandler([&](WORD vkCode, bool isDown) -> bool {
		switch (vkCode)
		{
		case VK_LBUTTON:
		{
			auto cursor = InputManager::GetMousePosition();
			SetAction(Action::NewMousePosition(cursor.x, cursor.y, m_action.delayInMilliseconds));
			[[fallthrough]];
		}
		case VK_ESCAPE:
		{
			ext::InvokeMethodAsync([&]() {
				InputManager::RemoveKeyOrMouseHandler(m_keyPressedSubscriptionId);
				m_keyPressedSubscriptionId = -1;
				InputManager::RemoveMouseMoveHandler(m_mouseMoveSubscriptionId);
				m_mouseMoveSubscriptionId = -1;

				m_cursorReplacingWindow.DestroyWindow();
				m_toolWindow.DestroyWindow();

				showProgramWindows(SW_SHOW);
			});
			return true;
		}
		}

		return false;
	});
	m_mouseMoveSubscriptionId = InputManager::AddMouseMoveHandler([&](const POINT& position, const POINT&) {
		syncCrosshairWindowWithCursor();
	});

	syncCrosshairWindowWithCursor();
}

void CMouseMovementEditorView::SetAction(const Action& action)
{
	m_action.mouseX = action.mouseX;
	m_action.mouseY = action.mouseY;
	m_action.delayInMilliseconds = action.delayInMilliseconds;

	updateMousePositionControlsStates();
}

Action CMouseMovementEditorView::GetAction()
{
	CString text;
	m_editMousePositionX.GetWindowTextW(text);
	m_action.mouseX = text.IsEmpty() ? 0 : std::stol(text.GetString());

	m_editMousePositionY.GetWindowTextW(text);
	m_action.mouseY = text.IsEmpty() ? 0 : std::stol(text.GetString());

	return m_action;
}

void CMouseMovementEditorView::updateMousePositionControlsStates()
{
	m_editMousePositionX.SetWindowTextW(std::to_wstring(m_action.mouseX).c_str());
	m_editMousePositionY.SetWindowTextW(std::to_wstring(m_action.mouseY).c_str());
}

void CMouseMovementEditorView::syncCrosshairWindowWithCursor()
{
	ASSERT(m_keyPressedSubscriptionId != -1 && m_mouseMoveSubscriptionId != -1 &&
		::IsWindow(m_cursorReplacingWindow.m_hWnd) && ::IsWindow(m_toolWindow.m_hWnd));

	const CPoint cursorPosition = InputManager::GetMousePosition();
	const auto cursorWindowSize = m_cursorReplacingWindow.GetWindowRect().Size();

	CPoint windowTopLeft;
	if (!GetPhysicalCursorPos(&windowTopLeft))
		windowTopLeft = cursorPosition;

	windowTopLeft.x -= LONG(cursorWindowSize.cx / 2.);
	windowTopLeft.y -= LONG(cursorWindowSize.cy / 2.);

	m_cursorReplacingWindow.SetWindowPos(nullptr,
		windowTopLeft.x,
		windowTopLeft.y,
		0,
		0,
		SWP_NOZORDER | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_SHOWWINDOW);

	CString text;
	text.Format(L"x=%d, y=%d", cursorPosition.x, cursorPosition.y);
	m_toolWindow.SetDescription(text);

	const CSize windowSize = m_toolWindow.GetWindowSize();
	m_toolWindow.SetWindowPos(nullptr,
		windowTopLeft.x + cursorWindowSize.cx,
		windowTopLeft.y + cursorWindowSize.cy,
		windowSize.cx,
		windowSize.cy,
		SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
}

void CMouseMovementEditorView::showProgramWindows(int nCmdShow)
{
	ShowWindow(nCmdShow);
	CWnd* wnd = this;
	do
	{
		wnd = wnd->GetParent();
		wnd->ShowWindow(nCmdShow);
	} while (wnd != AfxGetMainWnd());
}

IMPLEMENT_DYNCREATE(CCursorPositionEditorView, CMouseMovementEditorView)

CCursorPositionEditorView::CCursorPositionEditorView()
{
	m_action.type = Action::Type::eCursorPosition;
}

void CCursorPositionEditorView::OnInitialUpdate()
{
	CMouseMovementEditorView::OnInitialUpdate();

	m_checkUseDirectInput.ShowWindow(SW_HIDE);
	m_editMousePositionX.UsePositiveDigitsOnly();
	m_editMousePositionY.UsePositiveDigitsOnly();
}

bool CCursorPositionEditorView::CanClose() const
{
	static const long screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	static const long screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	if (m_action.mouseX < 0 || m_action.mouseX > screenWidth ||
		m_action.mouseY < 0 || m_action.mouseY > screenHeight)
	{
		if (::MessageBox(m_hWnd, std::string_swprintf(
			L"Current maximum screen size if (%d,%d), are you sure that you want to set mouse point(%d,%d) beyond its borders?",
			screenWidth, screenHeight, m_action.mouseX, m_action.mouseY).c_str(),
			L"Not valid mouse position selected", MB_OKCANCEL) == IDCANCEL)
			return false;
	}
	return true;
}

void CCursorPositionEditorView::SetAction(const Action& action)
{
	static const long screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	static const long screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	Action modifiedAction = action;
	modifiedAction.mouseX = std::clamp(modifiedAction.mouseX, 0l, screenWidth);
	modifiedAction.mouseY = std::clamp(modifiedAction.mouseY, 0l, screenHeight);
	CMouseMovementEditorView::SetAction(modifiedAction);
}

IMPLEMENT_DYNCREATE(CMouseMoveEditorView, CMouseMovementEditorView)

void CMouseMoveEditorView::OnInitialUpdate()
{
	CMouseMovementEditorView::OnInitialUpdate();

	m_buttonMousePositionSelect.ShowWindow(SW_HIDE);
}

bool CMouseMoveEditorView::CanClose() const
{
	return true;
}

void CMouseMoveEditorView::SetAction(const Action& action)
{
	CMouseMovementEditorView::SetAction(action);
	m_checkUseDirectInput.SetCheck(action.type == Action::Type::eMouseMoveDirectInput);
}

Action CMouseMoveEditorView::GetAction()
{
	m_action.type = m_checkUseDirectInput.GetCheck() ? Action::Type::eMouseMoveDirectInput : Action::Type::eMouseMove;
	return CMouseMovementEditorView::GetAction();
}
