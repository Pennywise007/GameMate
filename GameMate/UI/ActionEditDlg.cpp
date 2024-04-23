#include "pch.h"
#include "GameMate.h"
#include "afxdialogex.h"
#include "ActionEditDlg.h"

IMPLEMENT_DYNAMIC(CActionEditDlg, CDialogEx)

CActionEditDlg::CActionEditDlg(CWnd* pParent, Type type, std::optional<Action>& action)
	: CDialogEx(IDD_DIALOG_ACTION_EDIT, pParent)
	, m_editingType(type)
	, m_currentAction(action)
{
}

void CActionEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_ACTION, m_editAction);
}

BEGIN_MESSAGE_MAP(CActionEditDlg, CDialogEx)
END_MESSAGE_MAP()

[[nodiscard]] std::optional<Bind> CActionEditDlg::EditBind(CWnd* pParent, const std::optional<Bind>& editBind)
{
	std::optional<Action> action;
	if (editBind.has_value())
		action = editBind->action;

	if (CActionEditDlg(pParent, Type::eBind, action).DoModal() != IDOK)
		return std::nullopt;

	Bind bind;
	bind.action = std::move(action.value());
	return bind;
}

[[nodiscard]] std::optional<MacrosAction> CActionEditDlg::EditMacros(CWnd* pParent, const std::optional<MacrosAction>& editMacros)
{
	std::optional<Action> action;
	if (editMacros.has_value())
		action = editMacros->action;

	if (CActionEditDlg(pParent, Type::eAction, action).DoModal() != IDOK)
		return std::nullopt;

	MacrosAction macrosAction;
	macrosAction.action = std::move(action.value());
	macrosAction.delayInMilliseconds = editMacros.value_or(MacrosAction{}).delayInMilliseconds;
	return macrosAction;
}

BOOL CActionEditDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (m_currentAction.has_value())
	{
		m_editAction.SetWindowTextW(GetActionText().c_str());
	}
	else
	{
		switch (m_editingType)
		{
		case Type::eAction:
			m_editAction.SetWindowTextW(L"Enter action");
			break;
		case Type::eBind:
			m_editAction.SetWindowTextW(L"Enter bind");
			break;
		default:
			ASSERT(false);
			break;
		}
	}

	return TRUE;
}

BOOL CActionEditDlg::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	{
		if (GetDlgItem(IDOK)->m_hWnd == pMsg->hwnd)
			break;

		[[fallthrough]];
	}
	default:
	{
		switch (m_editingType)
		{
		case Type::eAction:
			{
				if (auto action = MacrosAction::GetMacrosActionFromMessage(pMsg, 0); action.has_value())
				{
					m_editAction.SetWindowTextW(action->ToString().c_str());
					m_currentAction = std::move(action->action);
					return TRUE;
				}
			}
			break;
		case Type::eBind:
			{
				if (auto bind = Bind::GetBindFromMessage(pMsg); bind.has_value())
				{
					m_editAction.SetWindowTextW(bind->ToString().c_str());
					m_currentAction = std::move(bind->action);
					return TRUE;
				}
			}
			break;
		default:
			ASSERT(false);
			break;
		}
	}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

std::wstring CActionEditDlg::GetActionText() const
{
	ASSERT(m_currentAction.has_value());

	switch (m_editingType)
	{
	case Type::eBind:
		return m_currentAction->ToString();
	case Type::eAction:
		{
			MacrosAction macros;
			macros.action = m_currentAction.value();
			return macros.ToString();
		}
	default:
		ASSERT(false);
		return {};
	}
}

void CActionEditDlg::OnOK()
{
	if (!m_currentAction.has_value() &&
		(MessageBox(L"You need to enter action, press any key or mouse button", L"No action selected", MB_OKCANCEL) == IDOK))
	{
		return;
	}

	CDialogEx::OnOK();
}
