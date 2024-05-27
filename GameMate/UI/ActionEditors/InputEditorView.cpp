#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "core/events.h"

#include "InputManager.h"
#include "InputEditorView.h"

namespace {

constexpr auto kTimerIdActionsSubscription = 1;
constexpr auto kSubscribeToActionInMs = 300;
constexpr auto kTimerIdKeyTextChage = 2;

} // namespace

CInputEditorBaseView::CInputEditorBaseView()
	: ActionsEditor(IDD_VIEW_EDIT_INPUT)
{
}

void CInputEditorBaseView::DoDataExchange(CDataExchange* pDX)
{
	ActionsEditor::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_ACTION, m_editAction);
}

BEGIN_MESSAGE_MAP(CInputEditorBaseView, ActionsEditor)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

void CInputEditorBaseView::OnInitialUpdate()
{
	ActionsEditor::OnInitialUpdate();

	m_editAction.SetWindowTextW(GetActionText().c_str());
	m_editAction.HideCaret();

	ext::send_event(&IKeyHandlerBlocker::OnBlockHandler);
}

void CInputEditorBaseView::OnTimer(UINT_PTR nIDEvent)
{
	KillTimer(nIDEvent);

	switch (nIDEvent)
	{
	case kTimerIdActionsSubscription:
		m_keyPressedSubscriptionId = InputManager::AddKeyOrMouseHandler([&](WORD vkCode, bool isDown) -> bool {
			auto owner = GetOwner();
			if (owner != GetForegroundWindow())
				return false;

			switch (vkCode)
			{
			case VK_LBUTTON:
			{
				CPoint cursor;
				::GetCursorPos(&cursor);

				// Don't store click on parent controls like Ok button or combobox with mode switchers
				auto window = CWnd::FromHandle(::WindowFromPoint(cursor));

				wchar_t className[64];
				GetClassName(window->m_hWnd, className, 64);

				if ((window && window->GetParent() == owner && !window->IsKindOf(RUNTIME_CLASS(CStatic))) ||
					std::wstring(className) != L"ComboLBox")
				//if (window != GetParent()->GetDlgItem(IDOK)->m_hWnd)
				{
					return false;
				}

				OnVkCodeAction(vkCode, isDown);

				// Set timer to change displayed text because user might hit close button with mouse
				// and if we change text immediatly the user will see Mouse down as a key text which wasn't his last input
				KillTimer(kTimerIdKeyTextChage);
				SetTimer(kTimerIdKeyTextChage, 50, nullptr);

				m_editAction.HideCaret();
				return false;
			}
			}

			OnVkCodeAction(vkCode, isDown);
			m_editAction.SetWindowTextW(GetActionText().c_str());

			return true;
		});
		break;
	case kTimerIdKeyTextChage:
		m_editAction.SetWindowTextW(GetActionText().c_str());
		break;
	default:
		ASSERT(FALSE);
		break;
	}

	ActionsEditor::OnTimer(nIDEvent);
}

void CInputEditorBaseView::OnDestroy()
{
	KillTimer(kTimerIdActionsSubscription);
	if (m_keyPressedSubscriptionId != -1)
		InputManager::RemoveKeyOrMouseHandler(m_keyPressedSubscriptionId);
	ActionsEditor::OnDestroy();

	ext::send_event(&IKeyHandlerBlocker::OnUnblockHandler);
}

void CInputEditorBaseView::SetAction(const Action& action)
{
	m_currentAction = action;
	m_editAction.SetWindowTextW(GetActionText().c_str());
}

Action CInputEditorBaseView::GetAction()
{
	return m_currentAction;
}

void CInputEditorBaseView::OnShowWindow(BOOL bShow, UINT nStatus)
{
	ActionsEditor::OnShowWindow(bShow, nStatus);

	if (!bShow)
	{
		if (m_keyPressedSubscriptionId != -1)
			InputManager::RemoveKeyOrMouseHandler(m_keyPressedSubscriptionId);
		m_keyPressedSubscriptionId = -1;
	}
	else
		SetTimer(kTimerIdActionsSubscription, kSubscribeToActionInMs, nullptr);
}

IMPLEMENT_DYNCREATE(CActionEditorView, CInputEditorBaseView)

void CActionEditorView::OnInitialUpdate()
{
	CInputEditorBaseView::OnInitialUpdate();

	SetWindowText(L"Action editor");
}

std::wstring CActionEditorView::GetActionText() const
{
	if (m_currentAction.vkCode == kNotSetVkCode)
		return L"Enter action...";
	else
		return m_currentAction.ToString();
}

void CActionEditorView::OnVkCodeAction(WORD vkCode, bool down)
{
	m_currentAction.vkCode = vkCode;
	m_currentAction.down = down;
}

bool CActionEditorView::CanClose() const
{
	if (m_currentAction.vkCode == kNotSetVkCode &&
		(::MessageBox(m_hWnd, L"You need to enter action, press any key or mouse button", L"No action selected", MB_OKCANCEL) == IDOK))
	{
		return false;
	}

	return true;
}

IMPLEMENT_DYNCREATE(CBindEditorView, CInputEditorBaseView)

void CBindEditorView::OnInitialUpdate()
{
	CInputEditorBaseView::OnInitialUpdate();

	SetWindowText(L"Bind editor");

	//TODO GetDlgItem(IDC_STATIC_DESCRIPTION)->SetWindowTextW(L"Enter mouse or keyboard action");
}

std::wstring CBindEditorView::GetActionText() const
{
	if (m_currentBind.vkCode == kNotSetVkCode)
		return L"Enter bind...";
	else
		return m_currentBind.ToString();
}

void CBindEditorView::OnVkCodeAction(WORD vkCode, bool down)
{
	m_currentBind.UpdateBind(vkCode, down);
}

bool CBindEditorView::CanClose() const
{
	if (m_currentBind.vkCode == kNotSetVkCode &&
		(::MessageBox(m_hWnd, L"You need to enter bind, press any key or mouse button", L"No bind selected", MB_OKCANCEL) == IDOK))
	{
		return false;
	}

	return true;
}

void CBindEditorView::SetAction(const Action&)
{
	EXT_ASSERT(false);
}

Action CBindEditorView::GetAction()
{
	EXT_ASSERT(false);
	return {};
}

void CBindEditorView::SetBind(Bind bind)
{
	m_currentBind = bind;
	m_editAction.SetWindowTextW(GetActionText().c_str());
}

Bind CBindEditorView::GetBind() const
{
	return m_currentBind;
}
