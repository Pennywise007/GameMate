#include "pch.h"
#include "GameMate.h"
#include "afxdialogex.h"
#include "BindEdit.h"

IMPLEMENT_DYNAMIC(CBindEdit, CDialogEx)

CBindEdit::CBindEdit(CWnd* pParent)
	: CDialogEx(IDD_DIALOG_BIND_EDIT, pParent)
{
}

void CBindEdit::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_ACTION, m_editAction);
}

BEGIN_MESSAGE_MAP(CBindEdit, CDialogEx)
END_MESSAGE_MAP()

std::optional<Macros::Action> CBindEdit::ExecModal()
{
	if (DoModal() != IDOK)
		return std::nullopt;

	return m_lastAction;
}

BOOL CBindEdit::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_editAction.SetWindowTextW(L"Enter action");

	return TRUE;
}

BOOL CBindEdit::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	{
		// Ignore stop recording click
		CRect rect;
		GetDlgItem(IDOK)->GetWindowRect(&rect);

		POINT cursorPoint;
		GetCursorPos(&cursorPoint);

		if (rect.PtInRect(cursorPoint))
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
