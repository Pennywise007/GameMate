#include "pch.h"
#include "GameMate.h"
#include "afxdialogex.h"
#include "BaseKeyEditDlg.h"
#include "InputManager.h"

IMPLEMENT_DYNAMIC(CBaseKeyEditDlg, CDialogEx)

CBaseKeyEditDlg::CBaseKeyEditDlg(CWnd* pParent)
	: CDialogEx(IDD_DIALOG_ACTION_EDIT, pParent)
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

	m_keyPressedSubscriptionId = InputManager::AddKeyOrMouseHandler([&](WORD vkKey, bool isDown) -> bool {
		switch (vkKey)
		{
		case VK_LBUTTON:
			{
				CPoint cursor;
				::GetCursorPos(&cursor);

				// Don't store click on OK button
				auto window = ::WindowFromPoint(cursor);
				if (window != GetDlgItem(IDOK)->m_hWnd)
				{
					OnVkKeyAction(vkKey, isDown);
					m_editAction.SetWindowTextW(GetActionText().c_str());
				}

				m_editAction.HideCaret();
				return false;
			}
		}

		OnVkKeyAction(vkKey, isDown);
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

IMPLEMENT_DYNAMIC(CMacrosActionEditDlg, CBaseKeyEditDlg)

BEGIN_MESSAGE_MAP(CMacrosActionEditDlg, CBaseKeyEditDlg)
END_MESSAGE_MAP()

CMacrosActionEditDlg::CMacrosActionEditDlg(CWnd* pParent, std::optional<MacrosAction>& action)
	: CBaseKeyEditDlg(pParent)
	, m_currentAction(action)
{}

[[nodiscard]] std::optional<MacrosAction> CMacrosActionEditDlg::EditMacros(CWnd* pParent, const std::optional<MacrosAction>& editMacros)
{
	std::optional<MacrosAction> action = editMacros;
	if (CMacrosActionEditDlg(pParent, action).DoModal() != IDOK)
		return std::nullopt;

	return action;
}

std::wstring CMacrosActionEditDlg::GetActionText() const
{
	if (m_currentAction.has_value())
		return m_currentAction->ToString();

	return L"Enter action...";
}

void CMacrosActionEditDlg::OnVkKeyAction(WORD vkKey, bool down)
{
	auto currentDelay = 0u;
	if (m_currentAction.has_value())
		currentDelay = m_currentAction->delayInMilliseconds;
	m_currentAction.emplace(vkKey, down, currentDelay);
}

void CMacrosActionEditDlg::OnOK()
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

void CBindEditDlg::OnVkKeyAction(WORD vkKey, bool down)
{
	if (down)
		m_currentBind.emplace(vkKey);
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
