#include "pch.h"
#include "GameMate.h"
#include "afxdialogex.h"
#include "BindEditDlg.h"

IMPLEMENT_DYNAMIC(CBindEditDlg, CDialogEx)

CBindEditDlg::CBindEditDlg(CWnd* pParent, std::optional<Macros::Action> action)
	: CDialogEx(IDD_DIALOG_BIND_EDIT, pParent)
	, m_lastAction(action)
{
}

void CBindEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_ACTION, m_editAction);
}

BEGIN_MESSAGE_MAP(CBindEditDlg, CDialogEx)
END_MESSAGE_MAP()

[[nodiscard]] std::optional<Macros::Action> CBindEditDlg::ExecModal()
{
	if (DoModal() != IDOK)
		return std::nullopt;

	return m_lastAction;
}

BOOL CBindEditDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (m_lastAction.has_value())
        m_editAction.SetWindowTextW(m_lastAction->ToString().c_str());
    else
		m_editAction.SetWindowTextW(L"Enter action");

	return TRUE;
}

BOOL CBindEditDlg::PreTranslateMessage(MSG* pMsg)
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
		auto action = Macros::Action::GetActionFromMessage(pMsg, 0);
		if (action.has_value())
		{
			m_editAction.SetWindowTextW(action->ToString().c_str());
			m_lastAction = std::move(action);
			return TRUE;
		}
		break;
	}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CBindEditDlg::OnOK()
{
	if (!m_lastAction.has_value() &&
		(MessageBox(L"You need to enter action, press any key or mouse button", L"No action selected", MB_OKCANCEL) == IDOK))
	{
		return;
	}

	CDialogEx::OnOK();
}
