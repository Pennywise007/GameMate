#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "UI/Dlg/InputEditorDlg.h"

#include "InputManager.h"

IMPLEMENT_DYNAMIC(CInputEditorDlg, CDialogEx)

CInputEditorDlg::CInputEditorDlg(CWnd* pParent, InputEditor::EditorType editorType, std::shared_ptr<IBaseInput> baseInput, bool addingInput)
	: CDialogEx(IDD_DIALOG_INPUT_EDITOR, pParent)
	, m_addingInput(addingInput)
	, m_editorType(editorType)
	, m_baseInput(baseInput)
{
}

BOOL CInputEditorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_editor = InputEditor::CreateEditor(this, m_editorType, m_baseInput);
	EXT_EXPECT(m_editor);

	CWnd* placeholder = GetDlgItem(IDC_STATIC_INPUT_EDITOR_PLACEHOLDER);

	CRect editorRect;
	placeholder->GetWindowRect(editorRect);
	ScreenToClient(editorRect);

	CSize requiredEditorRect = m_editor->GetTotalSize();

	CRect windowRect;
	GetWindowRect(windowRect);
	CSize extraSpace = requiredEditorRect - editorRect.Size();
	windowRect.right += extraSpace.cx;
	windowRect.bottom += extraSpace.cy;
	MoveWindow(windowRect);

	editorRect.right = editorRect.left + requiredEditorRect.cx;
	editorRect.bottom = editorRect.top + requiredEditorRect.cy;
	m_editor->MoveWindow(editorRect);
	m_editor->SetOwner(this);

	placeholder->ShowWindow(SW_HIDE);
	m_editor->ShowWindow(SW_SHOW);

	switch (m_editorType)
	{
	case InputEditor::EditorType::eBind:
		SetWindowText(m_addingInput ? L"Add bind" : L"Edit bind");
		break;
	case InputEditor::EditorType::eKey:
		SetWindowText(m_addingInput ? L"Add key" : L"Edit key");
		break;
	default:
		EXT_UNREACHABLE();
	}

	return TRUE;
}

void CInputEditorDlg::OnOK()
{
	m_baseInput = m_editor->TryFinishDialog();
	if (!m_baseInput)
		return;

	CDialogEx::OnOK();
}

std::optional<Bind> CInputEditorDlg::EditBind(CWnd* pParent, const std::optional<Bind>& editBind)
{
	auto dlg = CInputEditorDlg(
		pParent, InputEditor::EditorType::eBind, std::make_shared<Bind>(editBind.value_or(Bind{})), !editBind.has_value());
	if (dlg.DoModal() != IDOK)
		return std::nullopt;

	EXT_EXPECT(dlg.m_baseInput);
	return *std::dynamic_pointer_cast<Bind>(dlg.m_baseInput);
}

[[nodiscard]] std::optional<Key> CInputEditorDlg::EditKey(CWnd* pParent, const std::optional<Key>& editKey)
{
	auto dlg = CInputEditorDlg(
		pParent, InputEditor::EditorType::eKey, std::make_shared<Key>(editKey.value_or(Key{})), !editKey.has_value());
	if (dlg.DoModal() != IDOK)
		return std::nullopt;

	EXT_EXPECT(dlg.m_baseInput);
	return *std::dynamic_pointer_cast<Key>(dlg.m_baseInput);
}
