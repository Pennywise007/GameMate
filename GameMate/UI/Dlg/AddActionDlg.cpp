#include "pch.h"
#include "resource.h"
#include "UI/Dlg/AddActionDlg.h"

#include <core/Settings.h>

#include <ext/reflection/enum.h>

#include <Controls/Layout/Layout.h>

IMPLEMENT_DYNAMIC(CAddActionDlg, CDialogEx)

CAddActionDlg::CAddActionDlg(CWnd* pParent, Action& currentAction, bool addAction)
	: CDialogEx(IDD_DIALOG_ADD_ACTION, pParent)
	, m_currentAction(currentAction)
	, m_addAction(addAction)
{
}

std::optional<Action> CAddActionDlg::EditAction(CWnd* parent, std::optional<Action> currentAction)
{
	Action action = currentAction.value_or(Action{});
	if (CAddActionDlg(parent, action, !currentAction.has_value()).DoModal() == IDCANCEL)
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
	m_type.AddString(L"Set cursor position");
	m_type.AddString(L"Mouse move");
	m_type.AddString(L"Run script");

	switch (m_currentAction.type)
	{
	case Action::Type::eKeyOrMouseAction:
		m_type.SetCurSel(0);
		break;
	case Action::Type::eCursorPosition:
		m_type.SetCurSel(1);
		break;
	case Action::Type::eMouseMove:
	case Action::Type::eMouseMoveDirectInput:
		m_type.SetCurSel(2);
		break;
	case Action::Type::eRunScript:
		m_type.SetCurSel(3);
		break;
	default:
		static_assert(ext::reflection::get_enum_size<Action::Type>() == 5, "Not handled enum value");
		EXT_UNREACHABLE();
	}

	const auto action = std::make_shared<Action>(m_currentAction);
	m_editors = {
		InputEditor::CreateEditor(&m_editor, InputEditor::EditorType::eAction, action),
		InputEditor::CreateEditor(&m_editor, InputEditor::EditorType::eCursorPosition, action),
		InputEditor::CreateEditor(&m_editor, InputEditor::EditorType::eMouseMove, action),
		InputEditor::CreateEditor(&m_editor, InputEditor::EditorType::eScript, action),
	};

	CRect editorRect;
	m_editor.GetClientRect(editorRect);
	LONG editorsMaxWidth = editorRect.Width();
	for (auto& editor : m_editors)
	{
		editor->ShowWindow(SW_HIDE);
		editor->MoveWindow(editorRect, FALSE);
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

	if (!m_addAction)
		SetWindowText(L"Edit action");

	return TRUE;
}

void CAddActionDlg::OnOK()
{
	ASSERT(m_activeEditor);

	auto action = m_activeEditor->TryFinishDialog();
	if (!action)
		return;

	m_currentAction = *std::dynamic_pointer_cast<Action>(action);

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
