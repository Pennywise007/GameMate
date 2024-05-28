#include "pch.h"
#include "resource.h"
#include <afxtoolbarimages.h>
#include <vector>
#include <string>

#include <ext/thread/invoker.h>

#include "CursorPositionEditorView.h"

IMPLEMENT_DYNCREATE(CCursorPositionEditorView, ActionsEditor)

CCursorPositionEditorView::CCursorPositionEditorView()
	: ActionsEditor(IDD_VIEW_EDIT_MOUSE_MOVE)
{
	m_action.type = Action::Type::eMouseMove;
}

void CCursorPositionEditorView::DoDataExchange(CDataExchange* pDX)
{
	ActionsEditor::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_MOUSE_POSITION_SELECT, m_buttonMousePositionSelect);
	DDX_Control(pDX, IDC_EDIT_MOUSE_POSITION_X, m_editMousePositionX);
	DDX_Control(pDX, IDC_EDIT_MOUSE_POSITION_Y, m_editMousePositionY);
}

BEGIN_MESSAGE_MAP(CCursorPositionEditorView, ActionsEditor)
	ON_BN_CLICKED(IDC_BUTTON_MOUSE_POSITION_SELECT, &CCursorPositionEditorView::OnBnClickedButtonMousePositionSelect)
END_MESSAGE_MAP()

void CCursorPositionEditorView::OnInitialUpdate()
{
	ActionsEditor::OnInitialUpdate();

	m_editMousePositionX.SetUseOnlyIntegersValue();
	m_editMousePositionX.UsePositiveDigitsOnly();
	m_editMousePositionY.SetUseOnlyIntegersValue();
	m_editMousePositionY.UsePositiveDigitsOnly();

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
	m_toolWindow.SetLabel(L"Click to select point or ESC to exit");
}

void CCursorPositionEditorView::OnDestroy()
{
	ActionsEditor::OnDestroy();

	ASSERT(m_keyPressedSubscriptionId == -1 && m_mouseMoveSubscriptionId == -1);
	m_cursorReplacingWindow.DestroyWindow();
}

void CCursorPositionEditorView::OnBnClickedButtonMousePositionSelect()
{
	CWnd* wnd = this;
	wnd->ShowWindow(SW_HIDE);
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
			CPoint cursor;
			::GetCursorPos(&cursor);
			SetAction(Action::NewMouseMove(cursor.x, cursor.y, m_action.delayInMilliseconds));
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

				CWnd* wnd = this;
				wnd->ShowWindow(SW_SHOW);
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

bool CCursorPositionEditorView::CanClose() const
{
	// TODO
	return true;
}

void CCursorPositionEditorView::SetAction(const Action& action)
{
	m_action.mouseX = action.mouseX;
	m_action.mouseY = action.mouseY;

	updateMousePositionControlsStates();
}

Action CCursorPositionEditorView::GetAction()
{
	CString text;
	m_editMousePositionX.GetWindowTextW(text);
	m_action.mouseX = text.IsEmpty() ? 0 : std::stol(text.GetString());

	m_editMousePositionY.GetWindowTextW(text);
	m_action.mouseY = text.IsEmpty() ? 0 : std::stol(text.GetString());

	return m_action;
}

void CCursorPositionEditorView::updateMousePositionControlsStates()
{
	m_editMousePositionX.SetWindowTextW(std::to_wstring(m_action.mouseX).c_str());
	m_editMousePositionY.SetWindowTextW(std::to_wstring(m_action.mouseY).c_str());
}

void CCursorPositionEditorView::syncCrosshairWindowWithCursor()
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
