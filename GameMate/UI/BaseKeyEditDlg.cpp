#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "BaseKeyEditDlg.h"
#include "InputManager.h"

IMPLEMENT_DYNAMIC(CBaseKeyEditDlg, CDialogEx)

CBaseKeyEditDlg::CBaseKeyEditDlg(CWnd* pParent)
	: CDialogEx(IDD_DIALOG_BIND_EDIT, pParent)
{
}

void CBaseKeyEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_ACTION, m_editAction);
}

BEGIN_MESSAGE_MAP(CBaseKeyEditDlg, CDialogEx)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CBaseKeyEditDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_editAction.SetWindowTextW(GetActionText().c_str());
	m_editAction.HideCaret();

	m_keyPressedSubscriptionId = InputManager::AddKeyOrMouseHandler([&](WORD vkCode, bool isDown) -> bool {
		switch (vkCode)
		{
		case VK_LBUTTON:
			{
				CPoint cursor;
				::GetCursorPos(&cursor);

				// Don't store click on OK button
				auto window = ::WindowFromPoint(cursor);
				if (window != GetDlgItem(IDOK)->m_hWnd)
				{
					OnVkCodeAction(vkCode, isDown);
					m_editAction.SetWindowTextW(GetActionText().c_str());
				}

				m_editAction.HideCaret();
				return false;
			}
		}

		OnVkCodeAction(vkCode, isDown);
		m_editAction.SetWindowTextW(GetActionText().c_str());

		return true;
	});

	return TRUE;
}

void CBaseKeyEditDlg::OnDestroy()
{
	if (m_keyPressedSubscriptionId != -1)
		InputManager::RemoveKeyOrMouseHandler(m_keyPressedSubscriptionId);
	CDialogEx::OnDestroy();
}

IMPLEMENT_DYNAMIC(CActionEditDlg, CBaseKeyEditDlg)

BEGIN_MESSAGE_MAP(CActionEditDlg, CBaseKeyEditDlg)
END_MESSAGE_MAP()

CActionEditDlg::CActionEditDlg(CWnd* pParent, std::optional<Action>& action)
	: CBaseKeyEditDlg(pParent)
	, m_currentAction(action)
{}

BOOL CActionEditDlg::OnInitDialog()
{
	auto res = CBaseKeyEditDlg::OnInitDialog();
	SetWindowText(L"Action editor");
	return res;
}

[[nodiscard]] std::optional<Action> CActionEditDlg::EditAction(CWnd* pParent, const std::optional<Action>& _action)
{
	std::optional<Action> action = _action;
	if (CActionEditDlg(pParent, action).DoModal() != IDOK)
		return std::nullopt;

	return action;
}

std::wstring CActionEditDlg::GetActionText() const
{
	if (m_currentAction.has_value())
		return m_currentAction->ToString();

	return L"Enter action...";
}

void CActionEditDlg::OnVkCodeAction(WORD vkCode, bool down)
{
	auto currentDelay = 0u;
	if (m_currentAction.has_value())
		currentDelay = m_currentAction->delayInMilliseconds;
	m_currentAction.emplace(vkCode, down, currentDelay);
}

void CActionEditDlg::OnOK()
{
	if (!m_currentAction.has_value() &&
		(MessageBox(L"You need to enter action, press any key or mouse button", L"No action selected", MB_OKCANCEL) == IDOK))
	{
		return;
	}

	CBaseKeyEditDlg::OnOK();
}

IMPLEMENT_DYNAMIC(CBindEditDlg, CBaseKeyEditDlg)

BEGIN_MESSAGE_MAP(CBindEditDlg, CBaseKeyEditDlg)
END_MESSAGE_MAP()

CBindEditDlg::CBindEditDlg(CWnd* pParent, std::optional<Bind>& bind)
	: CBaseKeyEditDlg(pParent)
	, m_currentBind(bind)
{}

[[nodiscard]] std::optional<Bind> CBindEditDlg::EditBind(CWnd* pParent, const std::optional<Bind>& editBind)
{
	std::optional<Bind> bind = editBind;
	if (CBindEditDlg(pParent, bind).DoModal() != IDOK)
		return std::nullopt;

	return bind;
}

std::wstring CBindEditDlg::GetActionText() const
{
	if (m_currentBind.has_value())
		return m_currentBind->ToString();

	return L"Enter bind...";
}

void CBindEditDlg::OnVkCodeAction(WORD vkCode, bool down)
{
	if (down)
		m_currentBind.emplace(vkCode);
}

void CBindEditDlg::OnOK()
{
	if (!m_currentBind.has_value() &&
		(MessageBox(L"You need to enter bind, press any key or mouse button", L"No bind selected", MB_OKCANCEL) == IDOK))
	{
		return;
	}

	CBaseKeyEditDlg::OnOK();
}
