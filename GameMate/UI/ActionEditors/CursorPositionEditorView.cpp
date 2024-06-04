#include "pch.h"
#include "resource.h"
#include <afxtoolbarimages.h>
#include <vector>
#include <string>

#include <ext/thread/invoker.h>

#include <Controls/Layout/Layout.h>

#include "CursorPositionEditorView.h"

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

	CBitmap bitmap;
	try
	{
		CPngImage pngImage;
		EXT_CHECK(pngImage.Load(IDB_PNG_CURSOR_POSITION_SELECTION, AfxGetResourceHandle()))
			<< "Failed to load cursor position image";
		EXT_CHECK(bitmap.Attach(pngImage.Detach())) << "Failed to load resouce";
	}
	catch (...)
	{
		MessageBox(ext::ManageExceptionText(L"").c_str(), L"Failed to load image", MB_ICONERROR);
	}

	m_cursorReplacingWindow.Create(std::move(bitmap));

	m_toolWindow.Create(m_cursorReplacingWindow.m_hWnd, true);
	m_toolWindow.SetLabel(L"Left click to select point or ESC to cancel");
}

void CMouseMovementEditorView::OnDestroy()
{
	ActionsEditor::OnDestroy();

	ASSERT(m_keyPressedSubscriptionId == -1 && m_mouseMoveSubscriptionId == -1);
	m_cursorReplacingWindow.DestroyWindow();
}

void CMouseMovementEditorView::OnBnClickedButtonMousePositionSelect()
{
	ShowWindow(SW_HIDE);
	CWnd* wnd = this;
	do
	{
		wnd = wnd->GetParent();
		wnd->ShowWindow(SW_HIDE);
	} while (wnd != AfxGetMainWnd());

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

				m_cursorReplacingWindow.SetWindowPos(nullptr, 0, 0, 0, 0,
					SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOREPOSITION);
				m_toolWindow.ShowWindow(SW_HIDE);

				ShowWindow(SW_SHOW);
				CWnd* wnd = this;
				do
				{
					wnd = wnd->GetParent();
					wnd->ShowWindow(SW_SHOW);
				} while (wnd != AfxGetMainWnd());
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
	ASSERT(m_keyPressedSubscriptionId != -1 && m_mouseMoveSubscriptionId != -1);
	
	CPoint requiredPoint = InputManager::GetMousePosition();
	auto currentRect = m_cursorReplacingWindow.GetWindowRect();
	currentRect.OffsetRect(requiredPoint - currentRect.CenterPoint());

	m_cursorReplacingWindow.SetWindowPos(nullptr,
		currentRect.left,
		currentRect.top,
		0,
		0,
		SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

	const CSize windowSize = m_toolWindow.GetWindowSize();
	m_toolWindow.SetWindowPos(nullptr, currentRect.right, currentRect.bottom, windowSize.cx, windowSize.cy,
		SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	CString text;
	text.Format(L"x=%d, y=%d", requiredPoint.x, requiredPoint.y);
	m_toolWindow.SetDescription(text);
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
	static const auto screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	static const auto screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

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

IMPLEMENT_DYNCREATE(CMouseMoveEditorView, CMouseMovementEditorView)

CMouseMoveEditorView::CMouseMoveEditorView()
{
	m_action.type = Action::Type::eMouseMove;
}

void CMouseMoveEditorView::OnInitialUpdate()
{
	CMouseMovementEditorView::OnInitialUpdate();

	// TODO add check logic processing, remember about set action function
	m_buttonMousePositionSelect.ShowWindow(SW_HIDE);
}

void CMouseMoveEditorView::OnInitDone()
{
	CMouseMovementEditorView::OnInitDone();

	// hiding select mouse pos button
	/*CRect rect;
	m_buttonMousePositionSelect.GetWindowRect(rect);
	m_buttonMousePositionSelect.ShowWindow(SW_HIDE);
	long offset = rect.Height() + 10;

	HWND hWndChild = ::GetWindow(m_hWnd, GW_CHILD);
	while (hWndChild)
	{
		CRect rect;
		::GetWindowRect(hWndChild, &rect);
		ScreenToClient(rect);
		::SetWindowPos(hWndChild, NULL, rect.left, rect.top - offset, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		hWndChild = ::GetWindow(hWndChild, GW_HWNDNEXT);
	}

	CSize size = GetTotalSize();
	size.cy -= offset;
	SetScrollSizes(MM_TEXT, size);

	LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);*/
}

bool CMouseMoveEditorView::CanClose() const
{
	return true;
}
