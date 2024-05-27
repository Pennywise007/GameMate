#include "pch.h"
#include "resource.h"
#include "AddActionDlg.h"

#include <core/Settings.h>

#include <Controls/Layout/Layout.h>

IMPLEMENT_DYNAMIC(CAddActionDlg, CDialogEx)

CAddActionDlg::CAddActionDlg(CWnd* pParent, Action& currentAction, bool editAction)
	: CDialogEx(IDD_DIALOG_ADD_ACTION, pParent)
	, m_currentAction(currentAction)
	, m_editAction(editAction)
{
}

std::optional<Action> CAddActionDlg::EditAction(CWnd* parent, std::optional<Action> currentAction)
{
	Action action = currentAction.value_or(Action{});
	if (CAddActionDlg(parent, action, currentAction.has_value()).DoModal() == IDCANCEL)
		return std::nullopt;

	return action;
}

void CAddActionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_TYPE, m_type);
	DDX_Control(pDX, IDC_STATIC_EDITOR, m_editor);
}

BEGIN_MESSAGE_MAP(CAddActionDlg, CDialogEx)
	ON_CBN_SELENDOK(IDC_COMBO_TYPE, &CAddActionDlg::OnCbnSelendokComboType)
END_MESSAGE_MAP()

BOOL CAddActionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_type.AddString(L"Mouse or keyboard input");
	m_type.AddString(L"Mouse move");
	m_type.AddString(L"Run script");

	switch (m_currentAction.type)
	{
	case Action::Type::eKeyAction:
	case Action::Type::eMouseAction:
		m_type.SetCurSel(0);
		break;
	case Action::Type::eMouseMove:
		m_type.SetCurSel(1);
		break;
	case Action::Type::eRunScript:
		m_type.SetCurSel(2);
		break;
	default:
		EXT_UNREACHABLE();
	}

	m_editors = {
		ActionsEditor::CreateEditor(&m_editor, ActionsEditor::Editor::eAction),
		ActionsEditor::CreateEditor(&m_editor, ActionsEditor::Editor::eMouseMove),
		ActionsEditor::CreateEditor(&m_editor, ActionsEditor::Editor::eScript),
	};

	CRect editorRect;
	m_editor.GetClientRect(editorRect);
	LONG editorsMaxWidth = editorRect.Width();
	for (auto& editor : m_editors)
	{
		editor->ShowWindow(SW_HIDE);
		editor->MoveWindow(editorRect, FALSE);
		editor->SetAction(m_currentAction);
		editor->SetOwner(this);

		editorsMaxWidth = std::max(editorsMaxWidth, editor->GetTotalSize().cx);

		Layout::AnchorWindow(*editor, m_editor, { AnchorSide::eRight }, AnchorSide::eRight, 100);
		Layout::AnchorWindow(*editor, m_editor, { AnchorSide::eBottom }, AnchorSide::eBottom, 100);
	}

	// Fit dialog to maximum editor view width
	CRect windowRect;
	GetWindowRect(windowRect);
	windowRect.right = windowRect.left + windowRect.Width() - editorRect.Width() + editorsMaxWidth;
	MoveWindow(windowRect);

	updateCurrentView();

	LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);

	if (m_editAction)
		SetWindowText(L"Edit action");

	return TRUE;
}

void CAddActionDlg::OnOK()
{
	ASSERT(m_activeEditor);
	if (!m_activeEditor->CanClose())
		return;

	m_currentAction = m_activeEditor->GetAction();

	CDialogEx::OnOK();
}

BOOL CAddActionDlg::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			// Don't close window on enter
			return TRUE;
		case VK_ESCAPE:
			return TRUE;
		}
		break;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CAddActionDlg::OnCbnSelendokComboType()
{
	updateCurrentView();
}

void CAddActionDlg::updateCurrentView()
{
	if (m_activeEditor != nullptr)
		m_activeEditor->ShowWindow(SW_HIDE);

	auto curSel = m_type.GetCurSel();
	ASSERT(curSel < (int)m_editors.size());
	m_activeEditor = m_editors[curSel];

	CRect editorClientRect;
	m_editor.GetClientRect(editorClientRect);

	CRect windowRect;
	GetWindowRect(windowRect);

	CSize editorSize = m_activeEditor->GetTotalSize();
	int expectedHeight = windowRect.Height() - editorClientRect.Height() + editorSize.cy;

	SetWindowPos(0, 0, 0, windowRect.Width(), expectedHeight, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER);

	m_activeEditor->ShowWindow(SW_SHOW);
}
