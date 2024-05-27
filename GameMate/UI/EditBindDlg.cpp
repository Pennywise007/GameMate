#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "EditBindDlg.h"

#include "InputManager.h"

IMPLEMENT_DYNAMIC(CEditBindDlg, CDialogEx)

CEditBindDlg::CEditBindDlg(CWnd* pParent, Bind& bind, bool editBind)
	: CDialogEx(IDD_DIALOG_EDIT_BIND, pParent)
	, m_currentBind(bind)
	, m_editBind(editBind)
{}

[[nodiscard]] std::optional<Bind> CEditBindDlg::EditBind(CWnd* pParent, const std::optional<Bind>& editBind)
{
	Bind bind = editBind.value_or(Bind{});
	if (CEditBindDlg(pParent, bind, editBind.has_value()).DoModal() != IDOK)
		return std::nullopt;

	return bind;
}

BOOL CEditBindDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_bindEditor = dynamic_cast<CBindEditorView*>(ActionsEditor::CreateEditor(this, ActionsEditor::Editor::eBind));
	ENSURE(m_bindEditor);

	CWnd* placeholder = GetDlgItem(IDC_STATIC_BIND_EDITOR_PLACEHOLDER);

	CRect editorRect;
	placeholder->GetWindowRect(editorRect);
	ScreenToClient(editorRect);

	CSize requiredEditorRect = m_bindEditor->GetTotalSize();

	CRect windowRect;
	GetWindowRect(windowRect);
	CSize extraSpace = requiredEditorRect - editorRect.Size();
	windowRect.right += extraSpace.cx;
	windowRect.bottom += extraSpace.cy;
	MoveWindow(windowRect);

	editorRect.right = editorRect.left + requiredEditorRect.cx;
	editorRect.bottom = editorRect.top + requiredEditorRect.cy;
	m_bindEditor->MoveWindow(editorRect);
	m_bindEditor->SetOwner(this);

	placeholder->ShowWindow(SW_HIDE);
	m_bindEditor->ShowWindow(SW_SHOW);

	if (!m_editBind)
		SetWindowText(L"Add bind");

	return TRUE;
}

void CEditBindDlg::OnOK()
{
	if (!m_bindEditor->CanClose())
		return;

	m_currentBind = m_bindEditor->GetBind();
	CDialogEx::OnOK();
}
